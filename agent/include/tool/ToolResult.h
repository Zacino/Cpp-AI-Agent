#pragma once

#include <string>

namespace agent {

struct ToolResult {
    bool success;
    std::string content;
    std::string error_msg;
};

} // namespace agent
