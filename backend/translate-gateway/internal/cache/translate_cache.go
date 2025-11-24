package cache

import (
	"context"
	"time"
)

func GetCachedTranslation(hash string) (string, error) {
	return RDB().Get(context.Background(), KeyTranslateCache(hash)).Result()
}

func SaveCachedTranslation(hash, text string, ttl time.Duration) error {
	return RDB().Set(context.Background(), KeyTranslateCache(hash), text, ttl).Err()
}
