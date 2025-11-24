#define NOMINMAX
#include "SpeechRecognize.h"

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

#include <chrono>
#include <condition_variable>
#include <iostream>
#include <mutex>

#include "sherpa-display.h"

#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "oleaut32.lib")
#pragma comment(lib, "uuid.lib")

std::mutex mutex;
std::condition_variable condition_variable;

SpeechRecognizer::SpeechRecognizer(MessageBus* bus) :bus_(bus)
{
	
}

SpeechRecognizer::~SpeechRecognizer()
{
	Stop();
}

void SpeechRecognizer::Start()
{
   
    // 启动录音线程
    stop = false;
    capture_thread = std::thread(&SpeechRecognizer::CaptureLoop, this);

    //std::cout << "Started! Please speak (Ctrl+C 停止)\n";

    recognize_thread = std::thread(&SpeechRecognizer::RecognizeLoop, this);

}


void SpeechRecognizer::Stop()
{
    if (!stop)
    {
        stop = true;

        if (audio_client)
            audio_client->Stop();  // 重要！

        condition_variable.notify_all();

        if (capture_thread.joinable())
            capture_thread.join();
        if (recognize_thread.joinable())
            recognize_thread.join();

        
    }
}

// -------------------- WASAPI 录音线程 --------------------
void SpeechRecognizer::CaptureLoop() {
    OutputDebugStringW(L"[CaptureLoop] thread started\n");

    // 确保成员指针初始化 (在构造函数中也最好设置)
    IMMDeviceEnumerator* device_enum = nullptr;
    IMMDevice* loopback_device = nullptr;

    HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    {
        wchar_t buf[256];
        swprintf(buf, 256, L"[CaptureLoop] CoInitializeEx hr=0x%08X\n", hr);
        OutputDebugStringW(buf);
    }
    if (FAILED(hr)) {
        // 如果返回 RPC_E_CHANGED_MODE，记录并尝试继续/退出
        OutputDebugStringW(L"[CaptureLoop] CoInitializeEx FAILED, abort CaptureLoop\n");
        return;
    }
    
    try {
  
        hr = CoCreateInstance(
            __uuidof(MMDeviceEnumerator),
            nullptr,
            CLSCTX_ALL,
            __uuidof(IMMDeviceEnumerator),
            reinterpret_cast<void**>(&device_enum)
        );

        {
            wchar_t buf[256];
            swprintf(buf, 256, L"[CaptureLoop] CoCreateInstance hr=0x%08X\n", hr);
            OutputDebugStringW(buf);
        }

        if (FAILED(hr) || device_enum == nullptr) {
            OutputDebugStringW(L"[CaptureLoop] CoCreateInstance failed or returned null\n");
            CoUninitialize();
            return;
        }
    }
    catch (...) {
        OutputDebugStringW(L"[CaptureLoop] unknown exception\n");
        if (loopback_device) { loopback_device->Release(); loopback_device = nullptr; }
        if (device_enum) { device_enum->Release(); device_enum = nullptr; }
        CoUninitialize();
        return;
    }

    hr = device_enum->GetDefaultAudioEndpoint(eRender, eConsole, &loopback_device);
    if (FAILED(hr)) throw std::runtime_error("GetDefaultAudioEndpoint failed");

    IPropertyStore* prop_store = nullptr;
    hr = loopback_device->OpenPropertyStore(STGM_READ, &prop_store);
    if (SUCCEEDED(hr)) {
        PROPVARIANT prop_var;
        PropVariantInit(&prop_var);
        if (SUCCEEDED(prop_store->GetValue(PKEY_Device_FriendlyName, &prop_var))) {
            std::wcout << L"使用设备: " << prop_var.pwszVal << std::endl;
        }
        PropVariantClear(&prop_var);
        prop_store->Release();
    }

    audio_client = nullptr;
    hr = loopback_device->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr,
        reinterpret_cast<void**>(&audio_client));
    if (FAILED(hr)) throw std::runtime_error("Activate(IAudioClient) failed");

    WAVEFORMATEX* mix_format = nullptr;
    hr = audio_client->GetMixFormat(&mix_format);
    if (FAILED(hr)) throw std::runtime_error("GetMixFormat failed");

    bool is_float = false;
    if (mix_format->wFormatTag == WAVE_FORMAT_IEEE_FLOAT) is_float = true;
    else if (mix_format->wFormatTag == WAVE_FORMAT_EXTENSIBLE) {
        WAVEFORMATEXTENSIBLE* wfex = reinterpret_cast<WAVEFORMATEXTENSIBLE*>(mix_format);
        if (wfex->SubFormat == KSDATAFORMAT_SUBTYPE_IEEE_FLOAT) is_float = true;
    }

    hr = audio_client->Initialize(
        AUDCLNT_SHAREMODE_SHARED,
        AUDCLNT_STREAMFLAGS_LOOPBACK,
        0, 0,
        mix_format, nullptr
    );
    if (FAILED(hr)) throw std::runtime_error("AudioClient Initialize failed");

    IAudioCaptureClient* capture_client = nullptr;
    hr = audio_client->GetService(__uuidof(IAudioCaptureClient),
        reinterpret_cast<void**>(&capture_client));
    if (FAILED(hr)) throw std::runtime_error("GetService(IAudioCaptureClient) failed");

    hr = audio_client->Start();
    if (FAILED(hr)) throw std::runtime_error("AudioClient Start failed");

    while (!stop) {
        UINT32 num_frames = 0;
        BYTE* data = nullptr;
        DWORD flags = 0;
        hr = capture_client->GetBuffer(&data, &num_frames, &flags, nullptr, nullptr);
        if (SUCCEEDED(hr) && num_frames > 0) {
            size_t in_samples = num_frames * mix_format->nChannels;
            std::vector<float> float_buffer(in_samples, 0.0f);

            if (!(flags & AUDCLNT_BUFFERFLAGS_SILENT)) {
                if (is_float) {
                    float* f = reinterpret_cast<float*>(data);
                    for (size_t i = 0; i < in_samples; ++i) float_buffer[i] = f[i];
                }
                else { // PCM16 -> float
                    short* s = reinterpret_cast<short*>(data);
                    for (size_t i = 0; i < in_samples; ++i) float_buffer[i] = s[i] / 32768.0f;
                }
            }

            // 双声道平均 -> 单声道
            std::vector<float> mono_buffer(num_frames, 0.0f);
            for (UINT32 i = 0; i < num_frames; ++i) {
                float sum = 0.0f;
                for (int ch = 0; ch < mix_format->nChannels; ++ch) {
                    sum += float_buffer[i * mix_format->nChannels + ch];
                }
                mono_buffer[i] = sum / mix_format->nChannels;
            }

            // 下采样到 16kHz
            auto resampled = ResampleLinear(mono_buffer.data(), mono_buffer.size(),
                mix_format->nSamplesPerSec, 16000);

            {
                std::lock_guard<std::mutex> lock(mutex);
                samples_queue.emplace(std::move(resampled));
            }
            condition_variable.notify_one();

            capture_client->ReleaseBuffer(num_frames);
        }
        //Sleep(10);
    }

    audio_client->Stop();
    if (capture_client) capture_client->Release();
    if (audio_client) audio_client->Release();
    CoTaskMemFree(mix_format);
}

