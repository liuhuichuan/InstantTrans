package translator

import (
	"context"
	"fmt"
	"github.com/sashabaranov/go-openai"
)

type Client struct {
	client *openai.Client
	model  string
}

func NewClient(cfg *Config) *Client {
	config := openai.DefaultConfig(cfg.APIKey)
	config.BaseURL = cfg.BaseURL // ✅ 指向你的自定义 OpenAI 格式兼容API
	return &Client{
		client: openai.NewClientWithConfig(config),
		model:  cfg.Model,
	}
}

func (t *Client) Translate(ctx context.Context, req *TranslateRequest) (*TranslateResult, error) {
	//prompt := fmt.Sprintf("Translate the following text from %s to %s:\n%s", req.FromLang, req.ToLang, req.Text)
	prompt := fmt.Sprintf("你是一个翻译助手，负责将日语翻译为中文。只返回地道的中文翻译，不要解释")
	userPrompt := fmt.Sprintf("Today is a good day")
	answerPrompt := fmt.Sprintf("今天是个好日子")
	//userPrompt := fmt.Sprintf("もうご飯を食べましたか？")
	//answerPrompt := fmt.Sprintf("你吃饭了吗？")

	resp, err := t.client.CreateChatCompletion(ctx, openai.ChatCompletionRequest{
		Model: t.model,
		Messages: []openai.ChatCompletionMessage{
			{Role: openai.ChatMessageRoleSystem, Content: prompt},
			{Role: openai.ChatMessageRoleUser, Content: userPrompt},
			{Role: openai.ChatMessageRoleAssistant, Content: answerPrompt},
			{Role: openai.ChatMessageRoleUser, Content: req.Text},
		},
		Temperature: 0.2,
	})
	if err != nil {
		return &TranslateResult{ID: req.ID, Success: false, Error: err.Error()}, err
	}

	return &TranslateResult{
		ID:      req.ID,
		Text:    resp.Choices[0].Message.Content,
		Success: true,
	}, nil
}
