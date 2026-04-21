#pragma once
#include <chrono>
#include <cstdint>

namespace agent {

// 返回当前 Unix 毫秒时间戳（跨模块统一使用）
inline int64_t NowMs() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
}

} // namespace agent
