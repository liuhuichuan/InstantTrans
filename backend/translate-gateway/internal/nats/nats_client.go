package nats

import (
	"encoding/json"
	"fmt"
	"log"
	"time"

	"github.com/google/uuid"
	"github.com/nats-io/nats.go"
	"translategateway/internal/types"
)

type NatsClient struct {
	conn *nats.Conn
}

func (n *NatsClient) Close() {
	if n.conn != nil {
		n.conn.Close()
	}
}

func NewNatsClient(url string) (*NatsClient, error) {
	nc, err := nats.Connect(url,
		nats.Name("TranslateGateway"),
		nats.ReconnectWait(2*time.Second),
		nats.MaxReconnects(10),
	)
	if err != nil {
		return nil, err
	}

	log.Println("[NATS] Connected:", nc.ConnectedUrlRedacted())
	return &NatsClient{conn: nc}, nil
}

// 发布请求到翻译服务
func (n *NatsClient) PublishTranslate(req *types.TranslateRequest) error {
	if req.RequestID == "" {
		req.RequestID = uuid.NewString()
	}
	data, _ := json.Marshal(req)
	return n.conn.Publish("origin", data)
}

func (n *NatsClient) PublishResult(res *types.TranslateResponse) error {
	data, err := json.Marshal(res)
	if err != nil {
		return err
	}
	return n.conn.Publish("result", data)
}

// 订阅翻译结果
func (n *NatsClient) SubscribeResults(onMessage func(result *types.TranslateResponse)) error {
	_, err := n.conn.Subscribe("result", func(msg *nats.Msg) {
		var res types.TranslateResponse
		if err := json.Unmarshal(msg.Data, &res); err != nil {
			fmt.Println("[NATS] Bad message:", err)
			return
		}
		onMessage(&res)
	})
	return err
}

func (n *NatsClient) SubscribeTranslate(onMessage func(*types.TranslateRequest)) error {
	_, err := n.conn.Subscribe("origin", func(msg *nats.Msg) {
		var req types.TranslateRequest
		if err := json.Unmarshal(msg.Data, &req); err != nil {
			log.Println("[NATS] Failed to parse origin:", err)
			return
		}
		onMessage(&req)
	})
	return err
}
