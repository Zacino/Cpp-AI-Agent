#include "agent/include/session/Session.h"
#include "agent/include/utils/TimeUtils.h"
#include <format>
#include <filesystem>
#include <iostream>
#include <sstream>
#include <json/json.h>

namespace agent {

namespace fs = std::filesystem;

Session::Session(std::string id) 
    : id_(std::move(id))
    , created_at_(NowMs())
    , updated_at_(created_at_)
    , memory_(std::make_unique<CompositeMemory>()) {
    
    file_path_ = std::format("sessions/{}.md", id_);
    EnsureDirectoryExists();
    LoadFromFile();
}

Session::~Session() {
    // 析构时不需要再保存，因为 AddMessage 已经实时追加
}

void Session::EnsureDirectoryExists() const {
    fs::path dir = fs::path(file_path_).parent_path();
    if (!dir.empty() && !fs::exists(dir)) {
        fs::create_directories(dir);
    }
}

void Session::LoadFromFile() {
    std::lock_guard<std::mutex> lock(file_mtx_);
    
    if (!fs::exists(file_path_)) {
        return;
    }
    
    std::ifstream file(file_path_);
    if (!file.is_open()) {
        std::cerr << "Failed to open session file: " << file_path_ << std::endl;
        return;
    }
    
    Json::CharReaderBuilder reader_builder;
    std::string line;
    
    while (std::getline(file, line)) {
        if (line.empty()) continue;
        
        Json::Value root;
        std::string errs;
        std::istringstream ss(line);
        
        if (!Json::parseFromStream(reader_builder, ss, &root, &errs)) {
            continue;  // 跳过无法解析的行（如旧格式）
        }
        
        if (!root.isObject() || !root.isMember("role") || !root.isMember("content")) {
            continue;  // 跳过非消息对象
        }
        
        ChatMessage msg;
        msg.role      = root["role"].asString();
        msg.content   = root["content"].asString();
        msg.timestamp = root["timestamp"].asInt64();
        
        if (!msg.role.empty() && !msg.content.empty()) {
            full_history_.push_back(msg);  // 完整历史（用于前端展示）
            memory_->Add(msg);             // STM 窗口（用于 prompt 构建）
        }
    }
    
    file.close();
}

// 追加一条消息到文件末尾（无需重写全文）
static void AppendMsgToFile(const std::string& file_path, const ChatMessage& msg) {
    std::ofstream file(file_path, std::ios::app);
    if (!file.is_open()) return;
    
    Json::Value root;
    root["role"]      = msg.role;
    root["content"]   = msg.content;
    root["timestamp"] = static_cast<Json::Int64>(msg.timestamp);
    
    Json::StreamWriterBuilder wb;
    wb["indentation"] = "";   // 压缩成单行
    wb["emitUTF8"] = true;    // 输出原始 UTF-8，避免 \uXXXX 转义
    file << Json::writeString(wb, root) << "\n";
}

std::vector<ChatMessage> Session::GetAllMessages() const {
    return full_history_;  // 完整历史，供前端展示
}

std::vector<ChatMessage> Session::GetContextMessages() const {
    return memory_->GetContext();  // STM 窗口，供 LLM prompt 构建
}

void Session::AddMessage(const ChatMessage& msg) {
    full_history_.push_back(msg);  // 追加到完整历史
    memory_->Add(msg);             // 追加到 STM 窗口
    {
        std::lock_guard<std::mutex> lock(file_mtx_);
        AppendMsgToFile(file_path_, msg);
    }
    Touch();
}

std::string Session::GetTitle() const {
    for (const auto& msg : full_history_) {
        if (msg.role == "user" && !msg.content.empty()) {
            std::string title = msg.content;
            size_t nl = title.find('\n');
            if (nl != std::string::npos) title = title.substr(0, nl);
            while (!title.empty() && (title.front() == ' ' || title.front() == '\t')) title.erase(title.begin());
            while (!title.empty() && (title.back()  == ' ' || title.back()  == '\t')) title.pop_back();
            if (!title.empty()) {
                if (title.size() > 30) title = title.substr(0, 28) + "...";
                return title;
            }
        }
    }
    return std::format("新对话 {}", id_.substr(0, 8));
}

void Session::Touch() {
    updated_at_ = NowMs();
}

} // namespace agent
