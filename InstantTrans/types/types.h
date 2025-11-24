#pragma once
#include <string>
#include <chrono>

struct RecognitionMessage {
    std::string recog_text;   // 识别到的文本（可能是片段）
    bool is_final = false;    // 是否识别出“句尾”/完整句子
    std::chrono::steady_clock::time_point ts = std::chrono::steady_clock::now();
};

struct TranslationMessage {
    std::string recog_text;   // 对应的完整识别文本
    std::string trans_text;   // 翻译结果
};
