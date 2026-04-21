#include "agent/include/memory/PromptBuilder.h"
#include "agent/include/memory/GlobalMemory.h"
#include <format>

namespace agent {

std::string PromptBuilder::BuildSystemPrompt(
    const std::vector<std::pair<std::string, std::string>>& preferences,
    const std::vector<std::pair<std::string, std::string>>& knowledge)
{
    std::string prompt =
        "You are a helpful AI assistant with access to file and memory tools.\n"
        "Available tools:\n"
        "- read_file: Read file contents (path must be within /home/ubuntu)\n"
        "- write_file: Write content to file (path must be within /home/ubuntu)\n"
        "- memory: Manage long-term memory (add/query user preferences and knowledge)\n"
        "\n"
        "When you need to use a tool, respond with a tool call in the function calling format.\n"
        "After receiving tool results, provide a helpful response.\n";

    if (!preferences.empty()) {
        prompt += "\n[User Preferences]\n";
        for (const auto& [key, value] : preferences) {
            prompt += std::format("- {}: {}\n", key, value);
        }
    }

    if (!knowledge.empty()) {
        prompt += "\n[Knowledge Base]\n";
        for (const auto& [topic, content] : knowledge) {
            prompt += std::format("- {}: {}\n", topic, content);
        }
    }

    return prompt;
}

std::string PromptBuilder::BuildSystemPrompt() {
    return BuildSystemPrompt(
        GlobalMemory::Instance().GetAllPreferences(),
        GlobalMemory::Instance().GetAllKnowledge()
    );
}

} // namespace agent
