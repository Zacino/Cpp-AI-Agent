#pragma once

#include "IMemory.h"
#include <deque>
#include <mutex>

namespace agent {

// 短期记忆 - 滑动窗口，最近 N 条
class ShortTermMemory : public IMemory {
public:
    explicit ShortTermMemory(size_t max_messages = 20);
    
    void Add(const ChatMessage& msg) override;
    std::vector<ChatMessage> GetContext(size_t max_tokens = 4000) const override;
    void Clear() override;
    size_t Size() const override;
    
    // 获取所有消息（用于摘要）
    std::vector<ChatMessage> GetAll() const;

private:
    size_t max_messages_;
    std::deque<ChatMessage> messages_;
    mutable std::mutex mtx_;
};

} // namespace agent
