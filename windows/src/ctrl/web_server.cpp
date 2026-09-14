// web_server.cpp
#include "ctrl/web_server.hpp"
#include "utils/path_utils.hpp"
#include "config.hpp"
#include <App.h>
#include <thread>
#include <atomic>
#include <string>
#include <string_view>
#include <cstdio>
#include <iostream>
#include <memory>
#include <utility>
#include <mutex>
#include <winsock2.h>
#include <ws2tcpip.h>

static HWND g_hwnd = nullptr;
static std::thread g_thread;
static std::atomic<bool> g_running{false};
static MessageCallback g_messageCallback;
static us_listen_socket_t* g_listenSocket = nullptr;
static us_loop_t* g_loop = nullptr;
static std::mutex g_loopMutex;

static std::pair<std::string, std::string> ReadStaticFile(const std::string& urlPath) {
    std::string relPath = urlPath;
    if (relPath.empty() || relPath == "/") {
        relPath = "/index.html";
    }
    if (!relPath.empty() && relPath.front() == '/') {
        relPath = relPath.substr(1);
    }

    std::wstring wrelPath(relPath.begin(), relPath.end());
    for (auto& c : wrelPath) {
        if (c == L'/') c = L'\\';
    }

    std::wstring fullPath = utils::JoinExeDir(L"www\\" + wrelPath);

    std::string mime = "application/octet-stream";
    if (relPath.find(".html") != std::string::npos) {
        mime = "text/html; charset=UTF-8";
    } else if (relPath.find(".js") != std::string::npos) {
        mime = "application/javascript; charset=UTF-8";
    } else if (relPath.find(".css") != std::string::npos) {
        mime = "text/css; charset=UTF-8";
    } else if (relPath.find(".json") != std::string::npos) {
        mime = "application/json; charset=UTF-8";
    } else if (relPath.find(".png") != std::string::npos) {
        mime = "image/png";
    } else if (relPath.find(".ico") != std::string::npos) {
        mime = "image/x-icon";
    }

    FILE* fp = _wfopen(fullPath.c_str(), L"rb");
    if (!fp) return {"", "text/plain"};

    std::string content;
    char buf[4096];
    size_t n;
    while ((n = fread(buf, 1, sizeof(buf), fp)) > 0) {
        content.append(buf, n);
    }
    fclose(fp);

    return {content, mime};
}

static void NotifyMessage(const std::string& message) {
    if (g_messageCallback) {
        g_messageCallback(message);
    }
}

// ---------------------------------------------------------------------------
// 检测指定地址和端口是否可绑定（同步检测，立即返回）
// ---------------------------------------------------------------------------
static bool IsPortAvailable(const std::string& address, int port) {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        return false;
    }

    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) {
        WSACleanup();
        return false;
    }

    // 设置 SO_REUSEADDR 不影响检测：我们要检测的是是否有其他进程正在监听
    // 不设置 SO_REUSEADDR，确保检测结果反映真实占用情况
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port   = htons(static_cast<u_short>(port));

    if (address.empty() || address == "0.0.0.0") {
        addr.sin_addr.s_addr = INADDR_ANY;
    } else {
        if (inet_pton(AF_INET, address.c_str(), &addr.sin_addr) != 1) {
            // 地址解析失败，回退到 INADDR_ANY
            addr.sin_addr.s_addr = INADDR_ANY;
        }
    }

    bool available = false;
    if (bind(sock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == 0) {
        available = true;
    }

    closesocket(sock);
    WSACleanup();
    return available;
}

bool StartWebServer(HWND mainHwnd, MessageCallback onMessage, PortBusyCallback onPortBusy) {
    g_hwnd = mainHwnd;
    g_messageCallback = std::move(onMessage);
    g_running = true;

    // 从配置文件读取监听地址与端口
    const auto& cfg = remokey::Config::Instance();
    const std::string listenAddr = cfg.ListenAddress();
    const int         listenPort = cfg.ListenPort();

    // 同步检测端口是否可用
    if (!IsPortAvailable(listenAddr, listenPort)) {
        g_running = false;
        if (onPortBusy) {
            onPortBusy(listenPort);
        }
        return false;
    }

    g_thread = std::thread([listenAddr, listenPort]() {
        uWS::App()
            .get("/*", [](auto* res, auto* req) {
                std::string path(req->getUrl());
                auto [content, mime] = ReadStaticFile(path);
                if (content.empty()) {
                    res->writeStatus("404 Not Found")
                       ->writeHeader("Content-Type", "text/plain")
                       ->end("404: File not found");
                } else {
                    res->writeStatus("200 OK")
                       ->writeHeader("Content-Type", mime)
                       ->end(content);
                }
            })

            .ws<std::string>("/ws", {
                .open = [](auto* /*ws*/) {
                    std::cout << "[ws] client connected" << std::endl;
                },
                .message = [](auto* /*ws*/, std::string_view msg, uWS::OpCode) {
                    NotifyMessage(std::string(msg));
                },
                .close = [](auto* /*ws*/, int code, std::string_view) {
                    std::cout << "[ws] client disconnected, code=" << code << std::endl;
                }
            })

            .post("/send", [](auto* res, auto* /*req*/) {
                auto buffer = std::make_shared<std::string>();
                res->onData([res, buffer](std::string_view chunk, bool last) {
                    buffer->append(chunk.data(), chunk.size());
                    if (last) {
                        NotifyMessage(*buffer);
                        res->writeStatus("200 OK")
                           ->writeHeader("Content-Type", "text/plain")
                           ->end("ok");
                    }
                });
                res->onAborted([]() {});
            })

            .listen(listenAddr, listenPort, [listenAddr, listenPort](auto* token) {
                std::lock_guard<std::mutex> lock(g_loopMutex);
                g_listenSocket = token;
                if (token) {
                    g_loop = us_socket_context_loop(0, us_socket_context(0, (us_socket_t*)token));
                    std::cout << "Web server listening on http://"
                              << listenAddr << ":" << listenPort << std::endl;
                } else {
                    std::cerr << "Failed to listen on "
                              << listenAddr << ":" << listenPort << std::endl;
                }
            })

            .run();
    });

    return true;
}

void StopWebServer() {
    g_running = false;

    us_loop_t* loop = nullptr;
    us_listen_socket_t* socket = nullptr;
    {
        std::lock_guard<std::mutex> lock(g_loopMutex);
        loop = g_loop;
        socket = g_listenSocket;
        g_listenSocket = nullptr;
        g_loop = nullptr;
    }

    if (loop && socket) {
        us_wakeup_loop(loop);
        us_listen_socket_close(0, socket);
    }

    if (g_thread.joinable()) {
        g_thread.join();
    }
}
