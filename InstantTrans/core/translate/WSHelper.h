#pragma once

#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <curl/curl.h>

namespace WebSocketClient {
	inline std::string GenerateUUID() {
		GUID guid;
		CoCreateGuid(&guid);

		char buffer[64] = { 0 };
		snprintf(
			buffer,
			sizeof(buffer),
			"%08lX-%04hX-%04hX-%02hhX%02hhX-%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX",
			guid.Data1,
			guid.Data2,
			guid.Data3,
			guid.Data4[0], guid.Data4[1],
			guid.Data4[2], guid.Data4[3],
			guid.Data4[4], guid.Data4[5],
			guid.Data4[6], guid.Data4[7]
		);

		return std::string(buffer);
	}
	// 建立 WebSocket 连接
	CURL* WS_Connect(const std::string& url);

	// 发送 WebSocket 消息
	bool WS_Send(CURL* curl, const std::string& message, unsigned int flags = CURLWS_TEXT);

	// 接收 WebSocket 消息（阻塞等待）
	bool WS_Receive(CURL* curl, std::string& out_message);

	// 关闭 WebSocket 连接
	void WS_Close(CURL* curl);

	bool SendTranslateOnce(
		CURL* curl,
		const std::string& client_id,
		const std::string& lang_from,
		const std::string& lang_to,
		const std::string& source_text,
		std::string& out_request_id   // <<< 新增：输出 request_id
	);

	bool SendTranslateAndReceive(
		CURL* curl,
		const std::string& client_id,
		const std::string& lang_from,
		const std::string& lang_to,
		const std::string& source_text,
		std::string& out_result
	);
}
