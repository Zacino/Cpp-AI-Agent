#pragma once

#include "ITool.h"

namespace agent {

class ReadFileTool : public ITool {
public:
    std::string Name() const override { return "read_file"; }
    std::string Description() const override { return "Read the contents of a file"; }
    Json::Value Schema() const override;
    ToolResult Execute(const Json::Value& args) override;
};

} // namespace agent
