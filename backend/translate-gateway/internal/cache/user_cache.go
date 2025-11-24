package cache

import (
	"context"
	"time"
)

func AddUserTask(userID, taskID string) error {
	key := KeyUserTasks(userID)
	pipe := RDB().TxPipeline()
	pipe.LPush(context.Background(), key, taskID)
	pipe.LTrim(context.Background(), key, 0, 49) // 保留最近50条
	pipe.Expire(context.Background(), key, 30*24*time.Hour)
	_, err := pipe.Exec(context.Background())
	return err
}

func GetUserTasks(userID string, limit int64) ([]string, error) {
	return RDB().LRange(context.Background(), KeyUserTasks(userID), 0, limit-1).Result()
}