// -------------------- 简单线性下采样 --------------------
std::vector<float> SpeechRecognizer::ResampleLinear(const float* in_samples, size_t in_len, int in_rate, int out_rate) {
    if (in_rate == out_rate) return std::vector<float>(in_samples, in_samples + in_len);

    size_t out_len = static_cast<size_t>(in_len * out_rate / in_rate);
    std::vector<float> out(out_len);

    double ratio = static_cast<double>(in_len - 1) / (out_len - 1);
    for (size_t i = 0; i < out_len; ++i) {
        double idx = i * ratio;
        size_t idx_int = static_cast<size_t>(idx);
        double frac = idx - idx_int;
        float s1 = in_samples[idx_int];
        float s2 = (idx_int + 1 < in_len) ? in_samples[idx_int + 1] : s1;
        out[i] = static_cast<float>(s1 + frac * (s2 - s1));
    }
    return out;
}

void SpeechRecognizer::RecognizeLoop()
{
    using namespace sherpa_onnx::cxx;

    auto vad = CreateVad();
    auto recognizer = CreateOfflineRecognizer();

    float sample_rate = 16000;
    int32_t window_size = 512; // samples

    int32_t offset = 0;
    std::vector<float> buffer;
    bool speech_started = false;
    auto started_time = std::chrono::steady_clock::now();
    //SherpaDisplay display;

    while (!stop) {
        {
            std::unique_lock<std::mutex> lock(mutex);
            while (samples_queue.empty() && !stop) {
                condition_variable.wait(lock);
            }

            const auto& s = samples_queue.front();
            buffer.insert(buffer.end(), s.begin(), s.end());
            samples_queue.pop();
        }

        // VAD
        for (; offset + window_size < buffer.size(); offset += window_size) {
            vad.AcceptWaveform(buffer.data() + offset, window_size);
            if (!speech_started && vad.IsDetected()) {
                speech_started = true;
                started_time = std::chrono::steady_clock::now();
            }
        }

        if (!speech_started) {
            if (buffer.size() > 10 * window_size) {
                offset -= buffer.size() - 10 * window_size;
                buffer = { buffer.end() - 10 * window_size, buffer.end() };
            }
        }

        auto current_time = std::chrono::steady_clock::now();
        const float elapsed_seconds =
            std::chrono::duration_cast<std::chrono::milliseconds>(current_time -
                started_time)
            .count() /
            1000.;

        if (speech_started && elapsed_seconds > 0.2) {
            OfflineStream stream = recognizer.CreateStream();
            stream.AcceptWaveform(sample_rate, buffer.data(), buffer.size());
            recognizer.Decode(&stream);

            OfflineRecognizerResult result = recognizer.GetResult(&stream);
            
            RecognitionMessage msg;
            msg.recog_text = result.text;
            msg.is_final = false;
            bus_->PostRecognition(msg);

            started_time = std::chrono::steady_clock::now();
        }

        while (!vad.IsEmpty()) {
            auto segment = vad.Front();
            vad.Pop();

            OfflineStream stream = recognizer.CreateStream();
            stream.AcceptWaveform(sample_rate, segment.samples.data(),
                segment.samples.size());
            recognizer.Decode(&stream);

            OfflineRecognizerResult result = recognizer.GetResult(&stream);
            
            RecognitionMessage msg;
            msg.recog_text = result.text;
            msg.is_final = true;
            bus_->PostRecognition(msg);

            // send to queue
            //{
            //    std::lock_guard<std::mutex> lock(recognized_mutex);
            //    recognized_text_queue.push(result.text);
            //}
            //recognized_cv.notify_one();

            //display.Display();

            buffer.clear();
            offset = 0;
            speech_started = false;
        }
    }
}

