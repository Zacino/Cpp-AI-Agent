#include "agent/include/memory/GlobalMemory.h"
#include <format>
#include <fstream>
#include <sstream>
#include <json/json.h>

namespace agent {

GlobalMemory& GlobalMemory::Instance() {
    static GlobalMemory instance;
    return instance;
}

void GlobalMemory::Load(const std::string& file_path) {
    std::lock_guard<std::recursive_mutex> lock(mtx_);
    file_path_ = file_path;
    
    std::ifstream file(file_path);
    if (!file.is_open()) return;
    
    Json::Value root;
    Json::CharReaderBuilder builder;
    std::string errs;
    if (!Json::parseFromStream(builder, file, &root, &errs)) return;
    
    if (root.isMember("user_preferences") && root["user_preferences"].isArray()) {
        for (const auto& item : root["user_preferences"]) {
            if (item.isMember("key") && item.isMember("value")) {
                user_preferences_.emplace_back(item["key"].asString(), item["value"].asString());
            }
        }
    }
    if (root.isMember("knowledge_base") && root["knowledge_base"].isArray()) {
        for (const auto& item : root["knowledge_base"]) {
            if (item.isMember("topic") && item.isMember("content")) {
                knowledge_base_.emplace_back(item["topic"].asString(), item["content"].asString());
            }
        }
    }
}

void GlobalMemory::Save() const {
    std::lock_guard<std::recursive_mutex> lock(mtx_);
    if (file_path_.empty()) return;
    
    Json::Value root;
    root["user_preferences"] = Json::arrayValue;
    for (const auto& [key, value] : user_preferences_) {
        Json::Value item;
        item["key"]   = key;
        item["value"] = value;
        root["user_preferences"].append(item);
    }
    root["knowledge_base"] = Json::arrayValue;
    for (const auto& [topic, content] : knowledge_base_) {
        Json::Value item;
        item["topic"]   = topic;
        item["content"] = content;
        root["knowledge_base"].append(item);
    }
    
    Json::StreamWriterBuilder wb;
    wb["indentation"] = "  ";
    wb["emitUTF8"] = true;
    std::ofstream file(file_path_);
    if (file.is_open()) {
        file << Json::writeString(wb, root);
    }
}

void GlobalMemory::AddUserPreference(const std::string& key, const std::string& value) {
    {
        std::lock_guard<std::recursive_mutex> lock(mtx_);
        for (auto& pref : user_preferences_) {
            if (pref.first == key) {
                pref.second = value;
                Save();
                return;
            }
        }
        user_preferences_.emplace_back(key, value);
        Save();
    }
}

void GlobalMemory::AddKnowledge(const std::string& topic, const std::string& content) {
    std::lock_guard<std::recursive_mutex> lock(mtx_);
    knowledge_base_.emplace_back(topic, content);
    Save();
}

std::string GlobalMemory::GetSystemPrompt() const {
    std::lock_guard<std::recursive_mutex> lock(mtx_);
    
    std::string prompt = R"(You are a helpful AI assistant with access to file tools.
Available tools:
- read_file: Read file contents (path must be within /home/ubuntu)
- write_file: Write content to file (path must be within /home/ubuntu)

When you need to use a tool, respond with a tool call in the function calling format.
After receiving tool results, provide a helpful response.
)";

    // 添加用户偏好
    if (!user_preferences_.empty()) {
        prompt += "\n[User Preferences]\n";
        for (const auto& [key, value] : user_preferences_) {
            prompt += std::format("- {}: {}\n", key, value);
        }
    }
    
    // 添加知识库
    if (!knowledge_base_.empty()) {
        prompt += "\n[Knowledge Base]\n";
        for (const auto& [topic, content] : knowledge_base_) {
            prompt += std::format("- {}: {}\n", topic, content);
        }
    }
    
    return prompt;
}

std::vector<std::pair<std::string, std::string>> GlobalMemory::GetAllKnowledge() const {
    std::lock_guard<std::recursive_mutex> lock(mtx_);
    return knowledge_base_;
}

std::vector<std::pair<std::string, std::string>> GlobalMemory::GetAllPreferences() const {
    std::lock_guard<std::recursive_mutex> lock(mtx_);
    return user_preferences_;
}

void GlobalMemory::Clear() {
    std::lock_guard<std::recursive_mutex> lock(mtx_);
    user_preferences_.clear();
    knowledge_base_.clear();
    Save();
}

} // namespace agent
