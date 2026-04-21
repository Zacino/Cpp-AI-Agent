#include "agent/include/router/ResponseRouter.h"

namespace agent {

ResponseRouter& ResponseRouter::Instance() {
    static ResponseRouter instance;
    return instance;
}

std::future<OutboundMessage> ResponseRouter::Register(const std::string& message_id) {
    std::unique_lock<std::mutex> lock(mtx_);
    // 防止重复注册同一 message_id
    if (pending_.count(message_id)) {
        // 已存在则返回一个已设置错误的 future
        std::promise<OutboundMessage> p;
        OutboundMessage err;
        err.message_id = message_id;
        err.success = false;
        err.error_msg = "Duplicate message_id: " + message_id;
        p.set_value(std::move(err));
        return p.get_future();
    }
    auto [it, inserted] = pending_.try_emplace(message_id);
    return it->second.get_future();
}

void ResponseRouter::Fulfill(const OutboundMessage& msg) {
    std::unique_lock<std::mutex> lock(mtx_);
    auto it = pending_.find(msg.message_id);
    if (it != pending_.end()) {
        it->second.set_value(msg);
        pending_.erase(it);
    }
}

void ResponseRouter::Fail(const std::string& message_id, const std::string& error) {
    std::unique_lock<std::mutex> lock(mtx_);
    auto it = pending_.find(message_id);
    if (it != pending_.end()) {
        OutboundMessage msg;
        msg.message_id = message_id;
        msg.success = false;
        msg.error_msg = error;
        it->second.set_value(msg);
        pending_.erase(it);
    }
}

void ResponseRouter::FailAll(const std::string& error) {
    std::unique_lock<std::mutex> lock(mtx_);
    for (auto& [id, promise] : pending_) {
        OutboundMessage msg;
        msg.message_id = id;
        msg.success = false;
        msg.error_msg = error;
        promise.set_value(std::move(msg));
    }
    pending_.clear();
}

} // namespace agent
