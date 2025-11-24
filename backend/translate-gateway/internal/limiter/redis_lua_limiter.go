package limiter

import (
	"context"
	_ "embed"
	"fmt"
	"time"

	"github.com/redis/go-redis/v9"
)

//go:embed ratelimit.lua
var luaScript string

type redisLuaLimiter struct {
	rdb      *redis.Client
	limit    int
	windowMs int64
	script   *redis.Script
}

func NewRedisLuaLimiter(addr, pass string, limit int, window time.Duration) (RateLimiter, error) {
	client := redis.NewClient(&redis.Options{
		Addr:     addr,
		Password: pass,
		DB:       0,
	})
	if err := client.Ping(context.Background()).Err(); err != nil {
		return nil, fmt.Errorf("redis connect failed: %w", err)
	}

	return &redisLuaLimiter{
		rdb:      client,
		limit:    limit,
		windowMs: window.Milliseconds(),
		script:   redis.NewScript(luaScript),
	}, nil
}

func (r *redisLuaLimiter) Allow(ctx context.Context, key string) bool {
	result, err := r.script.Run(ctx, r.rdb, []string{key}, r.limit, r.windowMs).Int()
	if err != nil {
		return false
	}
	return result == 1
}
