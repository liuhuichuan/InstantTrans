#pragma once
#include "types/types.h"
#include <windows.h>
#include <memory>

// UI 端自定义消息
constexpr UINT WM_APP_RECOG = WM_APP + 1;   // payload: RecognitionMessage*
constexpr UINT WM_APP_TRANS = WM_APP + 2;  // payload: TranslationMessage*

class MessageBus {
public:
    // UI HWND 需在 MainWindow 初始化后设置
    void SetUIWindow(HWND hwnd) { ui_hwnd_ = hwnd; }

    void PostRecognition(const RecognitionMessage& msg) {
        if (!ui_hwnd_) return;
        auto p = new RecognitionMessage(msg);
        ::PostMessage(ui_hwnd_, WM_APP_RECOG, reinterpret_cast<WPARAM>(p), 0);
    }

    void PostTranslation(const TranslationMessage& msg) {
        if (!ui_hwnd_) return;
        auto p = new TranslationMessage(msg);
        ::PostMessage(ui_hwnd_, WM_APP_TRANS, reinterpret_cast<WPARAM>(p), 0);
    }

private:
    HWND ui_hwnd_ = nullptr;
};
