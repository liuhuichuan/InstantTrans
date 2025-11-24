package limiter

import (
	"context"
	"fmt"
	"time"
	"translategateway/config"
)

type RateLimiter interface {
	Allow(ctx context.Context, key string) bool
}

func NewRateLimiter(mode string, limit int, window time.Duration, redisCfg config.RedisSection) (RateLimiter, error) {
	switch mode {
	case "memory":
		return NewMemoryLimiter(limit, window), nil
	case "redis_lock":
		return NewRedisLockLimiter(fmt.Sprintf("%s:%d", redisCfg.Single.Host, redisCfg.Single.Port), redisCfg.Single.Password, limit, window)
	case "redis_lua":
		return NewRedisLuaLimiter(fmt.Sprintf("%s:%d", redisCfg.Single.Host, redisCfg.Single.Port), redisCfg.Single.Password, limit, window)
	default:
		return nil, fmt.Errorf("unknown limiter mode: %s", mode)
	}
}
