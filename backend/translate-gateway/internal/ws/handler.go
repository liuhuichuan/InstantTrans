package ws

import (
	"context"
	"encoding/json"
	"fmt"
	"log"
	"net/http"
	"time"
	"translategateway/config"
	"translategateway/internal/cache"
	"translategateway/internal/limiter"

	"github.com/google/uuid"
	"github.com/gorilla/websocket"
	"translategateway/internal/nats"
	"translategateway/internal/types"
)

type Handler struct {
	Hub    *Hub
	Nats   *nats.NatsClient
	Config *config.Config
}

var upgrader = websocket.Upgrader{
	CheckOrigin: func(r *http.Request) bool { return true },
}

func (h *Handler) ServeWS(w http.ResponseWriter, r *http.Request) {
	conn, err := upgrader.Upgrade(w, r, nil)
	if err != nil {
		log.Println("[WS] Upgrade error:", err)
		return
	}

	// 对ip做过滤
	cfgLimiter := h.Config.Limiter
	window, _ := time.ParseDuration(cfgLimiter.Window)

	// 创建基础限流器
	baseLimiter, err := limiter.NewRateLimiter(cfgLimiter.Mode, cfgLimiter.Limit, window, h.Config.Redis)
	if err != nil {
		panic(err)
	}

	// 包裹全局限流
	globalLimiter, err := limiter.NewGlobalLimiter(
		baseLimiter,
		fmt.Sprintf("%s:%d", h.Config.Redis.Single.Host, h.Config.Redis.Single.Port),
		h.Config.Redis.Single.Password,
		cfgLimiter.GlobalLimit,
		window,
		cfgLimiter.WhiteList,
	)
	if err != nil {
		panic(err)
	}

	if globalLimiter.Allow(context.Background(), conn.RemoteAddr().String()) {
		client := &Client{
			Conn: conn,
			Hub:  h.Hub,
			Send: make(chan []byte, 256),
		}

		// 启动读写循环
		go client.readPump(h.handleTranslateRequest)
		go client.writePump()
	} else {
		log.Fatal("exceed global limit")
	}
}

func (h *Handler) handleTranslateRequest(c *Client, msg []byte) {
	var req types.TranslateRequest
	if err := json.Unmarshal(msg, &req); err != nil {
		log.Println("[WS] Invalid json:", err)
		return
	}

	task, err := cache.GetTaskResult(req.RequestID)
	if err == nil && task.Status == "done" {
		log.Printf("[WS] got same message from redis, taskid: %s\n", task.TaskID)
		response := types.TranslateResponse{
			ClientID:  task.UserID,
			RequestID: task.TaskID,
			LangFrom:  task.LangFrom,
			LangTo:    task.LangTo,
			Result:    task.ResultText,
		}
		h.Hub.SendToClient(&response, task.UserID)
		return
	}

	// 第一次收到消息时注册 client
	if c.ID == "" {
		if req.ClientID == "" {
			req.ClientID = uuid.NewString()
		}
		c.ID = req.ClientID
		h.Hub.AddClient(c)
		log.Printf("[Hub] Registered new client: %s", c.ID)
	}

	if req.RequestID == "" {
		req.RequestID = uuid.NewString()
	}

	log.Printf("[Worker] Received: client=%s, requestID=%s, text=%s",
		req.ClientID, req.RequestID, req.SourceText)

	if err := h.Nats.PublishTranslate(&req); err != nil {
		log.Println("[WS] NATS publish error:", err)
	}
}
