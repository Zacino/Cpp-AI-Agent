#include "agent/include/memory/SummaryMemory.h"

namespace agent {

SummaryMemory::SummaryMemory(size_t trigger_threshold)
    : trigger_threshold_(trigger_threshold) {
}

void SummaryMemory::Add(const ChatMessage& msg) {
    // TODO(phase-3): 摘要记忆未实现。当前消息内容由 ShortTermMemory 管理。
    // 完整闭环需要：达到阈値 -> 调用 LLM 生成摘要 -> UpdateSummary() -> 压缩 STM
    (void)msg;
}

std::vector<ChatMessage> SummaryMemory::GetContext(size_t max_tokens) const {
    std::lock_guard<std::mutex> lock(mtx_);
    
    std::vector<ChatMessage> context;
    if (!summary_.empty()) {
        // 将摘要作为系统消息加入上下文
        ChatMessage summary_msg;
        summary_msg.role = "system";
        summary_msg.content = "[Conversation Summary] " + summary_;
        summary_msg.timestamp = 0;
        context.push_back(std::move(summary_msg));
    }
    return context;
}

void SummaryMemory::Clear() {
    std::lock_guard<std::mutex> lock(mtx_);
    summary_.clear();
}

size_t SummaryMemory::Size() const {
    std::lock_guard<std::mutex> lock(mtx_);
    // 返回摘要的字数作为"大小"
    return summary_.empty() ? 0 : 1;
}

void SummaryMemory::UpdateSummary(const std::string& new_summary) {
    std::lock_guard<std::mutex> lock(mtx_);
    summary_ = new_summary;
}

std::string SummaryMemory::GetSummary() const {
    std::lock_guard<std::mutex> lock(mtx_);
    return summary_;
}

bool SummaryMemory::NeedsSummary(size_t message_count) const {
    return message_count >= trigger_threshold_;
}

} // namespace agent
