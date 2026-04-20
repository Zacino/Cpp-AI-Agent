#include "agent/include/tool/WriteFileTool.h"
#include <fstream>
#include <format>

namespace agent {

Json::Value WriteFileTool::Schema() const {
    Json::Value schema;
    schema["type"] = "object";
    schema["properties"]["path"]["type"] = "string";
    schema["properties"]["path"]["description"] = "Path to the file to write";
    schema["properties"]["content"]["type"] = "string";
    schema["properties"]["content"]["description"] = "Content to write to the file";
    schema["required"] = Json::Value(Json::arrayValue);
    schema["required"].append("path");
    schema["required"].append("content");
    return schema;
}

ToolResult WriteFileTool::Execute(const Json::Value& args) {
    if (!args.isMember("path") || !args.isMember("content")) {
        return ToolResult{false, "", "Missing required arguments: path and content"};
    }
    
    std::string path = args["path"].asString();
    std::string content = args["content"].asString();
    
    // 安全检查：禁止访问父目录
    if (path.find("..") != std::string::npos) {
        return ToolResult{false, "", "Access denied: path cannot contain '..'"};
    }
    
    // 安全检查：必须以 /home/ubuntu 开头
    if (path.find("/home/ubuntu") != 0) {
        return ToolResult{false, "", "Access denied: path must be within /home/ubuntu"};
    }
    
    std::ofstream file(path);
    if (!file.is_open()) {
        return ToolResult{false, "", std::format("Failed to open file for writing: {}", path)};
    }
    
    file << content;
    file.close();
    
    if (file.fail()) {
        return ToolResult{false, "", std::format("Failed to write to file: {}", path)};
    }
    
    return ToolResult{true, std::format("Successfully wrote {} bytes to {}", content.size(), path), ""};
}

} // namespace agent
