#include "main.hpp"
#include "ctrl/web_server.hpp"
#include "utils/path_utils.hpp"
#include "message_handler.hpp"
#include "status_window.hpp"
#include "config.hpp"
#include <windows.h>
#include <string>
#include <memory>
#include <cstdlib>

namespace {

std::unique_ptr<remokey::StatusWindow>   g_statusWindow;
std::unique_ptr<remokey::MessageHandler> g_messageHandler;

} // namespace

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int nCmdShow) {
    // 0. 加载配置
    remokey::Config::Instance().Load();
    const int listenPort = remokey::Config::Instance().ListenPort();

    // 1. 消息处理器
    g_messageHandler = std::make_unique<remokey::MessageHandler>();

    // 2. 状态窗口
    g_statusWindow = std::make_unique<remokey::StatusWindow>();
    if (!g_statusWindow->Create(hInstance, nCmdShow)) {
        MessageBoxW(nullptr, L"Failed to create status window", L"Error", MB_ICONERROR);
        return 1;
    }
    g_statusWindow->SetListenPort(listenPort);

    HWND hwnd = g_statusWindow->GetHandle();

    // 3. 状态回调
    g_messageHandler->SetStatusCallback([hwnd](const std::wstring& status) {
        COPYDATASTRUCT cds{};
        cds.dwData = 1;
        cds.cbData = static_cast<DWORD>((status.size() + 1) * sizeof(wchar_t));
        cds.lpData = const_cast<wchar_t*>(status.c_str());
        SendMessageW(hwnd, WM_COPYDATA, 0, reinterpret_cast<LPARAM>(&cds));
    });

    g_statusWindow->UpdateStatus(L"就绪");

    // 4. 启动 Web 服务器
    bool serverStarted = StartWebServer(
        hwnd,
        [](const std::string& message) {
            if (g_messageHandler) {
                g_messageHandler->HandleMessage(message);
            }
        },
        [hwnd](int port) {
            std::wstring msg = L"端口 " + std::to_wstring(port) +
                               L" 已被占用，请关闭占用该端口的程序后重新启动。";
            MessageBoxW(hwnd, msg.c_str(), L"remokey - 端口被占用",
                        MB_OK | MB_ICONWARNING);
        }
    );

    if (!serverStarted) {
        g_statusWindow.reset();
        g_messageHandler.reset();
        return 1;
    }

    // 5. 消息循环
    MSG msg{};
    while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    // 6. 隐藏窗口，避免 StopWebServer 阻塞期间"未响应"观感
    if (g_statusWindow) {
        HWND h = g_statusWindow->GetHandle();
        if (h) {
            ShowWindow(h, SW_HIDE);
            UpdateWindow(h);
        }
    }

    // 7. 停止 Web 服务器
    StopWebServer();

    // 8. 销毁窗口
    g_statusWindow.reset();
    g_messageHandler.reset();

    // 9. 强制退出：确保所有后台线程/资源都被进程终止回收
    //    避免 uWebSockets / libuv 内部残留线程导致进程驻留。
    std::exit(static_cast<int>(msg.wParam));
}
