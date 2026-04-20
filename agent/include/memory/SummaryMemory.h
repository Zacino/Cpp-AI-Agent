#pragma once

#include "IMemory.h"
#include <string>
#include <mutex>

namespace agent {

// 长期摘要记忆 - 定期摘要，压缩历史
class SummaryMemory : public IMemory {
public:
    explicit SummaryMemory(size_t trigger_threshold = 10);
    
    void Add(const ChatMessage& msg) override;
    std::vector<ChatMessage> GetContext(size_t max_tokens = 4000) const override;
    void Clear() override;
    size_t Size() const override;
    
    // 更新摘要
    void UpdateSummary(const std::string& new_summary);
    
    // 获取当前摘要
    std::string GetSummary() const;
    
    // 检查是否需要摘要（消息数超过阈值）
    bool NeedsSummary(size_t message_count) const;

private:
    size_t trigger_threshold_;
    std::string summary_;
    mutable std::mutex mtx_;
};

} // namespace agent
