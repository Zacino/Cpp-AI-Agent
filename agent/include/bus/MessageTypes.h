#pragma once

#include <string>
#include <cstdint>
#include <json/json.h>

namespace agent {

// InboundMessage - HTTP层 -> Agent
struct InboundMessage {
    std::string message_id;
    std::string session_id;
    std::string content;
    int64_t timestamp;
    Json::Value metadata;
};

// OutboundMessage - Agent -> HTTP层
struct OutboundMessage {
    std::string message_id;
    std::string session_id;
    std::string content;
    Json::Value tool_calls;
    bool success = true;
    std::string error_msg;
    Json::Value metadata;
};

} // namespace agent
