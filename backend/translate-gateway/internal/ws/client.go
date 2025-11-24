package ws

import (
	"log"
	"time"

	"github.com/gorilla/websocket"
)

// ==================== 客户端结构 ====================

type Client struct {
	ID   string
	Conn *websocket.Conn
	Send chan []byte
	Hub  *Hub
}

// ==================== 客户端行为 ====================

// 从客户端读取消息
func (c *Client) readPump(onMessage func(client *Client, msg []byte)) {
	defer func() {
		c.Hub.RemoveClient(c.ID)
		c.Conn.Close()
	}()

	c.Conn.SetReadLimit(2048)
	c.Conn.SetReadDeadline(time.Now().Add(60 * time.Second))
	c.Conn.SetPongHandler(func(string) error {
		c.Conn.SetReadDeadline(time.Now().Add(60 * time.Second))
		return nil
	})
	log.Println("[WS] SetReadDeadline:")
	for {
		_, msg, err := c.Conn.ReadMessage()
		if err != nil {
			log.Println("[WS] Read error:", err)
			break
		}
		onMessage(c, msg)
	}
}

// 向客户端写消息
func (c *Client) writePump() {
	ticker := time.NewTicker(50 * time.Second)
	defer func() {
		ticker.Stop()
		c.Conn.Close()
	}()

	for {
		select {
		case msg, ok := <-c.Send:
			c.Conn.SetWriteDeadline(time.Now().Add(10 * time.Second))
			if !ok {
				// 通道关闭
				c.Conn.WriteMessage(websocket.CloseMessage, []byte{})
				return
			}
			if err := c.Conn.WriteMessage(websocket.TextMessage, msg); err != nil {
				log.Println("[WS] Write error:", err)
				return
			}
		case <-ticker.C:
			c.Conn.SetWriteDeadline(time.Now().Add(10 * time.Second))
			if err := c.Conn.WriteMessage(websocket.PingMessage, nil); err != nil {
				return
			}
		}
	}
}
