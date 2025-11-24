package limiter

import (
	"context"
	"sync"
	"time"
)

type memoryLimiter struct {
	mu     sync.Mutex
	tokens map[string]*tokenBucket
	limit  int
	window time.Duration
}

type tokenBucket struct {
	lastRefill time.Time
	tokens     int
}

func NewMemoryLimiter(limit int, window time.Duration) RateLimiter {
	return &memoryLimiter{
		tokens: make(map[string]*tokenBucket),
		limit:  limit,
		window: window,
	}
}

func (m *memoryLimiter) Allow(_ context.Context, key string) bool {
	m.mu.Lock()
	defer m.mu.Unlock()

	tb, ok := m.tokens[key]
	now := time.Now()
	if !ok {
		tb = &tokenBucket{lastRefill: now, tokens: m.limit - 1}
		m.tokens[key] = tb
		return true
	}

	// 若窗口期已过，则重置计数
	if now.Sub(tb.lastRefill) >= m.window {
		tb.tokens = m.limit
		tb.lastRefill = now
	}

	if tb.tokens > 0 {
		tb.tokens--
		return true
	}
	return false
}
