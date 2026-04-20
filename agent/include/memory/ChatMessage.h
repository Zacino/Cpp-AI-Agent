#pragma once

#include <string>
#include <cstdint>
#include <json/json.h>

namespace agent {

// 聊天消息结构
struct ChatMessage {
    std::string role;       // "user" | "assistant" | "tool" | "system"
    std::string content;
    int64_t timestamp;
    Json::Value metadata;
};

} // namespace agent
