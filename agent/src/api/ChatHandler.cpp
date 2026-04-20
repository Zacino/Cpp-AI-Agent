#include "agent/include/api/ChatHandler.h"
#include "agent/include/agent/AgentRuntime.h"
#include "agent/include/session/SessionRegistry.h"
#include "agent/include/session/Session.h"
#include "agent/include/router/ResponseRouter.h"
#include "agent/include/bus/MessageTypes.h"
#include <json/json.h>
#include <format>
#include <chrono>

namespace agent {

void ChatHandler::Chat(const http::HttpRequest& req, http::HttpResponse* resp) {
    // 解析请求体
    std::string body = req.getBody();
    
    Json::Value request_json;
    Json::CharReaderBuilder builder;
    std::string errors;
    
    std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
    if (!reader->parse(body.c_str(), body.c_str() + body.size(), &request_json, &errors)) {
        std::string resp_body = std::format("{{\"error\":\"Invalid JSON: {}\"}}", errors);
        resp->setStatusCode(http::HttpResponse::k400BadRequest);
        resp->setStatusMessage("Bad Request");
        resp->setContentLength(resp_body.size());
        resp->setBody(resp_body);
        return;
    }
    
    // 验证必需字段
    if (!request_json.isMember("session_id") || !request_json.isMember("message")) {
        std::string resp_body = "{\"error\":\"Missing session_id or message\"}";
        resp->setStatusCode(http::HttpResponse::k400BadRequest);
        resp->setStatusMessage("Bad Request");
        resp->setContentLength(resp_body.size());
        resp->setBody(resp_body);
        return;
    }
    
    std::string session_id = request_json["session_id"].asString();
    std::string message = request_json["message"].asString();
    
    // 验证 session 存在
    auto session = SessionRegistry::Instance().Get(session_id);
    if (!session) {
        std::string resp_body = "{\"error\":\"Session not found\"}";
        resp->setStatusCode(http::HttpResponse::k404NotFound);
        resp->setStatusMessage("Not Found");
        resp->setContentLength(resp_body.size());
        resp->setBody(resp_body);
        return;
    }
    
    // 生成 message_id
    static std::atomic<uint64_t> msg_counter{0};
    std::string message_id = std::format("msg_{:06d}", msg_counter.fetch_add(1));
    
    // 注册到 ResponseRouter
    auto future = ResponseRouter::Instance().Register(message_id);
    
    // 构建 InboundMessage
    InboundMessage inbound;
    inbound.message_id = message_id;
    inbound.session_id = session_id;
    inbound.content = message;
    inbound.timestamp = std::chrono::system_clock::now().time_since_epoch().count();
    
    // 发送到 InboundBus
    auto sender = AgentRuntime::Instance().GetInboundSender();
    if (!sender->Send(std::move(inbound))) {
        ResponseRouter::Instance().Fail(message_id, "Failed to send message to bus");
        std::string resp_body = "{\"error\":\"Failed to process message\"}";
        resp->setStatusCode(http::HttpResponse::k500InternalServerError);
        resp->setStatusMessage("Internal Server Error");
        resp->setContentLength(resp_body.size());
        resp->setBody(resp_body);
        return;
    }
    
    // 等待响应（同步等待，带超时）
    auto status = future.wait_for(std::chrono::seconds(120));
    if (status == std::future_status::timeout) {
        ResponseRouter::Instance().Fail(message_id, "Request timeout");
        std::string resp_body = "{\"error\":\"Request timeout\"}";
        resp->setStatusCode(http::HttpResponse::k500InternalServerError);
        resp->setStatusMessage("Internal Server Error");
        resp->setContentLength(resp_body.size());
        resp->setBody(resp_body);
        return;
    }
    
    // 获取结果
    OutboundMessage outbound = future.get();
    
    // 构建响应
    Json::Value response_json;
    response_json["message_id"] = outbound.message_id;
    response_json["session_id"] = outbound.session_id;
    response_json["reply"] = outbound.content;
    response_json["tool_calls"] = outbound.tool_calls;
    response_json["success"] = outbound.success;
    if (!outbound.success) {
        response_json["error"] = outbound.error_msg;
    }
    
    Json::StreamWriterBuilder writer_builder;
    std::string resp_body = Json::writeString(writer_builder, response_json);
    resp->setStatusCode(http::HttpResponse::k200Ok);
    resp->setStatusMessage("OK");
    resp->setContentType("application/json");
    resp->setContentLength(resp_body.size());
    resp->setBody(resp_body);
}

} // namespace agent
