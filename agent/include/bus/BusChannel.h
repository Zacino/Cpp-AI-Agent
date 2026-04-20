#pragma once

#include <queue>
#include <mutex>
#include <condition_variable>
#include <optional>
#include <stop_token>
#include <memory>
#include <atomic>

namespace agent {

// 通道内部实现
template<typename T>
class ChannelImpl {
public:
    explicit ChannelImpl(size_t capacity) : capacity_(capacity), closed_(false) {}

    bool Send(T&& value) {
        std::unique_lock<std::mutex> lock(mtx_);
        // 等待空间或关闭
        cv_not_full_.wait(lock, [this] {
            return queue_.size() < capacity_ || closed_.load();
        });
        
        if (closed_.load()) return false;
        
        queue_.push(std::move(value));
        cv_not_empty_.notify_one();
        return true;
    }

    bool TrySend(T&& value) {
        std::unique_lock<std::mutex> lock(mtx_);
        if (queue_.size() >= capacity_ || closed_.load()) {
            return false;
        }
        queue_.push(std::move(value));
        cv_not_empty_.notify_one();
        return true;
    }

    std::optional<T> Receive(std::stop_token st) {
        std::unique_lock<std::mutex> lock(mtx_);
        
        // 使用 stop_token 注册回调
        std::stop_callback cb(st, [this] {
            cv_not_empty_.notify_all();
            cv_not_full_.notify_all();
        });
        
        cv_not_empty_.wait(lock, [this, &st] {
            return !queue_.empty() || closed_.load() || st.stop_requested();
        });
        
        if (st.stop_requested() || (closed_.load() && queue_.empty())) {
            return std::nullopt;
        }
        
        T value = std::move(queue_.front());
        queue_.pop();
        cv_not_full_.notify_one();
        return value;
    }

    std::optional<T> TryReceive() {
        std::unique_lock<std::mutex> lock(mtx_);
        if (queue_.empty()) {
            return std::nullopt;
        }
        T value = std::move(queue_.front());
        queue_.pop();
        cv_not_full_.notify_one();
        return value;
    }

    void Close() {
        closed_.store(true);
        cv_not_empty_.notify_all();
        cv_not_full_.notify_all();
    }

    bool IsClosed() const { return closed_.load(); }

private:
    std::queue<T> queue_;
    mutable std::mutex mtx_;
    std::condition_variable cv_not_empty_;
    std::condition_variable cv_not_full_;
    size_t capacity_;
    std::atomic<bool> closed_;
};

// 发送端
template<typename T>
class ChannelSender {
public:
    explicit ChannelSender(std::shared_ptr<ChannelImpl<T>> impl) : impl_(impl) {}

    bool Send(T&& value) {
        if (!impl_) return false;
        return impl_->Send(std::move(value));
    }

    bool TrySend(T&& value) {
        if (!impl_) return false;
        return impl_->TrySend(std::move(value));
    }

    void Close() {
        if (impl_) impl_->Close();
    }

private:
    std::shared_ptr<ChannelImpl<T>> impl_;
};

// 接收端
template<typename T>
class ChannelReceiver {
public:
    explicit ChannelReceiver(std::shared_ptr<ChannelImpl<T>> impl) : impl_(impl) {}

    std::optional<T> Receive(std::stop_token st) {
        if (!impl_) return std::nullopt;
        return impl_->Receive(st);
    }

    std::optional<T> TryReceive() {
        if (!impl_) return std::nullopt;
        return impl_->TryReceive();
    }

private:
    std::shared_ptr<ChannelImpl<T>> impl_;
};

// BusChannel 工厂
template<typename T>
class BusChannel {
public:
    using Sender = std::shared_ptr<ChannelSender<T>>;
    using Receiver = std::shared_ptr<ChannelReceiver<T>>;

    static std::pair<Sender, Receiver> Create(size_t capacity = 1024) {
        auto impl = std::make_shared<ChannelImpl<T>>(capacity);
        auto sender = std::make_shared<ChannelSender<T>>(impl);
        auto receiver = std::make_shared<ChannelReceiver<T>>(impl);
        return {sender, receiver};
    }
};

} // namespace agent
