package cache

import (
	"context"
	"fmt"
	"log"
	"sync"
	"time"

	"github.com/redis/go-redis/v9"
	"translategateway/config"
)

var (
	rdbInstance redis.UniversalClient
	once        sync.Once
)

// InitRedisClient 初始化 Redis 客户端（整个服务启动时调用一次）
func InitRedisClient(cfg config.RedisSection) {
	once.Do(func() {
		switch cfg.Mode {
		case "single":
			rdbInstance = redis.NewClient(&redis.Options{
				Addr:     fmt.Sprintf("%s:%d", cfg.Single.Host, cfg.Single.Port),
				Password: cfg.Single.Password,
				DB:       cfg.Single.DB,
			})
		case "sentinel":
			rdbInstance = redis.NewFailoverClient(&redis.FailoverOptions{
				MasterName:    cfg.Sentinel.MasterName,
				SentinelAddrs: cfg.Sentinel.Nodes,
				Password:      cfg.Sentinel.Password,
				DB:            cfg.Sentinel.DB,
			})
		case "cluster":
			rdbInstance = redis.NewClusterClient(&redis.ClusterOptions{
				Addrs:    cfg.Cluster.Nodes,
				Password: cfg.Cluster.Password,
			})
		default:
			log.Fatalf("❌ 未知 Redis 模式: %s", cfg.Mode)
		}

		ctx, cancel := context.WithTimeout(context.Background(), 3*time.Second)
		defer cancel()
		if err := rdbInstance.Ping(ctx).Err(); err != nil {
			log.Fatalf("❌ Redis 连接失败: %v", err)
		}
		log.Println("✅ Redis 初始化成功")
	})
}

// RDB 返回全局 Redis 客户端实例（线程安全）
func RDB() redis.UniversalClient {
	if rdbInstance == nil {
		log.Fatalf("❌ Redis 客户端未初始化，请先调用 cache.InitRedisClient()")
	}
	return rdbInstance
}

func CloseRedis() {
	if rdbInstance != nil {
		_ = rdbInstance.Close()
	}
}
