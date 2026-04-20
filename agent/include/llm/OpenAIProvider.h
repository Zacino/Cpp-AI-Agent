#pragma once

#include "ILLMProvider.h"
#include <string>

namespace agent {

class OpenAIProvider : public ILLMProvider {
public:
    struct Config {
        std::string api_key;
        std::string base_url = "https://api.openai.com/v1";
        std::string model = "gpt-4o-mini";
        int timeout_ms = 60000;
    };

    explicit OpenAIProvider(Config config);
    LLMResponse Chat(const LLMRequest& request) override;

private:
    std::string BuildRequestBody(const LLMRequest& request);
    LLMResponse ParseResponse(const std::string& response_body);

    Config config_;
};

} // namespace agent
