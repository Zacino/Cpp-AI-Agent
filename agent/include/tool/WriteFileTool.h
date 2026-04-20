#pragma once

#include "ITool.h"

namespace agent {

class WriteFileTool : public ITool {
public:
    std::string Name() const override { return "write_file"; }
    std::string Description() const override { return "Write content to a file"; }
    Json::Value Schema() const override;
    ToolResult Execute(const Json::Value& args) override;
};

} // namespace agent
