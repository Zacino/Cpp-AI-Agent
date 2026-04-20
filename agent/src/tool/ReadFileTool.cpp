#include "agent/include/tool/ReadFileTool.h"
#include <fstream>
#include <sstream>
#include <format>

namespace agent {

Json::Value ReadFileTool::Schema() const {
    Json::Value schema;
    schema["type"] = "object";
    schema["properties"]["path"]["type"] = "string";
    schema["properties"]["path"]["description"] = "Path to the file to read";
    schema["required"] = Json::Value(Json::arrayValue);
    schema["required"].append("path");
    return schema;
}

ToolResult ReadFileTool::Execute(const Json::Value& args) {
    if (!args.isMember("path")) {
        return ToolResult{false, "", "Missing required argument: path"};
    }
    
    std::string path = args["path"].asString();
    
    // 安全检查：禁止访问父目录
    if (path.find("..") != std::string::npos) {
        return ToolResult{false, "", "Access denied: path cannot contain '..'"};
    }
    
    // 安全检查：必须以 /home/ubuntu 开头
    if (path.find("/home/ubuntu") != 0) {
        return ToolResult{false, "", "Access denied: path must be within /home/ubuntu"};
    }
    
    std::ifstream file(path);
    if (!file.is_open()) {
        return ToolResult{false, "", std::format("Failed to open file: {}", path)};
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    file.close();
    
    return ToolResult{true, buffer.str(), ""};
}

} // namespace agent
