#pragma once

#include "ChatMessage.h"
#include <vector>
#include <string>
#include <mutex>

namespace agent {

// 全局长期记忆 - 跨 session 共享，持久化到磁盘
class GlobalMemory {
public:
    static GlobalMemory& Instance();

    // 加载/保存
    void Load(const std::string& file_path);
    void Save() const;

    // 添加用户偏好/知识
    void AddUserPreference(const std::string& key, const std::string& value);
    void AddKnowledge(const std::string& topic, const std::string& content);
    
    // 纯数据访问（prompt 生成由 PromptBuilder 负责）
    std::vector<std::pair<std::string, std::string>> GetAllKnowledge() const;
    std::vector<std::pair<std::string, std::string>> GetAllPreferences() const;
    
    // 清空
    void Clear();

private:
    GlobalMemory() = default;
    
    mutable std::recursive_mutex mtx_;
    std::string file_path_;
    std::vector<std::pair<std::string, std::string>> user_preferences_;
    std::vector<std::pair<std::string, std::string>> knowledge_base_;
};

} // namespace agent
