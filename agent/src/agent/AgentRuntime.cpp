#include "agent/include/agent/AgentRuntime.h"
#include "agent/include/session/SessionRegistry.h"
#include "agent/include/session/Session.h"
#include "agent/include/router/ResponseRouter.h"
#include "agent/include/llm/OpenAIProvider.h"
#include "agent/include/tool/ToolRegistry.h"
#include "agent/include/memory/GlobalMemory.h"
#include <format>
#include <iostream>

namespace agent {

AgentRuntime& AgentRuntime::Instance() {
    static AgentRuntime instance;
    return instance;
}

void AgentRuntime::Initialize(const AgentConfig& config) {
    config_ = config;
    
    // 创建 Bus 通道
    auto [inbound_sender, inbound_receiver] = BusChannel<InboundMessage>::Create(1024);
    auto [outbound_sender, outbound_receiver] = BusChannel<OutboundMessage>::Create(1024);
    
    inbound_sender_ = inbound_sender;
    inbound_receiver_ = inbound_receiver;
    outbound_sender_ = outbound_sender;
    
    // 创建 LLM Provider
    OpenAIProvider::Config llm_config;
    llm_config.api_key = config.openai_api_key;
    llm_config.base_url = config.openai_base_url;
    llm_config.model = config.model;
    llm_provider_ = std::make_unique<OpenAIProvider>(llm_config);
    
    // 启动 OutboundBus 消费者（路由到 ResponseRouter）
    std::jthread outbound_consumer([outbound_receiver](std::stop_token st) {
        while (!st.stop_requested()) {
            auto msg = outbound_receiver->Receive(st);
            if (msg) {
                ResponseRouter::Instance().Fulfill(*msg);
            }
        }
    });
    outbound_consumer.detach();
}

void AgentRuntime::Start() {
    stop_source_ = std::stop_source{};
    
    for (size_t i = 0; i < config_.worker_count; ++i) {
        workers_.emplace_back([this](std::stop_token st) {
            WorkerLoop(st);
        });
    }
    
    std::cout << std::format("AgentRuntime started with {} workers", config_.worker_count) << std::endl;
}

void AgentRuntime::Stop() {
    stop_source_.request_stop();
    for (auto& worker : workers_) {
        if (worker.joinable()) {
            worker.join();
        }
    }
    workers_.clear();
    std::cout << "AgentRuntime stopped" << std::endl;
}

BusChannel<InboundMessage>::Sender AgentRuntime::GetInboundSender() {
    return inbound_sender_;
}

void AgentRuntime::WorkerLoop(std::stop_token st) {
    while (!st.stop_requested()) {
        auto msg = inbound_receiver_->Receive(st);
        if (!msg) continue;
        
        OutboundMessage response = ProcessMessage(*msg);
        outbound_sender_->Send(std::move(response));
    }
}

OutboundMessage AgentRuntime::ProcessMessage(const InboundMessage& msg) {
    OutboundMessage response;
    response.message_id = msg.message_id;
    response.session_id = msg.session_id;
    
    // 获取 Session
    auto session = SessionRegistry::Instance().Get(msg.session_id);
    if (!session) {
        response.success = false;
        response.error_msg = "Session not found";
        return response;
    }
    
    // 获取 Session 执行锁（保证串行处理）
    std::lock_guard<std::mutex> lock(session->ExecutionMutex());
    
    // 获取上下文（所有历史消息）
    std::vector<ChatMessage> context = session->GetAllMessages();
    
    // 运行 Agent Loop
    std::string reply = RunAgentLoop(msg.session_id, context, msg.content);
    
    // 更新 Session 记忆并保存到文件
    ChatMessage user_msg{"user", msg.content, msg.timestamp, {}};
    ChatMessage assistant_msg{"assistant", reply, 
        std::chrono::system_clock::now().time_since_epoch().count(), {}};
    session->AddMessage(user_msg);
    session->AddMessage(assistant_msg);
    
    // 检查是否需要触发摘要
    auto& summary_mem = session->Memory().GetSummary();
    auto& stm = session->Memory().GetSTM();
    if (summary_mem.NeedsSummary(stm.Size())) {
        // TODO: 异步生成摘要
        // 这里简化处理，实际应该调用 LLM 生成摘要
        std::cout << "[Memory] Triggering summarization for session " << msg.session_id << std::endl;
    }
    
    response.content = reply;
    response.success = true;
    return response;
}

std::string AgentRuntime::RunAgentLoop(
    const std::string& session_id,
    const std::vector<ChatMessage>& context,
    const std::string& user_input) {
    
    // 构建 LLM 请求
    LLMRequest request;
    request.model = config_.model;
    request.max_tokens = 4096;
    request.temperature = 0.7f;
    
    // 添加系统提示（包含全局长期记忆）
    std::string system_prompt = GlobalMemory::Instance().GetSystemPrompt();
    request.messages.push_back(LLMMessage{"system", system_prompt, {}});
    
    // 添加上下文
    for (const auto& msg : context) {
        LLMMessage llm_msg{msg.role, msg.content, {}};
        request.messages.push_back(std::move(llm_msg));
    }
    
    // 添加当前用户输入
    request.messages.push_back(LLMMessage{"user", user_input, {}});
    
    // 添加工具 schema
    request.tools = ToolRegistry::Instance().GetAllSchemas();
    
    // Agent Loop - 最多 max_tool_iterations 次
    for (int iteration = 0; iteration < config_.max_tool_iterations; ++iteration) {
        // 调用 LLM
        LLMResponse response = llm_provider_->Chat(request);
        
        if (!response.success) {
            return std::format("Error: {}", response.error_msg);
        }
        
        // 如果没有 tool calls，直接返回内容
        if (response.tool_calls.empty()) {
            return response.content;
        }
        
        // 将 assistant 消息带上 tool_calls 加入历史
        LLMMessage assistant_msg;
        assistant_msg.role = "assistant";
        assistant_msg.content = response.content;
        assistant_msg.tool_calls = response.tool_calls;  // 必须带上 tool_calls
        request.messages.push_back(std::move(assistant_msg));
        
        // 执行每个 tool，并带上对应的 tool_call_id 回复
        for (const auto& tc : response.tool_calls) {
            std::cout << std::format("[Tool] {}: {}", tc.name, tc.arguments.toStyledString()) << std::endl;
            
            ToolResult result = ToolRegistry::Instance().Execute(tc.name, tc.arguments);
            
            LLMMessage tool_msg;
            tool_msg.role = "tool";
            tool_msg.tool_call_id = tc.id;  // 必须与 tool call id 对应
            tool_msg.content = result.success ? result.content : result.error_msg;
            request.messages.push_back(std::move(tool_msg));
        }
    }
    
    return "Error: Maximum tool iterations reached";
}

std::string AgentRuntime::BuildPrompt(
    const std::vector<ChatMessage>& context,
    const std::string& user_input) {
    
    std::string prompt;
    
    // 添加上下文（最近的消息）
    const size_t max_context = 10;
    size_t start = context.size() > max_context ? context.size() - max_context : 0;
    
    for (size_t i = start; i < context.size(); ++i) {
        const auto& msg = context[i];
        prompt += std::format("{}: {}\n", msg.role, msg.content);
    }
    
    // 添加当前输入
    prompt += std::format("user: {}\n", user_input);
    prompt += "assistant: ";
    
    return prompt;
}

} // namespace agent
