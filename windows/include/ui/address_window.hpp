#ifndef ADDRESS_WINDOW_HPP
#define ADDRESS_WINDOW_HPP

#include <windows.h>
#include <string>
#include <vector>
#include "ip_helper.hpp"

namespace remokey {

// "地址和端口"窗口：显示本机 IPv4 + 监听端口，并绘制二维码。
class AddressWindow {
public:
    // 以 owner 为父窗口弹出模态窗口
    // listenPort 为当前监听端口
    static void Show(HWND owner, int listenPort);

private:
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    static bool EnsureClassRegistered(HINSTANCE hInstance);

    // 实例状态
    struct State {
        std::vector<LocalAddress> addresses;
        int   port = 8888;
        int   selectedIndex = 0;
        HWND  hCombo = nullptr;
        HWND  hJsonEdit = nullptr;   // 只读多行文本框，承载 JSON 原文
    };

    // 绘制真实二维码
    static void DrawQrCode(HDC hdc, const RECT& rc, const std::wstring& payload);

    // 构建 JSON payload
    static std::string BuildJsonPayload(const std::string& ip, int port);

    // 将 UTF-8 转换为 UTF-16
    static std::wstring Utf8ToWide(const std::string& utf8);

    // 更新 JSON 文本框与二维码重绘
    static void RefreshPayload(HWND hwnd, State* st);

    static constexpr wchar_t kClassName[]   = L"RemokeyAddressWindow";
    static constexpr wchar_t kWindowTitle[] = L"地址和端口";
    static constexpr int kWidth  = 420;
    static constexpr int kHeight = 400;

    static constexpr int kIdCombo    = 2001;
    static constexpr int kIdJsonEdit = 2002;
};

} // namespace remokey

#endif // ADDRESS_WINDOW_HPP
