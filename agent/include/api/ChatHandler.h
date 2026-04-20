#pragma once

#include "http/HttpRequest.h"
#include "http/HttpResponse.h"

namespace agent {

class ChatHandler {
public:
    // POST /chat
    static void Chat(const http::HttpRequest& req, http::HttpResponse* resp);
};

} // namespace agent
