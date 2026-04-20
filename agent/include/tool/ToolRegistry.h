#pragma once

#include "ITool.h"
#include <unordered_map>
#include <memory>
#include <mutex>

namespace agent {

class ToolRegistry {
public:
    static ToolRegistry& Instance();

    void Register(std::unique_ptr<ITool> tool);
    ToolResult Execute(const std::string& name, const Json::Value& args);
    Json::Value GetAllSchemas() const;
    bool HasTool(const std::string& name) const;

private:
    ToolRegistry() = default;

    std::unordered_map<std::string, std::unique_ptr<ITool>> tools_;
    mutable std::mutex mtx_;
};

} // namespace agent
