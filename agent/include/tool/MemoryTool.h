#pragma once

#include "ITool.h"

namespace agent {

// 记忆管理工具 - 添加/查询长期记忆
class MemoryTool : public ITool {
public:
    std::string Name() const override { return "memory"; }
    std::string Description() const override { return "Add or query long-term memory. Use this to remember important information about the user or store knowledge for future sessions."; }
    Json::Value Schema() const override;
    ToolResult Execute(const Json::Value& args) override;
};

} // namespace agent
