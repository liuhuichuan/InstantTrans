package main

import (
	"context"
	"fmt"
	"time"
	"translategateway/config"
	"translategateway/internal/limiter"
)

func main() {
	var cfg0 = config.LoadConfig("../../config/config.yaml")
	cfg := cfg0.Limiter
	window, _ := time.ParseDuration(cfg.Window)

	// 创建基础限流器
	baseLimiter, err := limiter.NewRateLimiter(cfg.Mode, cfg.Limit, window, cfg0.Redis)
	if err != nil {
		panic(err)
	}

	// 包裹全局限流
	globalLimiter, err := limiter.NewGlobalLimiter(
		baseLimiter,
		fmt.Sprintf("%s:%d", cfg0.Redis.Single.Host, cfg0.Redis.Single.Port),
		cfg0.Redis.Single.Password,
		cfg.GlobalLimit,
		window,
		cfg.WhiteList,
	)
	if err != nil {
		panic(err)
	}

	// 使用全局限流器
	for i := 0; i < 30; i++ {
		ok := globalLimiter.Allow(context.Background(), "ip:127.0.0.1")
		fmt.Printf("%02d -> %v\n", i, ok)
		time.Sleep(100 * time.Millisecond)
	}
}
