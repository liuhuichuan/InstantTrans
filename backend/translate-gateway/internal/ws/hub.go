package ws

import (
	"encoding/json"
	"log"
	"sync"
	"translategateway/internal/types"
)

// ==================== Hub 管理器 ====================

type Hub struct {
	mu         sync.RWMutex
	clients    map[string]*Client
	register   chan *Client
	unregister chan *Client
}

func NewHub() *Hub {
	return &Hub{
		clients:    make(map[string]*Client),
		register:   make(chan *Client),
		unregister: make(chan *Client),
	}
}

// 运行 Hub 主循环
func (h *Hub) Run() {
	for {
		select {
		case client := <-h.register:
			h.mu.Lock()
			h.clients[client.ID] = client
			h.mu.Unlock()
			log.Printf("[Hub] Client connected: %s", client.ID)

		case client := <-h.unregister:
			h.mu.Lock()
			if _, ok := h.clients[client.ID]; ok {
				delete(h.clients, client.ID)
				close(client.Send)
				log.Printf("[Hub] Client disconnected: %s", client.ID)
			}
			h.mu.Unlock()
		}
	}
}

// 添加客户端
func (h *Hub) AddClient(c *Client) {
	h.register <- c
}

// 移除客户端
func (h *Hub) RemoveClient(id string) {
	h.mu.Lock()
	defer h.mu.Unlock()
	if c, ok := h.clients[id]; ok {
		delete(h.clients, id)
		close(c.Send)
	}
}

// 单播：发送给特定客户端
func (h *Hub) SendToClient(msg *types.TranslateResponse, clientID string) {
	h.mu.RLock()
	defer h.mu.RUnlock()
	if client, ok := h.clients[clientID]; ok {
		data, _ := json.Marshal(msg)

		select {
		case client.Send <- data:
		default:
			log.Printf("[Hub] Client %s send buffer full, dropping message", clientID)
		}
	}
}
