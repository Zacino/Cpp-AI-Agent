#include "agent/include/memory/CompositeMemory.h"
#include "agent/include/config/ConfigManager.h"

namespace agent {

CompositeMemory::CompositeMemory() {
    const auto& cfg = ConfigManager::Instance().GetConfig();
    stm_     = std::make_unique<ShortTermMemory>(cfg.stm_max_messages);
    summary_ = std::make_unique<SummaryMemory>(cfg.summary_trigger_threshold);
}

void CompositeMemory::Add(const ChatMessage& msg) {
    std::lock_guard<std::mutex> lock(mtx_);
    stm_->Add(msg);
}

std::vector<ChatMessage> CompositeMemory::GetContext(size_t max_tokens) const {
    std::lock_guard<std::mutex> lock(mtx_);
    
    std::vector<ChatMessage> context;
    
    // 1. 添加摘要（如果有）
    auto summary_context = summary_->GetContext(max_tokens);
    context.insert(context.end(), summary_context.begin(), summary_context.end());
    
    // 2. 添加短期记忆
    auto stm_context = stm_->GetContext(max_tokens);
    context.insert(context.end(), stm_context.begin(), stm_context.end());
    
    return context;
}

void CompositeMemory::Clear() {
    std::lock_guard<std::mutex> lock(mtx_);
    stm_->Clear();
    summary_->Clear();
}

size_t CompositeMemory::Size() const {
    std::lock_guard<std::mutex> lock(mtx_);
    return stm_->Size() + summary_->Size();
}

void CompositeMemory::TriggerSummarization(const std::string& summary) {
    std::lock_guard<std::mutex> lock(mtx_);
    summary_->UpdateSummary(summary);
    // 可选：清空短期记忆或保留最近几条
}

} // namespace agent