std::wstring GetExeDirectory()
{
    TCHAR exePath[MAX_PATH] = { 0 };
    DWORD size = GetModuleFileName(nullptr, exePath, MAX_PATH);
    if (size == 0) {
        return L"";
    }

    std::wstring pathStr(exePath);

    // 找最后一个反斜杠，去掉文件名
    size_t pos = pathStr.find_last_of(L"\\/");
    if (pos != std::wstring::npos) {
        pathStr = pathStr.substr(0, pos);
    }

    return pathStr;
}

std::string WStringToUtf8(const std::wstring& wstr)
{
    if (wstr.empty()) return std::string();

    // 第一次调用计算所需缓冲区大小
    int size_needed = WideCharToMultiByte(
        CP_UTF8,                // 转为 UTF-8
        0,                      // 默认转换标志
        wstr.c_str(),           // 输入的 UTF-16 字符串
        (int)wstr.size(),       // 输入长度
        nullptr,                // 不需要输出缓冲区
        0,
        nullptr,
        nullptr
    );

    std::string result(size_needed, 0);

    // 第二次调用进行实际转换
    WideCharToMultiByte(
        CP_UTF8,
        0,
        wstr.c_str(),
        (int)wstr.size(),
        &result[0],
        size_needed,
        nullptr,
        nullptr
    );

    return result;
}

// -------------------- Sherpa-ONNX VAD & Recognizer --------------------
sherpa_onnx::cxx::VoiceActivityDetector SpeechRecognizer::CreateVad() {
    using namespace sherpa_onnx::cxx;

    std::wstring parentPath = GetExeDirectory();
    std::wstring onnxPath = parentPath + L"\\silero_vad.onnx";

    VadModelConfig config;
    config.silero_vad.model = WStringToUtf8(onnxPath);
    config.silero_vad.threshold = 0.7;
    config.silero_vad.min_silence_duration = 0.15;
    config.silero_vad.min_speech_duration = 0.25;
    config.silero_vad.max_speech_duration = 8;
    config.sample_rate = 16000;
    config.debug = false;

    VoiceActivityDetector vad = VoiceActivityDetector::Create(config, 20);
    if (!vad.Get()) {
        std::cerr << "Failed to create VAD. Please check your config\n";
        exit(-1);
    }
    return vad;
}

sherpa_onnx::cxx::OfflineRecognizer SpeechRecognizer::CreateOfflineRecognizer() {
    using namespace sherpa_onnx::cxx;

    std::wstring parentPath = GetExeDirectory();
    std::wstring modelPath = parentPath + L"\\sherpa-onnx-sense-voice-zh-en-ja-ko-yue-2024-07-17\\model.onnx";
    std::wstring tokensPath = parentPath + L"\\sherpa-onnx-sense-voice-zh-en-ja-ko-yue-2024-07-17\\tokens.txt";
    OfflineRecognizerConfig config;
    config.model_config.sense_voice.model =
        WStringToUtf8(modelPath);
    config.model_config.sense_voice.use_itn = true;
    config.model_config.sense_voice.language = "ja";
    config.model_config.tokens =
       WStringToUtf8(tokensPath);
    config.model_config.num_threads = 2;
    config.model_config.debug = false;

    std::cout << "Loading model\n";
    OfflineRecognizer recognizer = OfflineRecognizer::Create(config);
    if (!recognizer.Get()) {
        std::cerr << "Please check your config\n";
        exit(-1);
    }
    std::cout << "Loading model done\n";
    return recognizer;
}