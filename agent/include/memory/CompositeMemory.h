#pragma once

#include "IMemory.h"
#include "ShortTermMemory.h"
#include "SummaryMemory.h"
#include <memory>

namespace agent {

// 组合记忆 - 整合 STM + LTM Summary
class CompositeMemory : public IMemory {
public:
    CompositeMemory();
    
    void Add(const ChatMessage& msg) override;
    std::vector<ChatMessage> GetContext(size_t max_tokens = 4000) const override;
    void Clear() override;
    size_t Size() const override;
    
    // 获取各层记忆
    ShortTermMemory& GetSTM() { return *stm_; }
    SummaryMemory& GetSummary() { return *summary_; }
    
    // 触发摘要（由外部调用，如 AgentRuntime）
    void TriggerSummarization(const std::string& summary);

private:
    std::unique_ptr<ShortTermMemory> stm_;
    std::unique_ptr<SummaryMemory> summary_;
    mutable std::mutex mtx_;
};

} // namespace agent
