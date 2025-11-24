package translator

import (
	"errors"
	"time"

	"github.com/spf13/viper"
)

// Config 定义 translator 模块的所有配置项
type Config struct {
	// OpenAI / 兼容 API
	BaseURL string        // 例如: "https://api.zhisuai.com/v1"
	APIKey  string        // 平台提供的 API Key
	Model   string        // 模型名，例如 "gpt-4o-mini"
	Timeout time.Duration // 网络请求超时

	// NATS 相关
	NatsURL        string // NATS 连接地址
	SubjectRequest string // 翻译请求的 Subject
	SubjectResult  string // 翻译结果的 Subject
	NatsQueueGroup string // 消费者组名称

	// 行为参数
	MaxRetries int // 调用失败时的最大重试次数
}

// LoadConfig 读取 YAML 配置文件（支持环境变量覆盖）
// 例如: TRANSLATOR_API_KEY 会覆盖 api_key 字段
func LoadConfig(path string) (*Config, error) {
	v := viper.New()
	v.SetConfigFile(path)
	v.SetConfigType("yaml")

	// 环境变量前缀
	v.SetEnvPrefix("TRANSLATOR")
	v.AutomaticEnv()

	// 默认值
	v.SetDefault("base_url", "https://api.openai.com/v1")
	v.SetDefault("model", "gpt-4o-mini")
	v.SetDefault("timeout_seconds", 15)
	v.SetDefault("nats.url", "nats://localhost:4222")
	v.SetDefault("nats.subject_request", "translate.request")
	v.SetDefault("nats.subject_result", "translate.result")
	v.SetDefault("nats.queue_group", "translator-workers")
	v.SetDefault("max_retries", 2)

	// 尝试读取配置文件
	_ = v.ReadInConfig() // 不报错，即使文件不存在

	cfg := &Config{
		BaseURL:        v.GetString("base_url"),
		APIKey:         v.GetString("api_key"),
		Model:          v.GetString("model"),
		Timeout:        time.Duration(v.GetInt("timeout_seconds")) * time.Second,
		NatsURL:        v.GetString("nats.url"),
		SubjectRequest: v.GetString("nats.subject_request"),
		SubjectResult:  v.GetString("nats.subject_result"),
		NatsQueueGroup: v.GetString("nats.queue_group"),
		MaxRetries:     v.GetInt("max_retries"),
	}

	if cfg.APIKey == "" {
		return nil, errors.New("api_key is required (from config file or TRANSLATOR_API_KEY environment variable)")
	}

	return cfg, nil
}
