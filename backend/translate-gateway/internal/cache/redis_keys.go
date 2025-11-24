package cache

import "fmt"

func KeyTask(taskID string) string {
	return fmt.Sprintf("task:%s", taskID)
}

func KeyUserTasks(userID string) string {
	return fmt.Sprintf("user:%s:tasks", userID)
}

func KeyTranslateCache(hash string) string {
	return fmt.Sprintf("translate:cache:%s", hash)
}
