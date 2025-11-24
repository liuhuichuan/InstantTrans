// loopback_audio.cc
// 基于 Windows WASAPI 实现系统音频回环捕获的核心逻辑

#include <stdio.h>
#include <stdlib.h>

#include "loopback-device.h"

namespace sherpa_onnx {

// ------------------- 构造函数：初始化回环捕获环境 -------------------
LoopbackAudio::LoopbackAudio() {
  try {
    // 1. 初始化 COM 环境（WASAPI 必须依赖 COM 组件）
    InitCOM();

    // 2. 查找系统默认输出设备的回环接口
    FindLoopbackDevice();

    // 3. 配置回环捕获的音频格式（采样率、声道数等）
    ConfigureCaptureFormat();

    printf("LoopbackAudio initialized successfully!\n");
    printf("  Sample Rate: %d Hz\n", sample_rate_);
    printf("  Channel Count: %d\n", channel_count_);
  } catch (const char *err_msg) {
    // 错误处理：与 Microphone 类一致，打印错误并退出
    fprintf(stderr, "LoopbackAudio init failed: %s\n", err_msg);
    exit(-1);
  }
}

// ------------------- 析构函数：释放资源 -------------------
LoopbackAudio::~LoopbackAudio() {
  // 1. 释放音频格式结构体（WASAPI 分配的内存需手动释放）
  if (capture_format_) {
    CoTaskMemFree(capture_format_);
    capture_format_ = nullptr;
  }

  // 2. 释放 WASAPI 接口（COM 接口需调用 Release() 减少引用计数）
  if (audio_client_) {
    audio_client_->Release();
    audio_client_ = nullptr;
  }
  if (loopback_device_) {
    loopback_device_->Release();
    loopback_device_ = nullptr;
  }
  if (device_enum_) {
    device_enum_->Release();
    device_enum_ = nullptr;
  }

  // 3. 终止 COM 环境（与 InitCOM 对应）
  CoUninitialize();

  printf("LoopbackAudio destroyed successfully!\n");
}

// ------------------- 内部辅助：初始化 COM 环境 -------------------
void LoopbackAudio::InitCOM() {
  // 初始化 COM 环境（STA 线程模型，适合单线程捕获场景）
  HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
  if (FAILED(hr)) {
    if (hr == RPC_E_CHANGED_MODE) {
      // 兼容已有 COM 环境（若线程已初始化 COM，直接复用）
      printf("COM already initialized in current thread\n");
    } else {
      throw "Failed to initialize COM (CoInitializeEx)";
    }
  }
}

// ------------------- 内部辅助：查找回环设备 -------------------
void sherpa_onnx::LoopbackAudio::FindLoopbackDevice() {
  HRESULT hr;

  // 1. 创建音频设备枚举器（原逻辑不变）
  hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL,
                        __uuidof(IMMDeviceEnumerator),
                        reinterpret_cast<void **>(&device_enum_));
  if (FAILED(hr)) {
    throw "Failed to create MMDeviceEnumerator (CoCreateInstance)";
  }

  // 2. 获取系统默认输出设备（原逻辑不变）
  hr = device_enum_->GetDefaultAudioEndpoint(eRender,   // 类型：输出设备
                                             eConsole,  // 角色：控制台
                                             &loopback_device_);
  if (FAILED(hr)) {
    throw "Failed to get default render device (GetDefaultAudioEndpoint)";
  }

  // ------------------- 关键修改：通过 IPropertyStore 查询属性
  // -------------------
  IPropertyStore *prop_store = nullptr;  // 设备属性存储接口
  // 打开设备的属性存储（获取 IPropertyStore 实例）
  hr = loopback_device_->OpenPropertyStore(
      STGM_READ,  // 只读模式（查询属性无需写入）
      &prop_store);
  if (FAILED(hr)) {
    throw "Failed to open property store (OpenPropertyStore)";
  }

  // 定义要查询的属性键（设备接口属性）
  PROPVARIANT prop_var;
  PropVariantInit(&prop_var);  // 初始化属性值结构体

  LPWSTR deviceId = nullptr;
  hr = loopback_device_->GetId(&deviceId);
  if (FAILED(hr)) {
    throw "Failed to get device ID (IMMDevice::GetId)";
  }

  // 清理属性相关资源（必须调用，避免内存泄漏）
  PropVariantClear(&prop_var);
  prop_store->Release();  // 释放 IPropertyStore 接口
  // -----------------------------------------------------------------------------

  // 3. 激活音频客户端（原逻辑不变）
  hr = loopback_device_->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr,
                                  reinterpret_cast<void **>(&audio_client_));
  if (FAILED(hr)) {
    throw "Failed to activate IAudioClient (Activate)";
  }
}

// ------------------- 内部辅助：配置回环捕获格式 -------------------
void LoopbackAudio::ConfigureCaptureFormat() {
  HRESULT hr;

  // 1. 获取设备支持的“默认输出格式”（回环需与输出格式一致）
  // 注意：WAVEFORMATEX 是通用音频格式结构体，CoTaskMemFree 需手动释放
  hr = audio_client_->GetMixFormat(&capture_format_);
  if (FAILED(hr)) {
    throw "Failed to get mix format (GetMixFormat)";
  }

  // 2. 强制配置为 16 位 PCM 格式（sherpa-onnx 常用格式，兼容多数模型）
  // 原格式可能是 32 位浮点/24 位 PCM，此处转为 16 位确保兼容性
  //capture_format_->wBitsPerSample = 16;  // 位深度：16 位
  //capture_format_->wFormatTag = WAVE_FORMAT_PCM;  // 格式类型：PCM 线性音频

  capture_format_->nBlockAlign =
      (capture_format_->nChannels * capture_format_->wBitsPerSample) /
      8;  // 块对齐（每帧字节数）
  capture_format_->nAvgBytesPerSec =
      capture_format_->nSamplesPerSec *
      capture_format_->nBlockAlign;               // 每秒字节数
  

  // 3. 保存核心参数（供外部调用）
  sample_rate_ = capture_format_->nSamplesPerSec;  // 采样率（如 48000 Hz）
  channel_count_ = capture_format_->nChannels;  // 声道数（如 2 声道）

  // 4. 初始化音频客户端（准备回环捕获）
  // 注意：AUDCLNT_SHAREMODE_SHARED 表示共享模式（与系统音频共享设备）
  // 10000000 表示 100ms 缓冲区（平衡延迟与稳定性）
  hr = audio_client_->Initialize(
      AUDCLNT_SHAREMODE_SHARED,
      AUDCLNT_STREAMFLAGS_LOOPBACK,  // 关键：启用回环模式
      10000000,  // 缓冲区时长（100ms，单位：100ns）
      0,         // 周期（与缓冲区时长一致，避免漂移）
      capture_format_,
      nullptr  // 会话 GUID（nullptr 表示默认会话）
  );
  if (FAILED(hr)) {
    throw "Failed to initialize IAudioClient (Initialize)";
  }
}

}  // namespace sherpa_onnx