#pragma once

#include "ChatMessage.h"
#include <vector>
#include <string>

namespace agent {

// 记忆接口 - 所有记忆类型都实现此接口
class IMemory {
public:
    virtual ~IMemory() = default;
    
    // 添加消息到记忆
    virtual void Add(const ChatMessage& msg) = 0;
    
    // 获取上下文（用于 LLM）
    virtual std::vector<ChatMessage> GetContext(size_t max_tokens = 4000) const = 0;
    
    // 清空记忆
    virtual void Clear() = 0;
    
    // 获取记忆大小（消息数量）
    virtual size_t Size() const = 0;
};

} // namespace agent
