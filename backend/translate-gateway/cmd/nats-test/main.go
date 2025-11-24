package main

import (
	"log"
	"time"
	"translategateway/internal/nats"
)

func main() {
	url := "nats://127.0.0.1:4222"
	consumer, err := nats.NewConsumer(url)
	if err != nil {
		log.Fatal(err)
	}
	defer consumer.Close()

	_ = consumer.Subscribe("translate.request", func(subject string, msg []byte) {
		log.Printf("📥 Received from [%s]: %s", subject, string(msg))
	})

	// 启动生产者
	producer, err := nats.NewProducer(url)
	if err != nil {
		log.Fatal(err)
	}
	defer producer.Close()

	// 模拟发布消息
	for i := 1; i <= 3; i++ {
		msg := []byte("hello nats " + time.Now().Format(time.RFC3339))
		if err := producer.Publish("translate.request", msg); err != nil {
			log.Println("publish error:", err)
		}
		time.Sleep(1 * time.Second)
	}

	time.Sleep(2 * time.Second)
}
