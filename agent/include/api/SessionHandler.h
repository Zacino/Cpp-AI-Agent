#pragma once

#include "http/HttpRequest.h"
#include "http/HttpResponse.h"

namespace agent {

class SessionHandler {
public:
    // POST /sessions
    static void CreateSession(const http::HttpRequest& req, http::HttpResponse* resp);
    
    // GET /sessions
    static void ListSessions(const http::HttpRequest& req, http::HttpResponse* resp);
    
    // GET /sessions/{id}
    static void GetSession(const http::HttpRequest& req, http::HttpResponse* resp);
    
    // DELETE /sessions/{id}
    static void DeleteSession(const http::HttpRequest& req, http::HttpResponse* resp);
    
    // GET /sessions/{id}/messages
    static void GetMessages(const http::HttpRequest& req, http::HttpResponse* resp);
};

} // namespace agent
