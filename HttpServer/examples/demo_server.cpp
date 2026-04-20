#include <iostream>
#include <string>
#include "http/HttpServer.h"

int main()
{
    std::cout << "=== HttpServer Demo ===" << std::endl;
    
    // 创建 HTTP 服务器（端口 8080，不使用 SSL）
    http::HttpServer server(8080, "DemoServer", false);
    
    // 设置线程数
    server.setThreadNum(4);
    
    // 注册 GET 路由
    server.Get("/", [](const http::HttpRequest& req, http::HttpResponse* resp) {
        resp->setStatusCode(http::HttpResponse::k200Ok);
        resp->setStatusMessage("OK");
        resp->setContentType("text/html; charset=utf-8");
        resp->setBody("<h1>Hello, HttpServer!</h1><p>This is a demo server.</p>");
    });
    
    // 注册 API 路由
    server.Get("/api/health", [](const http::HttpRequest& req, http::HttpResponse* resp) {
        resp->setStatusCode(http::HttpResponse::k200Ok);
        resp->setContentType("application/json");
        resp->setBody("{\"status\":\"ok\",\"message\":\"Server is running\"}");
    });
    
    // 注册 POST 路由
    server.Post("/api/echo", [](const http::HttpRequest& req, http::HttpResponse* resp) {
        resp->setStatusCode(http::HttpResponse::k200Ok);
        resp->setContentType("text/plain");
        resp->setBody("Echo: " + req.getBody());
    });
    
    std::cout << "Server starting on port 8080..." << std::endl;
    std::cout << "Test URLs:" << std::endl;
    std::cout << "  http://localhost:8080/" << std::endl;
    std::cout << "  http://localhost:8080/api/health" << std::endl;
    std::cout << "  POST http://localhost:8080/api/echo" << std::endl;
    
    // 启动服务器
    server.start();
    
    return 0;
}
