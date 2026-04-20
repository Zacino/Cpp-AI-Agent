cmake_minimum_required(VERSION 3.20)
project(agent_server)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 查找依赖
find_package(OpenSSL REQUIRED)
find_package(CURL REQUIRED)

# 添加头文件路径
include_directories(
    ${PROJECT_SOURCE_DIR}
    ${PROJECT_SOURCE_DIR}/HttpServer/include
    ${PROJECT_SOURCE_DIR}/agent/include
    ${OPENSSL_INCLUDE_DIR}
    ${CURL_INCLUDE_DIRS}
    /usr/include/jsoncpp
)

# HttpServer 核心源文件
file(GLOB_RECURSE HTTP_SERVER_SRC
    "${PROJECT_SOURCE_DIR}/HttpServer/src/http/*.cpp"
    "${PROJECT_SOURCE_DIR}/HttpServer/src/router/*.cpp"
    "${PROJECT_SOURCE_DIR}/HttpServer/src/middleware/*.cpp"
    "${PROJECT_SOURCE_DIR}/HttpServer/src/session/*.cpp"
    "${PROJECT_SOURCE_DIR}/HttpServer/src/ssl/*.cpp"
)

# Agent 模块源文件
set(AGENT_SRC
    ${PROJECT_SOURCE_DIR}/agent/src/session/Session.cpp
    ${PROJECT_SOURCE_DIR}/agent/src/session/SessionRegistry.cpp
    ${PROJECT_SOURCE_DIR}/agent/src/router/ResponseRouter.cpp
    ${PROJECT_SOURCE_DIR}/agent/src/agent/AgentRuntime.cpp
    ${PROJECT_SOURCE_DIR}/agent/src/api/SessionHandler.cpp
    ${PROJECT_SOURCE_DIR}/agent/src/api/ChatHandler.cpp
    ${PROJECT_SOURCE_DIR}/agent/src/llm/OpenAIProvider.cpp
    ${PROJECT_SOURCE_DIR}/agent/src/tool/ToolRegistry.cpp
    ${PROJECT_SOURCE_DIR}/agent/src/tool/ReadFileTool.cpp
    ${PROJECT_SOURCE_DIR}/agent/src/tool/WriteFileTool.cpp
    ${PROJECT_SOURCE_DIR}/agent/src/tool/MemoryTool.cpp
    ${PROJECT_SOURCE_DIR}/agent/src/memory/ShortTermMemory.cpp
    ${PROJECT_SOURCE_DIR}/agent/src/memory/SummaryMemory.cpp
    ${PROJECT_SOURCE_DIR}/agent/src/memory/CompositeMemory.cpp
    ${PROJECT_SOURCE_DIR}/agent/src/memory/GlobalMemory.cpp
    ${PROJECT_SOURCE_DIR}/agent/src/config/ConfigManager.cpp
)

# 主程序
set(MAIN_SRC "${PROJECT_SOURCE_DIR}/src/main.cpp")

# 添加可执行文件
add_executable(agent_server
    ${MAIN_SRC}
    ${HTTP_SERVER_SRC}
    ${AGENT_SRC}
)

# 链接库
target_link_libraries(agent_server
    pthread
    muduo_net
    muduo_base
    ssl
    crypto
    ${CURL_LIBRARIES}
    jsoncpp
)

# 链接目录
link_directories(/usr/local/lib)

message(STATUS "Build agent server with HttpServer framework")
