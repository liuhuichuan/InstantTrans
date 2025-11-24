package types

type TranslateRequest struct {
	RequestID  string `json:"request_id"`
	ClientID   string `json:"client_id"`
	SourceText string `json:"source_text"`
	LangFrom   string `json:"lang_from"`
	LangTo     string `json:"lang_to"`
}

type TranslateResponse struct {
	ClientID  string `json:"client_id"`
	RequestID string `json:"request_id"`
	LangFrom  string `json:"lang_from"`
	LangTo    string `json:"lang_to"`
	Result    string `json:"result"`
}
