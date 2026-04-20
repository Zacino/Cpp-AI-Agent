#include <iostream>
#include <string>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include "http/HttpServer.h"
#include "agent/include/agent/AgentRuntime.h"
#include "agent/include/api/SessionHandler.h"
#include "agent/include/api/ChatHandler.h"
#include "agent/include/tool/ToolRegistry.h"
#include "agent/include/tool/ReadFileTool.h"
#include "agent/include/tool/WriteFileTool.h"
#include "agent/include/tool/MemoryTool.h"
#include "agent/include/config/ConfigManager.h"
#include "agent/include/session/SessionRegistry.h"
#include "agent/include/memory/GlobalMemory.h"

int main(int argc, char* argv[])
{
    std::cout << "=== Agent Server ===" << std::endl;
    
    // 加载配置
    auto& config_mgr = agent::ConfigManager::Instance();
    
    // 1. 先尝试从配置文件加载
    std::string config_file = "agent_config.json";
    if (argc > 1) {
        config_file = argv[1];
    }
    bool config_loaded = config_mgr.LoadFromFile(config_file);
    
    // 2. 从环境变量加载（优先级更高）
    config_mgr.LoadFromEnv();
    
    const auto& app_config = config_mgr.GetConfig();
    
    // 检查必需的 API Key
    if (app_config.api_key.empty()) {
        std::cerr << "Error: API key not configured" << std::endl;
        std::cerr << "Please set it in config file or via OPENAI_API_KEY environment variable" << std::endl;
        return 1;
    }
    
    // 初始化 AgentRuntime
    agent::AgentConfig agent_config;
    agent_config.worker_count = app_config.worker_count;
    agent_config.max_tool_iterations = app_config.max_tool_iterations;
    agent_config.openai_api_key = app_config.api_key;
    agent_config.openai_base_url = app_config.base_url;
    agent_config.model = app_config.model;
    
    std::cout << "Initializing AgentRuntime..." << std::endl;
    std::cout << "  Base URL: " << agent_config.openai_base_url << std::endl;
    std::cout << "  Model: " << agent_config.model << std::endl;
    std::cout << "  Workers: " << agent_config.worker_count << std::endl;
    
    // 注册工具
    agent::ToolRegistry::Instance().Register(std::make_unique<agent::ReadFileTool>());
    agent::ToolRegistry::Instance().Register(std::make_unique<agent::WriteFileTool>());
    agent::ToolRegistry::Instance().Register(std::make_unique<agent::MemoryTool>());
    std::cout << "  Tools: read_file, write_file, memory" << std::endl;
    
    // 加载已有 sessions
    agent::SessionRegistry::Instance().LoadAllSessions();
    
    // 加载长期记忆
    agent::GlobalMemory::Instance().Load("memory.json");
    
    agent::AgentRuntime::Instance().Initialize(agent_config);
    agent::AgentRuntime::Instance().Start();
    
    // 创建 HTTP 服务器 - 绑定到所有网卡 (0.0.0.0)
    http::HttpServer server("0.0.0.0", app_config.port, "AgentServer", false);
    server.setThreadNum(app_config.thread_num);
    
    // 健康检查
    server.Get("/api/health", [](const http::HttpRequest& req, http::HttpResponse* resp) {
        std::string body = "{\"status\":\"ok\",\"message\":\"Agent server is running\"}";
        resp->setStatusCode(http::HttpResponse::k200Ok);
        resp->setStatusMessage("OK");
        resp->setContentType("application/json");
        resp->setContentLength(body.size());
        resp->setBody(body);
    });
    
    // Session API
    server.Post("/sessions", [](const http::HttpRequest& req, http::HttpResponse* resp) {
        agent::SessionHandler::CreateSession(req, resp);
    });
    
    server.Get("/sessions", [](const http::HttpRequest& req, http::HttpResponse* resp) {
        agent::SessionHandler::ListSessions(req, resp);
    });
    
    // 动态路由: GET /sessions/{id}/messages
    server.addRoute(http::HttpRequest::kGet, "/sessions/([^/]+)/messages",
        [](const http::HttpRequest& req, http::HttpResponse* resp) {
            agent::SessionHandler::GetMessages(req, resp);
        });
    
    // 动态路由: GET /sessions/{id}
    server.addRoute(http::HttpRequest::kGet, "/sessions/([^/]+)",
        [](const http::HttpRequest& req, http::HttpResponse* resp) {
            agent::SessionHandler::GetSession(req, resp);
        });
    
    // 动态路由: DELETE /sessions/{id}
    server.addRoute(http::HttpRequest::kDelete, "/sessions/([^/]+)",
        [](const http::HttpRequest& req, http::HttpResponse* resp) {
            agent::SessionHandler::DeleteSession(req, resp);
        });
    
    // Chat API
    server.Post("/chat", [](const http::HttpRequest& req, http::HttpResponse* resp) {
        agent::ChatHandler::Chat(req, resp);
    });
    
    // Web UI - 根路径返回 index.html
    server.Get("/", [](const http::HttpRequest& req, http::HttpResponse* resp) {
        std::ifstream file("static/index.html");
        if (!file.is_open()) {
            std::string body = "{\"error\":\"index.html not found\"}";
            resp->setStatusCode(http::HttpResponse::k404NotFound);
            resp->setStatusMessage("Not Found");
            resp->setContentLength(body.size());
            resp->setBody(body);
            return;
        }
        
        std::stringstream buffer;
        buffer << file.rdbuf();
        std::string body = buffer.str();
        
        resp->setStatusCode(http::HttpResponse::k200Ok);
        resp->setStatusMessage("OK");
        resp->setContentType("text/html; charset=utf-8");
        resp->setContentLength(body.size());
        resp->setBody(body);
    });
    
    std::cout << std::endl;
    std::cout << "Agent Server starting on port " << app_config.port << "..." << std::endl;
    std::cout << "API Endpoints:" << std::endl;
    std::cout << "  POST /sessions          - Create new session" << std::endl;
    std::cout << "  GET  /sessions/{id}     - Get session info" << std::endl;
    std::cout << "  GET  /sessions/{id}/messages - Get session messages" << std::endl;
    std::cout << "  DELETE /sessions/{id}   - Delete session" << std::endl;
    std::cout << "  POST /chat              - Send message" << std::endl;
    std::cout << "  GET  /                  - Web UI" << std::endl;
    std::cout << std::endl;
    
    // 启动服务器
    server.start();
    
    // 停止 AgentRuntime
    agent::AgentRuntime::Instance().Stop();
    
    return 0;
}
