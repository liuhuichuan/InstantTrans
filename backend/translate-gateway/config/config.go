package config

import (
	"log"
	"os"

	"gopkg.in/yaml.v3"
)

type Config struct {
	BaseURL        string `yaml:"base_url"`
	APIKey         string `yaml:"api_key"`
	Model          string `yaml:"model"`
	TimeoutSeconds int    `yaml:"timeout_seconds"`

	MaxRetries int `yaml:"max_retries"`

	NATS struct {
		URL            string `yaml:"url"`
		SubjectRequest string `yaml:"subject_request"`
		SubjectResult  string `yaml:"subject_result"`
		QueueGroup     string `yaml:"queue_group"`
	} `yaml:"nats"`

	Redis   RedisSection `yaml:"redis"`
	Limiter RateLimit    `yaml:"rate_limit"`
}

type RedisSection struct {
	Mode string `yaml:"mode"`

	Single struct {
		Host     string `yaml:"host"`
		Port     int    `yaml:"port"`
		Password string `yaml:"password"`
		DB       int    `yaml:"db"`
	} `yaml:"single"`

	Sentinel struct {
		MasterName string   `yaml:"master_name"`
		Nodes      []string `yaml:"nodes"`
		Password   string   `yaml:"password"`
		DB         int      `yaml:"db"`
	} `yaml:"sentinel"`

	Cluster struct {
		Nodes    []string `yaml:"nodes"`
		Password string   `yaml:"password"`
	} `yaml:"cluster"`
}

type RateLimit struct {
	Mode        string   `yaml:"mode"`
	Limit       int      `yaml:"limit"`
	Window      string   `yaml:"window"`
	GlobalLimit int      `yaml:"global_limit"`
	WhiteList   []string `yaml:"whitelist"`
}

// LoadConfig loads config.yaml and unmarshals it into Config struct
func LoadConfig(path string) *Config {
	data, err := os.ReadFile(path)
	if err != nil {
		log.Fatalf("❌ 读取配置文件失败: %v", err)
	}

	var cfg Config
	if err := yaml.Unmarshal(data, &cfg); err != nil {
		log.Fatalf("❌ 解析配置文件失败: %v", err)
	}

	log.Println("✅ 配置文件加载成功")
	return &cfg
}
