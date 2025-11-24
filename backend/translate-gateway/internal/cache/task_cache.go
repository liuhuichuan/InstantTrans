package cache

import (
	"context"
	"encoding/json"
	"time"
)

type TaskResult struct {
	TaskID     string `json:"taskId"`
	UserID     string `json:"userId"`
	SourceText string `json:"sourceText"`
	ResultText string `json:"resultText"`
	Status     string `json:"status"` // pending, done, failed
	CreatedAt  int64  `json:"createdAt"`
	LangFrom   string `json:"lang_from"`
	LangTo     string `json:"lang_to"`
}

func SaveTaskResult(task *TaskResult, ttl time.Duration) error {
	data, _ := json.Marshal(task)
	return RDB().Set(context.Background(), KeyTask(task.TaskID), data, ttl).Err()
}

func GetTaskResult(taskID string) (*TaskResult, error) {
	val, err := RDB().Get(context.Background(), KeyTask(taskID)).Result()
	if err != nil {
		return nil, err
	}
	var task TaskResult
	if err := json.Unmarshal([]byte(val), &task); err != nil {
		return nil, err
	}
	return &task, nil
}
