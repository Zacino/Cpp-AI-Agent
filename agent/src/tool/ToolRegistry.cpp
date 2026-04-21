#include "agent/include/tool/ToolRegistry.h"
#include <format>

namespace agent {

ToolRegistry& ToolRegistry::Instance() {
    static ToolRegistry instance;
    return instance;
}

void ToolRegistry::Register(std::unique_ptr<ITool> tool) {
    std::lock_guard<std::mutex> lock(mtx_);
    tools_[tool->Name()] = std::move(tool);
}

ToolResult ToolRegistry::Execute(const std::string& name, const Json::Value& args) {
    ITool* tool_ptr = nullptr;
    {
        std::lock_guard<std::mutex> lock(mtx_);
        auto it = tools_.find(name);
        if (it == tools_.end()) {
            return ToolResult{false, "", std::format("Tool '{}' not found", name)};
        }
        tool_ptr = it->second.get();  // 锁内只查找，取出指针
    }
    return tool_ptr->Execute(args);  // 锁外执行，避免长时间持锁
}

Json::Value ToolRegistry::GetAllSchemas() const {
    std::lock_guard<std::mutex> lock(mtx_);
    Json::Value schemas(Json::arrayValue);
    
    for (const auto& [name, tool] : tools_) {
        Json::Value tool_def;
        tool_def["type"] = "function";
        tool_def["function"]["name"] = tool->Name();
        tool_def["function"]["description"] = tool->Description();
        tool_def["function"]["parameters"] = tool->Schema();
        schemas.append(tool_def);
    }
    
    return schemas;
}

bool ToolRegistry::HasTool(const std::string& name) const {
    std::lock_guard<std::mutex> lock(mtx_);
    return tools_.find(name) != tools_.end();
}

} // namespace agent
