#pragma once

#include "bus/BusChannel.h"
#include "bus/MessageTypes.h"
#include "memory/ChatMessage.h"
#include "llm/ILLMProvider.h"
#include <vector>
#include <thread>
#include <stop_token>
#include <memory>

namespace agent {

struct AgentConfig {
    size_t worker_count = 2;
    int max_tool_iterations = 8;
    std::string openai_api_key;
    std::string openai_base_url = "https://api.openai.com/v1";
    std::string model = "gpt-4o-mini";
};

class AgentRuntime {
public:
    static AgentRuntime& Instance();

    void Initialize(const AgentConfig& config);
    void Start();
    void Stop();

    // 获取 Bus 端点（供 HTTP 层使用）
    BusChannel<InboundMessage>::Sender GetInboundSender();

private:
    AgentRuntime() = default;

    void WorkerLoop(std::stop_token st);
    OutboundMessage ProcessMessage(const InboundMessage& msg);
    std::string RunAgentLoop(
        const std::string& session_id,
        const std::vector<ChatMessage>& context,
        const std::string& user_input);

    AgentConfig config_;
    std::vector<std::jthread> workers_;
    std::jthread outbound_consumer_;  // 受控 outbound 消费者线程

    // Bus 通道
    BusChannel<InboundMessage>::Sender inbound_sender_;
    BusChannel<InboundMessage>::Receiver inbound_receiver_;
    BusChannel<OutboundMessage>::Sender outbound_sender_;

    // LLM Provider
    std::unique_ptr<ILLMProvider> llm_provider_;
};

} // namespace agent
