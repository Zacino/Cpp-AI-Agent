#include "agent/include/config/ConfigManager.h"
#include <fstream>
#include <iostream>
#include <cstdlib>

namespace agent {

ConfigManager& ConfigManager::Instance() {
    static ConfigManager instance;
    return instance;
}

bool ConfigManager::LoadFromFile(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "Failed to open config file: " << filepath << std::endl;
        return false;
    }
    
    Json::Value root;
    Json::CharReaderBuilder builder;
    std::string errors;
    
    std::string content((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());
    file.close();
    
    std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
    if (!reader->parse(content.c_str(), content.c_str() + content.size(), 
                       &root, &errors)) {
        std::cerr << "Failed to parse config file: " << errors << std::endl;
        return false;
    }
    
    // 读取 LLM 配置
    if (root.isMember("llm")) {
        const Json::Value& llm = root["llm"];
        if (llm.isMember("api_key")) config_.api_key = llm["api_key"].asString();
        if (llm.isMember("base_url")) config_.base_url = llm["base_url"].asString();
        if (llm.isMember("model")) config_.model = llm["model"].asString();
    }
    
    // 读取 Agent 配置
    if (root.isMember("agent")) {
        const Json::Value& agent = root["agent"];
        if (agent.isMember("worker_count")) config_.worker_count = agent["worker_count"].asUInt();
        if (agent.isMember("max_tool_iterations")) config_.max_tool_iterations = agent["max_tool_iterations"].asInt();
    }
    
    // 读取 Memory 配置
    if (root.isMember("memory")) {
        const Json::Value& memory = root["memory"];
        if (memory.isMember("stm_max_messages")) config_.stm_max_messages = memory["stm_max_messages"].asUInt();
        if (memory.isMember("summary_trigger_threshold")) config_.summary_trigger_threshold = memory["summary_trigger_threshold"].asUInt();
    }
    
    // 读取 Server 配置
    if (root.isMember("server")) {
        const Json::Value& server = root["server"];
        if (server.isMember("port")) config_.port = server["port"].asInt();
        if (server.isMember("thread_num")) config_.thread_num = server["thread_num"].asInt();
    }
    
    std::cout << "Config loaded from: " << filepath << std::endl;
    return true;
}

void ConfigManager::LoadFromEnv() {
    // LLM 配置（环境变量优先级高于配置文件）
    const char* api_key = std::getenv("OPENAI_API_KEY");
    if (api_key && std::strlen(api_key) > 0) {
        config_.api_key = api_key;
    }
    
    const char* base_url = std::getenv("OPENAI_BASE_URL");
    if (base_url && std::strlen(base_url) > 0) {
        config_.base_url = base_url;
    }
    
    const char* model = std::getenv("OPENAI_MODEL");
    if (model && std::strlen(model) > 0) {
        config_.model = model;
    }
    
    // Agent 配置
    const char* workers = std::getenv("AGENT_WORKERS");
    if (workers) {
        config_.worker_count = std::stoul(workers);
    }
    
    const char* max_iter = std::getenv("AGENT_MAX_ITERATIONS");
    if (max_iter) {
        config_.max_tool_iterations = std::stoi(max_iter);
    }
    
    // Server 配置
    const char* port = std::getenv("SERVER_PORT");
    if (port) {
        config_.port = std::stoi(port);
    }
    
    const char* threads = std::getenv("SERVER_THREADS");
    if (threads) {
        config_.thread_num = std::stoi(threads);
    }
}

} // namespace agent
