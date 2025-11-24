package worker

import (
	"context"
	"fmt"
	"log"
	"time"
	"translategateway/internal/cache"
	"translategateway/internal/types"

	"translategateway/internal/nats"
	"translategateway/internal/translator"
)

func StartWorker(natsURL string) {

	natsClient, _ := nats.NewNatsClient(natsURL)

	cfg, err := translator.LoadConfig("./config/config.yaml")
	if err != nil {
		log.Fatal(err)
	}

	client := translator.NewClient(cfg)

	err = natsClient.SubscribeTranslate(func(request *types.TranslateRequest) {
		fmt.Printf("[Worker] Received: %s\n", request.SourceText)

		result, err := client.Translate(context.Background(), &translator.TranslateRequest{
			ID:       "demo-1",
			FromLang: "ja",
			ToLang:   "zh",
			Text:     request.SourceText,
		})
		if err != nil {
			log.Fatal(err)
		}

		fmt.Println("Translation result:", result.Text)

		res := types.TranslateResponse{
			RequestID: request.RequestID,
			ClientID:  request.ClientID,
			Result:    result.Text,
			LangFrom:  request.LangFrom,
			LangTo:    request.LangTo,
		}
		// 发布到 translate 队列
		err = natsClient.PublishResult(&res)
		if err != nil {
			log.Fatal(err)
		}

		task := cache.TaskResult{
			TaskID:     request.RequestID,
			UserID:     request.ClientID,
			SourceText: request.SourceText,
			ResultText: result.Text,
			LangFrom:   request.LangFrom,
			LangTo:     request.LangTo,
			Status:     "done",
			CreatedAt:  time.Now().Unix(),
		}
		handleTranslationResult(&task)
	})
	if err != nil {
		return
	}
}

func handleTranslationResult(task *cache.TaskResult) {
	// 写单任务结果
	_ = cache.SaveTaskResult(task, 24*time.Hour)

	// 写用户任务列表
	//_ = cache.AddUserTask(task.UserID, task.TaskID)

	// 写翻译文本缓存（用hash）
	//hash := hashText(task.SourceText)
	//_ = cache.SaveCachedTranslation(hash, task.ResultText, 7*24*time.Hour)
}
