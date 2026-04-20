#pragma once

#include <string>
#include <json/json.h>

namespace agent {

struct AppConfig {
    // LLM 配置
    std::string api_key;
    std::string base_url = "https://api.openai.com/v1";
    std::string model = "gpt-4o-mini";
    
    // Agent 配置
    size_t worker_count = 2;
    int max_tool_iterations = 8;
    
    // Memory 配置
    size_t stm_max_messages = 20;
    size_t summary_trigger_threshold = 10;
    
    // Server 配置
    int port = 8080;
    int thread_num = 4;
};

class ConfigManager {
public:
    static ConfigManager& Instance();
    
    // 从文件加载配置
    bool LoadFromFile(const std::string& filepath);
    
    // 从环境变量加载（优先级高于文件）
    void LoadFromEnv();
    
    // 获取配置
    const AppConfig& GetConfig() const { return config_; }
    AppConfig& GetConfig() { return config_; }

private:
    ConfigManager() = default;
    AppConfig config_;
};

} // namespace agent
