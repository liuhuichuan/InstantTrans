package nats

import (
	"log"
	"time"

	"github.com/nats-io/nats.go"
)

type Producer struct {
	conn *nats.Conn
}

// NewProducer 初始化 NATS 连接
func NewProducer(url string) (*Producer, error) {
	nc, err := nats.Connect(url,
		nats.Name("TranslateGatewayProducer"),
		nats.ReconnectWait(2*time.Second),
		nats.MaxReconnects(10),
	)
	if err != nil {
		return nil, err
	}

	log.Println("[NATS] Producer connected to:", nc.ConnectedUrlRedacted())
	return &Producer{conn: nc}, nil
}

// Publish 发布消息到指定主题
func (p *Producer) Publish(subject string, data []byte) error {
	if p.conn == nil || !p.conn.IsConnected() {
		return nats.ErrConnectionClosed
	}
	return p.conn.Publish(subject, data)
}

// Close 关闭连接
func (p *Producer) Close() {
	if p.conn != nil {
		p.conn.Close()
	}
}
