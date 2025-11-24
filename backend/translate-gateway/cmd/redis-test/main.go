package main

import (
	"log"
	"time"
	"translategateway/config"
	"translategateway/internal/cache"
)

func main() {
	var cfg = config.LoadConfig("../../config/config.yaml")
	cache.InitRedisClient(cfg.Redis)
	log.Println("✅ Redis 初始化成功")

	task := cache.TaskResult{
		TaskID:     "12345",
		UserID:     "user_001",
		SourceText: "Hello world",
		ResultText: "你好，世界",
		Status:     "done",
		CreatedAt:  time.Now().Unix(),
	}
	err := cache.SaveTaskResult(&task, 24*time.Hour)
	if err != nil {
		log.Fatal(err)
	}
	result, err1 := cache.GetTaskResult("12345")
	if err1 != nil {
		log.Fatal(err1)
	}
	log.Println(result.ResultText)
}
