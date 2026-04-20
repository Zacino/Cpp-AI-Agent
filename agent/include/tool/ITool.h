#pragma once

#include "ToolResult.h"
#include <json/json.h>
#include <string>

namespace agent {

class ITool {
public:
    virtual ~ITool() = default;
    virtual std::string Name() const = 0;
    virtual std::string Description() const = 0;
    virtual Json::Value Schema() const = 0;
    virtual ToolResult Execute(const Json::Value& args) = 0;
};

} // namespace agent
