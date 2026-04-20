#pragma once

#include <unordered_map>
#include <shared_mutex>
#include <atomic>
#include <memory>
#include <vector>
#include "Session.h"

namespace agent {

// SessionRegistry 强引用管理 Session
class SessionRegistry {
public:
    static SessionRegistry& Instance();

    // 创建新 Session
    std::shared_ptr<Session> Create();

    // 获取 Session（不存在返回 nullptr）
    std::shared_ptr<Session> Get(const std::string& id);

    // 删除 Session
    bool Remove(const std::string& id);

    // 列出所有 Session ID
    std::vector<std::string> List() const;
    
    // 加载所有已保存的 sessions
    void LoadAllSessions();

private:
    SessionRegistry() = default;

    std::unordered_map<std::string, std::shared_ptr<Session>> sessions_;
    mutable std::shared_mutex mtx_;
    std::atomic<uint64_t> counter_{0};
};

} // namespace agent
