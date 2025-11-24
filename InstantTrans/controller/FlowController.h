#pragma once
#include "types/types.h"
#include <string>
#include <mutex>
#include <functional>
#include <chrono>

struct DisplaySlot {
    std::string recog;
    std::string trans;
    bool trans_ready = false;
    std::chrono::steady_clock::time_point last_update;
};

class FlowController {
public:
    // 回调：当有显示需要 UI 更新时调用（会在 UI 线程调用）
    using UpdateCallback = std::function<void(const DisplaySlot&, const DisplaySlot&)>;

    FlowController() {
        slots_[0].last_update = slots_[1].last_update = std::chrono::steady_clock::now();
    }

    // 收到中间识别更新（可能是不断变化）
    void OnRecognitionFragment(const std::string& text) {
        std::lock_guard<std::mutex> lk(mutex_);
        // 当前第二槽用于预备，active_index_ 为第一组（上方）
        slots_[next_index_].recog = text;
        slots_[next_index_].last_update = std::chrono::steady_clock::now();
        // 告知 UI 刷新（非最终翻译）
        if (cb_) cb_(slots_[active_index_], slots_[next_index_]);
    }

    // 收到翻译结果（对应某个完整识别文本）
    void OnTranslationReady(const std::string& recog, const std::string& trans) {
        std::lock_guard<std::mutex> lk(mutex_);
        // 将翻译放入 next_index_（第二组）
        slots_[next_index_].recog = recog;
        slots_[next_index_].trans = trans;
        slots_[next_index_].trans_ready = true;
        slots_[next_index_].last_update = std::chrono::steady_clock::now();

        // 如果 active slot 没有翻译或已超时，则触发滚动（将 next -> active）
        MaybeAdvance();
        if (cb_) cb_(slots_[active_index_], slots_[next_index_]);
    }


    void SetUpdateCallback(UpdateCallback cb) { cb_ = std::move(cb); }

    // 外部也可以主动触发超时检测（例如一个定时器，每秒调用一次）
    void Tick() {
        std::lock_guard<std::mutex> lk(mutex_);
        // 如果 active 的翻译完成或超时，且 next 有翻译就 advance
        MaybeAdvance();
        if (cb_) cb_(slots_[active_index_], slots_[next_index_]);
    }

    // 取当前显示（UI 用）
    void GetSlots(DisplaySlot& outActive, DisplaySlot& outNext) {
        std::lock_guard<std::mutex> lk(mutex_);
        outActive = slots_[active_index_];
        outNext = slots_[next_index_];
    }

private:
    void MaybeAdvance() {
        auto now = std::chrono::steady_clock::now();
        const auto TIMEOUT = std::chrono::seconds(10); // 可配置：翻译显示超时
        // 条件：active 已完成翻译 OR active 超时 (基于 last_update)
        bool active_done = slots_[active_index_].trans_ready;
        bool active_timeout = (now - slots_[active_index_].last_update) > TIMEOUT;

        if ((active_done || active_timeout) && (slots_[next_index_].trans_ready || !slots_[next_index_].recog.empty())) {
            // 把 next 向上滚
            active_index_ = next_index_;
            next_index_ = 1 - active_index_;
            // 清空新的 next
            slots_[next_index_] = DisplaySlot();
            slots_[next_index_].last_update = std::chrono::steady_clock::now();
        }
    }

    DisplaySlot slots_[2];
    int active_index_ = 0; // currently displayed (top slot)
    int next_index_ = 1;   // upcoming slot (bottom)
    std::mutex mutex_;
    UpdateCallback cb_;
};

