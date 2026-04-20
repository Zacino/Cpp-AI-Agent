#include "agent/include/tool/MemoryTool.h"
#include "agent/include/memory/GlobalMemory.h"
#include <format>

namespace agent {

Json::Value MemoryTool::Schema() const {
    Json::Value schema;
    schema["type"] = "object";
    schema["properties"]["action"]["type"] = "string";
    schema["properties"]["action"]["description"] = "Action to perform: 'add_preference', 'add_knowledge', 'list', or 'query'";
    schema["properties"]["action"]["enum"] = Json::Value(Json::arrayValue);
    schema["properties"]["action"]["enum"].append("add_preference");
    schema["properties"]["action"]["enum"].append("add_knowledge");
    schema["properties"]["action"]["enum"].append("list");
    schema["properties"]["action"]["enum"].append("query");
    
    schema["properties"]["key"]["type"] = "string";
    schema["properties"]["key"]["description"] = "Key for preference or knowledge (required for add_preference, add_knowledge, query)";
    
    schema["properties"]["value"]["type"] = "string";
    schema["properties"]["value"]["description"] = "Value to store (required for add_preference, add_knowledge)";
    
    schema["required"] = Json::Value(Json::arrayValue);
    schema["required"].append("action");
    return schema;
}

ToolResult MemoryTool::Execute(const Json::Value& args) {
    if (!args.isMember("action")) {
        return ToolResult{false, "", "Missing required parameter: action"};
    }
    
    std::string action = args["action"].asString();
    auto& global_mem = GlobalMemory::Instance();
    
    if (action == "add_preference") {
        if (!args.isMember("key") || !args.isMember("value")) {
            return ToolResult{false, "", "Missing required parameters: key and value"};
        }
        std::string key = args["key"].asString();
        std::string value = args["value"].asString();
        global_mem.AddUserPreference(key, value);
        return ToolResult{true, std::format("Added user preference: {} = {}", key, value), ""};
    }
    
    if (action == "add_knowledge") {
        if (!args.isMember("key") || !args.isMember("value")) {
            return ToolResult{false, "", "Missing required parameters: key and value"};
        }
        std::string key = args["key"].asString();
        std::string value = args["value"].asString();
        global_mem.AddKnowledge(key, value);
        return ToolResult{true, std::format("Added knowledge: {} = {}", key, value), ""};
    }
    
    if (action == "list") {
        auto knowledge = global_mem.GetAllKnowledge();
        std::string result = "Long-term memory:\n\nUser Preferences:\n";
        
        // 这里可以添加获取所有偏好的方法
        result += "(See system prompt for current preferences)\n\n";
        result += "Knowledge Base:\n";
        for (const auto& [topic, content] : knowledge) {
            result += std::format("- {}: {}\n", topic, content);
        }
        return ToolResult{true, result, ""};
    }
    
    if (action == "query") {
        if (!args.isMember("key")) {
            return ToolResult{false, "", "Missing required parameter: key"};
        }
        std::string key = args["key"].asString();
        // 这里可以实现查询特定 key 的逻辑
        return ToolResult{true, std::format("Query for '{}' - use 'list' to see all memories", key), ""};
    }
    
    return ToolResult{false, "", std::format("Unknown action: {}", action)};
}

} // namespace agent
