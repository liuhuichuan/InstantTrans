package main

import (
	"fmt"
	"log"
	"net/http"
	"os"
	"os/signal"
	"syscall"
	"translategateway/config"
	"translategateway/internal/cache"
	"translategateway/internal/nats"
	"translategateway/internal/types"
	"translategateway/internal/worker"
	"translategateway/internal/ws"
)

func main() {

	// ========= 0. 启动redis ========================
	var cfg = config.LoadConfig("config/config.yaml")
	cache.InitRedisClient(cfg.Redis)

	natsURL := "nats://localhost:4222"
	nc, err := nats.NewNatsClient(natsURL)
	if err != nil {
		log.Fatalf("❌ Failed to connect NATS: %v", err)
	}
	defer nc.Close()
	log.Printf("✅ Connected to NATS: %s", natsURL)

	// ============ 2. 初始化 WebSocket Hub ============
	hub := ws.NewHub()
	go hub.Run()

	worker.StartWorker("nats://127.0.0.1:4222")

	// ============ 3. 启动 WebSocket Handler ============
	handler := &ws.Handler{
		Hub:    hub,
		Nats:   nc,
		Config: cfg,
	}
	// WebSocket endpoint
	http.HandleFunc("/ws", handler.ServeWS)

	// ============ 4. 订阅翻译结果队列 ============
	err = nc.SubscribeResults(func(msg *types.TranslateResponse) {
		// 将翻译结果发回对应的客户端
		hub.SendToClient(msg, msg.ClientID)
	})
	if err != nil {
		log.Fatalf("❌ Failed to subscribe result queue: %v", err)
	}
	log.Println("✅ Subscribed to 'translate_result' queue")

	fmt.Println("✅ WebSocket server started at :8080/ws")
	// ============ 5. 启动 HTTP 服务器 ============
	serverAddr := ":8080"
	go func() {
		log.Printf("🚀 WebSocket server started on %s", serverAddr)
		if err := http.ListenAndServe(serverAddr, nil); err != nil {
			log.Fatalf("❌ HTTP server error: %v", err)
		}
	}()
	// ============ 6. 等待中断信号优雅退出 ============
	sigChan := make(chan os.Signal, 1)
	signal.Notify(sigChan, syscall.SIGINT, syscall.SIGTERM)
	<-sigChan
	log.Println("🧹 Shutting down gracefully...")
}
