package nats

import (
	"log"
	"time"

	"github.com/nats-io/nats.go"
)

type MessageHandler func(subject string, msg []byte)

type Consumer struct {
	conn *nats.Conn
	sub  *nats.Subscription
}

// NewConsumer 初始化 NATS 订阅者
func NewConsumer(url string) (*Consumer, error) {
	nc, err := nats.Connect(url,
		nats.Name("TranslateGatewayConsumer"),
		nats.ReconnectWait(2*time.Second),
		nats.MaxReconnects(10),
	)
	if err != nil {
		return nil, err
	}

	log.Println("[NATS] Consumer connected to:", nc.ConnectedUrlRedacted())
	return &Consumer{conn: nc}, nil
}

// Subscribe 订阅主题并处理消息
func (c *Consumer) Subscribe(subject string, handler MessageHandler) error {
	sub, err := c.conn.Subscribe(subject, func(msg *nats.Msg) {
		handler(msg.Subject, msg.Data)
	})
	if err != nil {
		return err
	}
	c.sub = sub
	log.Printf("[NATS] Subscribed to subject: %s\n", subject)
	return nil
}

// Close 关闭连接和订阅
func (c *Consumer) Close() {
	if c.sub != nil {
		_ = c.sub.Unsubscribe()
	}
	if c.conn != nil {
		c.conn.Close()
	}
}
