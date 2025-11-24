package main

import (
	"context"
	"fmt"
	"log"

	"translategateway/internal/translator"
)

func main() {
	cfg, err := translator.LoadConfig("../../config/config.yaml")
	if err != nil {
		log.Fatal(err)
	}

	client := translator.NewClient(cfg)
	result, err := client.Translate(context.Background(), &translator.TranslateRequest{
		ID:       "demo-1",
		FromLang: "ja",
		ToLang:   "zh",
		Text:     "私は本を読むことが好きです",
	})
	if err != nil {
		log.Fatal(err)
	}

	fmt.Println("Translation result:", result.Text)
}
