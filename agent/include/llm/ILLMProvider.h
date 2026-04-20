#pragma once

#include "LLMTypes.h"

namespace agent {

class ILLMProvider {
public:
    virtual ~ILLMProvider() = default;
    virtual LLMResponse Chat(const LLMRequest& request) = 0;
};

} // namespace agent
