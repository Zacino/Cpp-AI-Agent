#include "agent/include/llm/OpenAIProvider.h"
#include <curl/curl.h>
#include <format>
#include <iostream>

namespace agent {

static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* userp) {
    userp->append((char*)contents, size * nmemb);
    return size * nmemb;
}

OpenAIProvider::OpenAIProvider(Config config) : config_(std::move(config)) {
}

LLMResponse OpenAIProvider::Chat(const LLMRequest& request) {
    LLMResponse response;
    
    CURL* curl = curl_easy_init();
    if (!curl) {
        response.success = false;
        response.error_msg = "Failed to initialize CURL";
        return response;
    }

    std::string url = config_.base_url + "/chat/completions";
    std::string request_body = BuildRequestBody(request);
    std::string response_body;

    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, std::format("Authorization: Bearer {}", config_.api_key).c_str());

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, request_body.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_body);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, config_.timeout_ms);

    CURLcode res = curl_easy_perform(curl);
    
    if (res != CURLE_OK) {
        response.success = false;
        response.error_msg = std::format("CURL error: {}", curl_easy_strerror(res));
    } else {
        response = ParseResponse(response_body);
    }

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    return response;
}

std::string OpenAIProvider::BuildRequestBody(const LLMRequest& request) {
    Json::Value root;
    root["model"] = request.model;
    root["max_tokens"] = request.max_tokens;
    root["temperature"] = request.temperature;

    Json::Value messages(Json::arrayValue);
    for (const auto& msg : request.messages) {
        Json::Value m;
        m["role"] = msg.role;
        
        if (msg.role == "assistant" && !msg.tool_calls.empty()) {
            // assistant 消息带 tool_calls
            if (!msg.content.empty()) {
                m["content"] = msg.content;
            } else {
                m["content"] = Json::Value::null;
            }
            Json::Value tool_calls_arr(Json::arrayValue);
            for (const auto& tc : msg.tool_calls) {
                Json::Value tc_json;
                tc_json["id"] = tc.id;
                tc_json["type"] = "function";
                tc_json["function"]["name"] = tc.name;
                Json::StreamWriterBuilder wb;
                wb["indentation"] = "";
                tc_json["function"]["arguments"] = Json::writeString(wb, tc.arguments);
                tool_calls_arr.append(tc_json);
            }
            m["tool_calls"] = tool_calls_arr;
        } else if (msg.role == "tool") {
            // tool 结果消息必须带 tool_call_id
            m["content"] = msg.content;
            m["tool_call_id"] = msg.tool_call_id;
        } else {
            m["content"] = msg.content;
        }
        
        messages.append(m);
    }
    root["messages"] = messages;

    if (!request.tools.empty()) {
        root["tools"] = request.tools;
    }

    Json::StreamWriterBuilder builder;
    return Json::writeString(builder, root);
}

LLMResponse OpenAIProvider::ParseResponse(const std::string& response_body) {
    LLMResponse response;
    
    Json::Value root;
    Json::CharReaderBuilder builder;
    std::string errors;

    std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
    if (!reader->parse(response_body.c_str(), 
                       response_body.c_str() + response_body.size(), 
                       &root, &errors)) {
        response.success = false;
        response.error_msg = std::format("JSON parse error: {}", errors);
        return response;
    }

    if (root.isMember("error")) {
        response.success = false;
        response.error_msg = root["error"]["message"].asString();
        return response;
    }

    const Json::Value& choice = root["choices"][0];
    response.finish_reason = choice["finish_reason"].asString();

    if (choice.isMember("message")) {
        const Json::Value& message = choice["message"];
        response.content = message["content"].asString();

        // Parse tool calls
        if (message.isMember("tool_calls")) {
            const Json::Value& tool_calls = message["tool_calls"];
            for (const auto& tc : tool_calls) {
                ToolCall call;
                call.id = tc["id"].asString();
                call.name = tc["function"]["name"].asString();
                
                std::cerr << "[DEBUG] tool_call id='" << call.id << "' name='" << call.name << "'" << std::endl;
                
                std::string args_str = tc["function"]["arguments"].asString();
                Json::CharReaderBuilder args_builder;
                std::unique_ptr<Json::CharReader> args_reader(args_builder.newCharReader());
                args_reader->parse(args_str.c_str(), args_str.c_str() + args_str.size(), 
                                   &call.arguments, nullptr);
                
                response.tool_calls.push_back(std::move(call));
            }
        }
    }

    return response;
}

} // namespace agent
