#pragma once

#include <string>
#include <vector>
#include <optional>
#include <json/json.h>

namespace agent {

struct ToolCall {
    std::string id;
    std::string name;
    Json::Value arguments;
};

struct LLMMessage {
    std::string role;       // "system", "user", "assistant", "tool"
    std::string content;
    std::vector<ToolCall> tool_calls;  // assistant 发起的 tool calls
    std::string tool_call_id;          // tool 角色回复时需要
};

struct LLMRequest {
    std::string model = "gpt-4o-mini";
    std::vector<LLMMessage> messages;
    Json::Value tools;      // 可用工具 schema
    int max_tokens = 4096;
    float temperature = 0.7f;
};

struct LLMResponse {
    bool success = true;
    std::string content;
    std::vector<ToolCall> tool_calls;
    std::string finish_reason;
    std::string error_msg;
};

} // namespace agent
