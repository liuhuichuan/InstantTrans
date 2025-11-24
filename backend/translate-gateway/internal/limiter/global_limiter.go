package limiter

import (
	"context"
	"time"

	"github.com/redis/go-redis/v9"
)

// GlobalLimiter 用于在原有限流器外增加全局限制。
type GlobalLimiter struct {
	base        RateLimiter     // 原有限流器（memory / redis / lua）
	rdb         *redis.Client   // Redis 客户端（仅当使用 redis 模式时）
	globalKey   string          // 全局计数 key
	globalLimit int             // 全局请求上限
	window      time.Duration   // 时间窗口
	whitelist   map[string]bool // 白名单 key 集合
}

// NewGlobalLimiter 创建全局限流封装
func NewGlobalLimiter(base RateLimiter, redisAddr, redisPass string, globalLimit int, window time.Duration, whitelist []string) (*GlobalLimiter, error) {
	var client *redis.Client
	if redisAddr != "" {
		client = redis.NewClient(&redis.Options{
			Addr:     redisAddr,
			Password: redisPass,
			DB:       0,
		})
	}

	wl := make(map[string]bool)
	for _, k := range whitelist {
		wl[k] = true
	}

	return &GlobalLimiter{
		base:        base,
		rdb:         client,
		globalKey:   "ratelimit:global",
		globalLimit: globalLimit,
		window:      window,
		whitelist:   wl,
	}, nil
}

// Allow 包裹原有 Allow，同时检测全局请求量。
func (g *GlobalLimiter) Allow(ctx context.Context, key string) bool {
	// 白名单跳过全局限制
	if g.whitelist[key] {
		return g.base.Allow(ctx, key)
	}

	// 如果没配置全局限制，则直接走底层
	if g.globalLimit <= 0 || g.rdb == nil {
		return g.base.Allow(ctx, key)
	}

	// 全局计数逻辑（单 redis key）
	cnt, err := g.rdb.Incr(ctx, g.globalKey).Result()
	if err != nil {
		// Redis 出问题时，默认放行，避免可用性问题
		return g.base.Allow(ctx, key)
	}

	if cnt == 1 {
		g.rdb.Expire(ctx, g.globalKey, g.window)
	}

	if cnt > int64(g.globalLimit) {
		return false // 全局超限
	}

	// 全局通过，再看单 key 限流
	return g.base.Allow(ctx, key)
}
