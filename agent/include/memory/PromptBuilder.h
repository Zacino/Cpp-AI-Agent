#pragma once
#include <string>
#include <vector>
#include <utility>

namespace agent {

// PromptBuilder - 负责从 GlobalMemory 数据构建 system prompt
// GlobalMemory 只管存储，prompt 格式由此处统一控制
class PromptBuilder {
public:
    // 从全局记忆（用户偏好 + 知识库）构建 system prompt
    static std::string BuildSystemPrompt(
        const std::vector<std::pair<std::string, std::string>>& preferences,
        const std::vector<std::pair<std::string, std::string>>& knowledge
    );

    // 便捷重载：直接从 GlobalMemory::Instance() 读取
    static std::string BuildSystemPrompt();
};

} // namespace agent
