package limiter

import (
	"context"
	"fmt"
	"time"

	"github.com/redis/go-redis/v9"
)

type redisLockLimiter struct {
	rdb    *redis.Client
	limit  int
	window time.Duration
}

func NewRedisLockLimiter(addr, pass string, limit int, window time.Duration) (RateLimiter, error) {
	client := redis.NewClient(&redis.Options{
		Addr:     addr,
		Password: pass,
		DB:       0,
	})
	if err := client.Ping(context.Background()).Err(); err != nil {
		return nil, fmt.Errorf("redis connect failed: %w", err)
	}
	return &redisLockLimiter{rdb: client, limit: limit, window: window}, nil
}

func (r *redisLockLimiter) Allow(ctx context.Context, key string) bool {
	lockKey := "lock:" + key
	rateKey := "rate:" + key

	// 尝试获取锁，设置短期过期防止死锁
	ok, _ := r.rdb.SetNX(ctx, lockKey, 1, 200*time.Millisecond).Result()
	if !ok {
		return false // 其他客户端正在更新
	}
	defer r.rdb.Del(ctx, lockKey)

	val, _ := r.rdb.Get(ctx, rateKey).Int()
	if val >= r.limit {
		return false
	}

	pipe := r.rdb.TxPipeline()
	pipe.Incr(ctx, rateKey)
	pipe.Expire(ctx, rateKey, r.window)
	_, _ = pipe.Exec(ctx)
	return true
}
