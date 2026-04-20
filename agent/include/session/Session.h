#pragma once

#include <string>
#include <vector>
#include <mutex>
#include <cstdint>
#include <memory>
#include <fstream>
#include "memory/ChatMessage.h"
#include "memory/CompositeMemory.h"

namespace agent {

// Session 是纯状态对象，不包含执行线程
class Session : public std::enable_shared_from_this<Session> {
public:
    explicit Session(std::string id);
    ~Session();

    // 禁止拷贝，允许移动
    Session(const Session&) = delete;
    Session& operator=(const Session&) = delete;
    Session(Session&&) = default;
    Session& operator=(Session&&) = default;

    // 基本信息
    const std::string& GetId() const { return id_; }
    int64_t GetCreatedAt() const { return created_at_; }
    int64_t GetUpdatedAt() const { return updated_at_; }

    // 记忆系统访问
    CompositeMemory& Memory() { return *memory_; }
    
    // 获取所有消息（从内存 + 从文件加载）
    std::vector<ChatMessage> GetAllMessages() const;
    
    // 添加消息并保存到文件
    void AddMessage(const ChatMessage& msg);

    // 执行锁 - 保证同一会话串行处理
    std::mutex& ExecutionMutex() { return execution_mtx_; }

    // 更新时间戳
    void Touch();
    
    // 获取文件路径
    std::string GetFilePath() const { return file_path_; }
    
    // 获取第一行作为标题
    std::string GetTitle() const;

private:
    void LoadFromFile();
    void SaveToFile();
    void EnsureDirectoryExists() const;
    
    std::string id_;
    int64_t created_at_;
    int64_t updated_at_;
    std::string file_path_;
    std::unique_ptr<CompositeMemory> memory_;
    std::mutex execution_mtx_;  // 会话级执行锁
    mutable std::mutex file_mtx_;  // 文件操作锁
};

} // namespace agent
