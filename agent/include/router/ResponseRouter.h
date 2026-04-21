#pragma once

#include <unordered_map>
#include <future>
#include <mutex>
#include <string>
#include "bus/MessageTypes.h"

namespace agent {

// ResponseRouter - 同步 HTTP 与异步核心的桥梁
class ResponseRouter {
public:
    static ResponseRouter& Instance();

    // 注册等待响应，返回 future
    std::future<OutboundMessage> Register(const std::string& message_id);

    // 完成响应（由 OutboundBus 消费者调用）
    void Fulfill(const OutboundMessage& msg);

    // 失败响应
    void Fail(const std::string& message_id, const std::string& error);

    // 服务停止时将所有挂起请求全部失败，避免 HTTP 线程永久阻塞
    void FailAll(const std::string& error);

private:
    ResponseRouter() = default;

    std::unordered_map<std::string, std::promise<OutboundMessage>> pending_;
    std::mutex mtx_;
};

} // namespace agent
