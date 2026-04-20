#include "../../include/http/HttpResponse.h"

namespace http
{

void HttpResponse::appendToBuffer(muduo::net::Buffer* outputBuf) const
{
    // HttpResponse封装的信息格式化输出
    char buf[256]; 
    
    // 状态行: HTTP/1.1 200 OK\r\n
    snprintf(buf, sizeof buf, "%s %d %s\r\n", 
             httpVersion_.empty() ? "HTTP/1.1" : httpVersion_.c_str(), 
             statusCode_, 
             statusMessage_.empty() ? "OK" : statusMessage_.c_str());
    outputBuf->append(buf);

    // 连接头
    if (closeConnection_)
    {
        outputBuf->append("Connection: close\r\n");
    }
    else
    {
        outputBuf->append("Connection: keep-alive\r\n");
    }
    
    // Content-Length 必须设置
    snprintf(buf, sizeof buf, "Content-Length: %zu\r\n", body_.size());
    outputBuf->append(buf);

    // 其他 headers
    for (const auto& header : headers_)
    {
        outputBuf->append(header.first);
        outputBuf->append(": "); 
        outputBuf->append(header.second);
        outputBuf->append("\r\n");
    }
    
    // 空行分隔 headers 和 body
    outputBuf->append("\r\n");
    
    // body
    outputBuf->append(body_);
}

void HttpResponse::setStatusLine(const std::string& version,
                                 HttpStatusCode statusCode,
                                 const std::string& statusMessage)
{
    httpVersion_ = version;
    statusCode_ = statusCode;
    statusMessage_ = statusMessage;
}

} // namespace http