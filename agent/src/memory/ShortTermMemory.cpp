#include "agent/include/memory/ShortTermMemory.h"

namespace agent {

ShortTermMemory::ShortTermMemory(size_t max_messages) 
    : max_messages_(max_messages) {
}

void ShortTermMemory::Add(const ChatMessage& msg) {
    std::lock_guard<std::mutex> lock(mtx_);
    messages_.push_back(msg);
    
    // 滑动窗口：移除旧消息
    while (messages_.size() > max_messages_) {
        messages_.pop_front();
    }
}

std::vector<ChatMessage> ShortTermMemory::GetContext(size_t max_tokens) const {
    std::lock_guard<std::mutex> lock(mtx_);
    return std::vector<ChatMessage>(messages_.begin(), messages_.end());
}

void ShortTermMemory::Clear() {
    std::lock_guard<std::mutex> lock(mtx_);
    messages_.clear();
}

size_t ShortTermMemory::Size() const {
    std::lock_guard<std::mutex> lock(mtx_);
    return messages_.size();
}

std::vector<ChatMessage> ShortTermMemory::GetAll() const {
    std::lock_guard<std::mutex> lock(mtx_);
    return std::vector<ChatMessage>(messages_.begin(), messages_.end());
}

} // namespace agent
