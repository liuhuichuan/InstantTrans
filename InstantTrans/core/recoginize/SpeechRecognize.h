#pragma once
#include <queue>
#include <vector>

#include "core/ipc/MessageBus.h"
#include <thread>
#include <atomic>
#include <functional>

#include <Windows.h>
#include <objbase.h>
#include <mmdeviceapi.h>
#include <audioclient.h>
#include <functiondiscoverykeys_devpkey.h>
#include <cstdint>
#include "cxx-api.h"

class SpeechRecognizer {
public:
    SpeechRecognizer(MessageBus* bus);
    ~SpeechRecognizer();

    void Start();

    void Stop();

private:
    void RecognizeLoop();

    void CaptureLoop();

    sherpa_onnx::cxx::OfflineRecognizer CreateOfflineRecognizer();

    sherpa_onnx::cxx::VoiceActivityDetector CreateVad();

    std::vector<float> ResampleLinear(const float* in_samples, size_t in_len, int in_rate, int out_rate);

    std::thread capture_thread;
    std::thread recognize_thread;

    MessageBus* bus_;
   
    IAudioClient* audio_client = nullptr;
    bool stop = true;
    std::queue<std::vector<float>> samples_queue;

};
