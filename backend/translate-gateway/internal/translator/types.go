package translator

type TranslateRequest struct {
	ID       string `json:"id"`   // 消息ID，用于回推
	Text     string `json:"text"` // 要翻译的文本
	FromLang string `json:"from"` // 源语言
	ToLang   string `json:"to"`   // 目标语言
}

type TranslateResult struct {
	ID      string `json:"id"`
	Text    string `json:"text"` // 翻译后的文本
	Success bool   `json:"success"`
	Error   string `json:"error,omitempty"`
}
