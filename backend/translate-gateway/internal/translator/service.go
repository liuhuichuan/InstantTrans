package translator

import (
	"context"
	"encoding/json"
	"fmt"
	"github.com/nats-io/nats.go"
	"log"
)

type Service struct {
	natsConn *nats.Conn
	client   *Client
}

func NewService(nc *nats.Conn, client *Client) *Service {
	return &Service{natsConn: nc, client: client}
}

func (s *Service) Start(ctx context.Context) error {
	sub, err := s.natsConn.Subscribe("translate.request", func(msg *nats.Msg) {
		var req TranslateRequest
		if err := json.Unmarshal(msg.Data, &req); err != nil {
			log.Printf("unmarshal error: %v", err)
			return
		}

		res, err := s.client.Translate(ctx, &req)
		if err != nil {
			log.Printf("translate error: %v", err)
		}

		data, _ := json.Marshal(res)
		if err := s.natsConn.Publish("translate.result", data); err != nil {
			log.Printf("publish error: %v", err)
		}
	})
	if err != nil {
		return fmt.Errorf("failed to subscribe translate.request: %w", err)
	}

	log.Println("translator service subscribed to translate.request")
	<-ctx.Done()
	sub.Unsubscribe()
	return nil
}
