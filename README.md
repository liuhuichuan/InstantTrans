# InstantTrans

## 项目简介

InstantTrans 是一个实时语音翻译应用程序，能够将用户的语音输入实时转换为目标语言并显示翻译结果。该项目采用前后端分离架构，前端基于 C++/DuiLib 开发桌面界面，后端使用 Go 语言提供翻译服务支持。

## 功能特性

- **实时语音识别**：通过语音识别技术将用户语音转换为文本
- **即时翻译**：将识别出的文本实时翻译为目标语言（默认英文→中文）
- **用户友好界面**：简洁直观的桌面应用界面
- **WebSocket 通信**：前后端通过 WebSocket 实现实时数据传输
- **模块化设计**：清晰的代码结构和功能模块化

## 系统架构

### 前端（C++）
- **用户界面层**：基于 DuiLib 的桌面应用界面
- **业务逻辑层**：语音识别处理和翻译流程控制
- **通信层**：WebSocket 客户端实现与后端通信

### 后端（Go）
- **WebSocket 服务**：处理前端连接和消息转发
- **任务队列**：使用 NATS 进行消息队列管理
- **缓存服务**：使用 Redis 缓存翻译结果
- **翻译工作器**：处理实际的翻译任务

## 技术栈

### 前端
- C++
- DuiLib（UI 框架）
- Win32 API
- WebSocket 客户端

### 后端
- Go 语言
- NATS（消息队列）
- Redis（缓存）
- WebSocket 服务器

## 安装指南

### 前置要求

#### 前端
- Visual Studio 2019 或更高版本
- Windows SDK
- DuiLib 库

#### 后端
- Go 1.16 或更高版本
- Redis 服务器
- NATS 服务器

### 构建步骤

#### 前端构建
1. 使用 Visual Studio 打开 `InstantTrans\InstantTrans.sln` 解决方案
2. 配置项目属性和依赖项
3. 构建解决方案（Debug/Release 模式）

#### 后端构建
1. 进入后端目录：`cd backend\translate-gateway`
2. 安装依赖：`go mod tidy`
3. 构建项目：`go build -o translate-gateway.exe main.go`

## 使用说明

### 启动服务

1. 启动 Redis 服务器
2. 启动 NATS 服务器
3. 运行后端服务：`translate-gateway.exe`
4. 运行前端应用程序：`InstantTrans.exe`

### 操作步骤

1. 点击界面上的「开始」按钮启动语音识别
2. 开始讲话，系统会实时识别并翻译
3. 翻译结果会显示在界面上
4. 点击「停止」按钮结束语音识别

## 项目结构

```
InstantTrans/
├── InstantTrans/           # 前端 C++ 项目
│   ├── InstantTrans.cpp    # 应用程序入口
│   ├── MainThread.cpp      # 主线程实现
│   ├── ui/                 # UI 相关代码
│   │   ├── MainForm.cpp    # 主窗口实现
│   │   └── MainForm.h      # 主窗口头文件
│   ├── core/               # 核心功能模块
│   │   ├── ipc/            # 进程间通信
│   │   ├── recognize/      # 语音识别
│   │   └── translate/      # 翻译相关
│   └── bin/                # 可执行文件和资源
└── backend/                # 后端 Go 项目
    └── translate-gateway/  # 翻译网关服务
        ├── main.go         # 后端入口
        ├── internal/       # 内部实现
        │   ├── ws/         # WebSocket 服务
        │   ├── nats/       # NATS 客户端
        │   └── cache/      # 缓存服务
        └── config/         # 配置文件
```

## 配置说明

### 后端配置

后端服务配置文件位于 `backend/translate-gateway/config/config.yaml`，主要配置项包括：
- Redis 服务器地址和端口
- NATS 服务器地址
- WebSocket 服务端口

### 前端配置

前端 WebSocket 连接地址在 `MainForm.cpp` 中定义，可以根据需要修改：
```cpp
ws = WebSocketClient::WS_Connect("ws://127.0.0.1:8080/ws");
```

## 贡献指南

欢迎对项目进行贡献！请按照以下步骤：

1. Fork 本仓库
2. 创建您的特性分支：`git checkout -b feature/amazing-feature`
3. 提交您的更改：`git commit -m 'Add some amazing feature'`
4. 推送到分支：`git push origin feature/amazing-feature`
5. 提交 Pull Request

## 许可证

本项目使用 MIT 许可证 - 详情请查看 LICENSE 文件

## 联系方式

如有问题或建议，请通过以下方式联系我们：

- 提交 Issue：在项目仓库中创建新的 Issue
- 贡献代码：按照贡献指南提交 Pull Request