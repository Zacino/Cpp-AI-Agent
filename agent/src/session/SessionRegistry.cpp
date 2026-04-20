#include "agent/include/session/SessionRegistry.h"
#include <format>
#include <filesystem>
#include <iostream>

namespace agent {

namespace fs = std::filesystem;

SessionRegistry& SessionRegistry::Instance() {
    static SessionRegistry instance;
    return instance;
}

std::shared_ptr<Session> SessionRegistry::Create() {
    uint64_t id_num = counter_.fetch_add(1);
    std::string id = std::format("sess_{:06d}", id_num);
    
    auto session = std::make_shared<Session>(id);
    
    std::unique_lock<std::shared_mutex> lock(mtx_);
    sessions_[id] = session;
    
    return session;
}

std::shared_ptr<Session> SessionRegistry::Get(const std::string& id) {
    std::shared_lock<std::shared_mutex> lock(mtx_);
    auto it = sessions_.find(id);
    if (it != sessions_.end()) {
        return it->second;
    }
    return nullptr;
}

bool SessionRegistry::Remove(const std::string& id) {
    std::unique_lock<std::shared_mutex> lock(mtx_);
    return sessions_.erase(id) > 0;
}

std::vector<std::string> SessionRegistry::List() const {
    std::shared_lock<std::shared_mutex> lock(mtx_);
    std::vector<std::string> ids;
    ids.reserve(sessions_.size());
    for (const auto& [id, _] : sessions_) {
        ids.push_back(id);
    }
    return ids;
}

void SessionRegistry::LoadAllSessions() {
    if (!fs::exists("sessions")) {
        return;
    }
    
    for (const auto& entry : fs::directory_iterator("sessions")) {
        if (entry.is_regular_file() && entry.path().extension() == ".md") {
            std::string filename = entry.path().stem().string();
            // 检查是否是有效的 session id (sess_XXXXXX)
            if (filename.starts_with("sess_")) {
                auto session = std::make_shared<Session>(filename);
                std::unique_lock<std::shared_mutex> lock(mtx_);
                sessions_[filename] = session;
                
                // 更新计数器
                try {
                    int num = std::stoi(filename.substr(5));
                    if (num >= counter_) {
                        counter_ = num + 1;
                    }
                } catch (...) {}
            }
        }
    }
    
    std::cout << "Loaded " << sessions_.size() << " sessions from disk" << std::endl;
}

} // namespace agent
