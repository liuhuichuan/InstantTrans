#ifndef LOOPBACK_AUDIO_H_
#define LOOPBACK_AUDIO_H_

#include <windows.h>
#include <objbase.h>  // 补充：确保 COM 接口（如 IMMDevice）正常解析
#include <mmdeviceapi.h>  // WASAPI 核心接口头文件
#include <audioclient.h>  // 音频客户端接口
#include <functiondiscoverykeys_devpkey.h>  // 设备属性查询
#include <cstdint>

// 链接 WASAPI 所需的库（无需手动加库，通过 pragma 自动链接）
#pragma comment(lib, "ole32.lib")       // COM 组件初始化依赖
#pragma comment(lib, "oleaut32.lib")    // COM 智能指针依赖
#pragma comment(lib, "uuid.lib")        // GUID 相关依赖

namespace sherpa_onnx {

// 系统音频回环捕获类，结构与 Microphone 类对齐
class LoopbackAudio {
 public:
  // 构造函数：初始化 WASAPI 环境，查找默认音频输出设备的回环接口
  LoopbackAudio();

  // 析构函数：释放 WASAPI 资源，终止 COM 环境
  ~LoopbackAudio();

  // ------------------- 可选：对外暴露核心配置（按需使用）-------------------
  // 获取回环捕获的采样率（默认与系统输出设备一致，通常为 48000 Hz）
  int32_t GetSampleRate() const { return sample_rate_; }

  // 获取回环捕获的声道数（默认与系统输出设备一致，通常为 2 声道立体声）
  int32_t GetChannelCount() const { return channel_count_; }

  // 获取音频格式的位深度（固定 16 位 PCM，兼容 sherpa-onnx 处理）
  int32_t GetBitsPerSample() const { return 16; }

  // 获取 WASAPI 音频客户端接口（供后续捕获逻辑使用）
  IAudioClient *GetAudioClient() const { return audio_client_; }

 private:
  // ------------------- 核心成员变量 -------------------
  HMODULE com_lib_ = nullptr;  // COM 库句柄（确保动态加载）
  IMMDeviceEnumerator *device_enum_ = nullptr;  // 音频设备枚举器
  IMMDevice *loopback_device_ = nullptr;  // 回环设备（关联系统默认输出设备）
  IAudioClient *audio_client_ = nullptr;  // 音频客户端（核心捕获接口）
  WAVEFORMATEX *capture_format_ = nullptr;  // 回环捕获的音频格式
  int32_t sample_rate_ = 0;                 // 采样率（如 48000 Hz）
  int32_t channel_count_ = 0;               // 声道数（如 2 声道）

  // ------------------- 内部辅助函数 -------------------
  // 初始化 COM 环境（WASAPI 基于 COM 架构）
  void InitCOM();

  // 查找系统默认音频输出设备的回环接口
  void FindLoopbackDevice();

  // 配置回环捕获的音频格式（与系统输出格式对齐，确保兼容性）
  void ConfigureCaptureFormat();
};
}  // namespace sherpa_onnx
#endif