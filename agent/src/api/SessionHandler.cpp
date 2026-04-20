#include "agent/include/api/SessionHandler.h"
#include "agent/include/session/SessionRegistry.h"
#include "agent/include/session/Session.h"
#include <json/json.h>
#include <format>

namespace agent {

void SessionHandler::CreateSession(const http::HttpRequest& req, http::HttpResponse* resp) {
    auto session = SessionRegistry::Instance().Create();
    
    Json::Value response;
    response["session_id"] = session->GetId();
    response["title"] = session->GetTitle();
    response["created_at"] = static_cast<Json::Int64>(session->GetCreatedAt());
    
    Json::StreamWriterBuilder builder;
    std::string body = Json::writeString(builder, response);
    resp->setStatusCode(http::HttpResponse::k200Ok);
    resp->setStatusMessage("OK");
    resp->setContentType("application/json");
    resp->setContentLength(body.size());
    resp->setBody(body);
}

void SessionHandler::ListSessions(const http::HttpRequest& req, http::HttpResponse* resp) {
    auto ids = SessionRegistry::Instance().List();
    
    Json::Value sessions(Json::arrayValue);
    for (const auto& id : ids) {
        auto session = SessionRegistry::Instance().Get(id);
        if (session) {
            Json::Value sess;
            sess["session_id"] = id;
            sess["title"] = session->GetTitle();
            sess["created_at"] = static_cast<Json::Int64>(session->GetCreatedAt());
            sess["updated_at"] = static_cast<Json::Int64>(session->GetUpdatedAt());
            sessions.append(sess);
        }
    }
    
    Json::Value response;
    response["sessions"] = sessions;
    
    Json::StreamWriterBuilder builder;
    std::string body = Json::writeString(builder, response);
    resp->setStatusCode(http::HttpResponse::k200Ok);
    resp->setStatusMessage("OK");
    resp->setContentType("application/json");
    resp->setContentLength(body.size());
    resp->setBody(body);
}

void SessionHandler::GetSession(const http::HttpRequest& req, http::HttpResponse* resp) {
    // 从路径中提取 session_id
    std::string path = req.path();
    size_t last_slash = path.find_last_of('/');
    if (last_slash == std::string::npos || last_slash == path.length() - 1) {
        std::string body = "{\"error\":\"Invalid session ID\"}";
        resp->setStatusCode(http::HttpResponse::k400BadRequest);
        resp->setStatusMessage("Bad Request");
        resp->setContentLength(body.size());
        resp->setBody(body);
        return;
    }
    
    std::string session_id = path.substr(last_slash + 1);
    // 去掉可能的 "/messages" 后缀
    size_t messages_pos = session_id.find("/messages");
    if (messages_pos != std::string::npos) {
        session_id = session_id.substr(0, messages_pos);
    }
    
    auto session = SessionRegistry::Instance().Get(session_id);
    if (!session) {
        std::string body = "{\"error\":\"Session not found\"}";
        resp->setStatusCode(http::HttpResponse::k404NotFound);
        resp->setStatusMessage("Not Found");
        resp->setContentLength(body.size());
        resp->setBody(body);
        return;
    }
    
    Json::Value response;
    response["session_id"] = session->GetId();
    response["created_at"] = static_cast<Json::Int64>(session->GetCreatedAt());
    response["updated_at"] = static_cast<Json::Int64>(session->GetUpdatedAt());
    response["message_count"] = static_cast<int>(session->GetAllMessages().size());
    
    Json::StreamWriterBuilder builder;
    std::string body = Json::writeString(builder, response);
    resp->setStatusCode(http::HttpResponse::k200Ok);
    resp->setStatusMessage("OK");
    resp->setContentType("application/json");
    resp->setContentLength(body.size());
    resp->setBody(body);
}

void SessionHandler::DeleteSession(const http::HttpRequest& req, http::HttpResponse* resp) {
    std::string path = req.path();
    size_t last_slash = path.find_last_of('/');
    if (last_slash == std::string::npos || last_slash == path.length() - 1) {
        std::string body = "{\"error\":\"Invalid session ID\"}";
        resp->setStatusCode(http::HttpResponse::k400BadRequest);
        resp->setStatusMessage("Bad Request");
        resp->setContentLength(body.size());
        resp->setBody(body);
        return;
    }
    
    std::string session_id = path.substr(last_slash + 1);
    
    bool removed = SessionRegistry::Instance().Remove(session_id);
    if (!removed) {
        std::string body = "{\"error\":\"Session not found\"}";
        resp->setStatusCode(http::HttpResponse::k404NotFound);
        resp->setStatusMessage("Not Found");
        resp->setContentLength(body.size());
        resp->setBody(body);
        return;
    }
    
    std::string body = "{\"success\":true}";
    resp->setStatusCode(http::HttpResponse::k200Ok);
    resp->setStatusMessage("OK");
    resp->setContentType("application/json");
    resp->setContentLength(body.size());
    resp->setBody(body);
}

void SessionHandler::GetMessages(const http::HttpRequest& req, http::HttpResponse* resp) {
    // 解析路径 /sessions/{id}/messages
    std::string path = req.path();
    size_t sessions_pos = path.find("/sessions/");
    if (sessions_pos == std::string::npos) {
        std::string body = "{\"error\":\"Invalid path\"}";
        resp->setStatusCode(http::HttpResponse::k400BadRequest);
        resp->setStatusMessage("Bad Request");
        resp->setContentLength(body.size());
        resp->setBody(body);
        return;
    }
    
    size_t id_start = sessions_pos + 10;  // len("/sessions/") = 10
    size_t messages_pos = path.find("/messages", id_start);
    if (messages_pos == std::string::npos) {
        std::string body = "{\"error\":\"Invalid path\"}";
        resp->setStatusCode(http::HttpResponse::k400BadRequest);
        resp->setStatusMessage("Bad Request");
        resp->setContentLength(body.size());
        resp->setBody(body);
        return;
    }
    
    std::string session_id = path.substr(id_start, messages_pos - id_start);
    
    auto session = SessionRegistry::Instance().Get(session_id);
    if (!session) {
        std::string body = "{\"error\":\"Session not found\"}";
        resp->setStatusCode(http::HttpResponse::k404NotFound);
        resp->setStatusMessage("Not Found");
        resp->setContentLength(body.size());
        resp->setBody(body);
        return;
    }
    
    // 获取 limit 参数
    std::string limit_str = req.getQueryParameters("limit");
    int limit = 20;
    if (!limit_str.empty()) {
        try {
            limit = std::stoi(limit_str);
        } catch (...) {
            limit = 20;
        }
    }
    
    Json::Value messages(Json::arrayValue);
    const auto& msgs = session->GetAllMessages();
    int start = static_cast<int>(msgs.size()) > limit ? 
                static_cast<int>(msgs.size()) - limit : 0;
    
    for (int i = start; i < static_cast<int>(msgs.size()); ++i) {
        Json::Value msg;
        msg["role"] = msgs[i].role;
        msg["content"] = msgs[i].content;
        msg["timestamp"] = static_cast<Json::Int64>(msgs[i].timestamp);
        messages.append(msg);
    }
    
    Json::Value response;
    response["messages"] = messages;
    
    Json::StreamWriterBuilder builder;
    std::string body = Json::writeString(builder, response);
    resp->setStatusCode(http::HttpResponse::k200Ok);
    resp->setStatusMessage("OK");
    resp->setContentType("application/json");
    resp->setContentLength(body.size());
    resp->setBody(body);
}

} // namespace agent
