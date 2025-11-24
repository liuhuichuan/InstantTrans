#include "WSHelper.h"

#include <nlohmann/json.hpp>
namespace WebSocketClient {

    // 建立 WebSocket 连接
    CURL* WS_Connect(const std::string& url) {
        CURLcode res;
        CURL* curl = nullptr;

        curl_global_init(CURL_GLOBAL_DEFAULT);
        curl = curl_easy_init();
        if (!curl) {
            std::cerr << "[WS] curl_easy_init() failed\n";
            return nullptr;
        }

        // WebSocket 模式
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_CONNECT_ONLY, 2L); // WebSocket 模式
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, nullptr); // 不使用普通 HTTP 写回调

        res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            std::cerr << "[WS] Connection failed: " << curl_easy_strerror(res) << std::endl;
            curl_easy_cleanup(curl);
            curl_global_cleanup();
            return nullptr;
        }

        std::cout << "[WS] Connected to " << url << std::endl;
        return curl;
    }

    // 发送 WebSocket 消息
    bool WS_Send(CURL* curl, const std::string& message, unsigned int flags) {
        if (!curl) return false;
        size_t bytes_sent = 0;

        CURLcode res = curl_ws_send(curl, message.c_str(), message.size(), &bytes_sent, 0, flags);
        if (res != CURLE_OK) {
            std::cerr << "[WS] Send failed: " << curl_easy_strerror(res) << std::endl;
            return false;
        }

        std::cout << "[WS] Sent (" << bytes_sent << " bytes): " << message << std::endl;
        return true;
    }

    // 接收 WebSocket 消息（阻塞等待）
    bool WS_Receive(CURL* curl, std::string& out_message) {
        if (!curl) return false;

        char buffer[2048];
        size_t recv_len = 0;
        const struct curl_ws_frame* frame = nullptr;

        while (true) {
            CURLcode res = curl_ws_recv(curl, buffer, sizeof(buffer), &recv_len, &frame);
            if (res == CURLE_AGAIN) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                continue;
            }
            else if (res != CURLE_OK) {
                std::cerr << "[WS] Receive failed: " << curl_easy_strerror(res) << std::endl;
                return false;
            }

            out_message.assign(buffer, recv_len);
            std::cout << "[WS] Received: " << out_message << std::endl;
            return true;
        }
    }

    // 关闭 WebSocket 连接
    void WS_Close(CURL* curl) {
        if (!curl) return;

        // 按 RFC 6455 发送关闭帧
        size_t sent = 0;
        curl_ws_send(curl, "", 0, &sent, 0, CURLWS_CLOSE);

        curl_easy_cleanup(curl);
        curl_global_cleanup();
        std::cout << "[WS] Connection closed.\n";
    }

    bool SendTranslateOnce(
        CURL* curl,
        const std::string& client_id,
        const std::string& lang_from,
        const std::string& lang_to,
        const std::string& source_text,
        std::string& out_request_id
    ) {
        if (!curl) return false;

        out_request_id = GenerateUUID();

        nlohmann::json payload = {
            {"client_id", client_id},
            {"request_id", out_request_id},
            {"lang_from", lang_from},
            {"lang_to", lang_to},
            {"source_text", source_text}
        };

        std::string json_msg = payload.dump();

        size_t bytes_sent = 0;
        CURLcode rc = curl_ws_send(
            curl,
            json_msg.c_str(),
            json_msg.size(),
            &bytes_sent,
            0,
            CURLWS_TEXT
        );

        if (rc != CURLE_OK) {
            std::cerr << "[WS] Send failed: "
                << curl_easy_strerror(rc) << std::endl;
            return false;
        }

        return true;
    }

    bool SendTranslateAndReceive(
        CURL* curl,
        const std::string& client_id,
        const std::string& lang_from,
        const std::string& lang_to,
        const std::string& source_text,
        std::string& out_result
    ) {
        if (!curl) return false;

        std::string request_id;
        if (!SendTranslateOnce(curl, client_id, lang_from, lang_to, source_text, request_id)) {
            return false;
        }

        char buffer[4096];
        size_t recv_len = 0;
        const curl_ws_frame* frame = nullptr;

        while (true) {
            CURLcode rc = curl_ws_recv(curl, buffer, sizeof(buffer), &recv_len, &frame);

            if (rc == CURLE_AGAIN) {
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
                continue;
            }

            if (rc != CURLE_OK) {
                std::cerr << "[WS] Receive failed: "
                    << curl_easy_strerror(rc) << std::endl;
                return false;
            }

            std::string json_msg(buffer, recv_len);

            try {
                auto resp = nlohmann::json::parse(json_msg);

                if (resp.contains("request_id") &&
                    resp["request_id"].get<std::string>() == request_id) {

                    if (resp.contains("result")) {
                        out_result = resp["result"].get<std::string>();
                        return true;
                    }
                }

            }
            catch (...) {
                std::cerr << "[WS] JSON parse error: " << json_msg << std::endl;
            }
        }
    }

} // namespace WebSocketClient