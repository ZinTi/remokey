#ifndef STATUS_WINDOW_HPP
#define STATUS_WINDOW_HPP

#include <windows.h>
#include <string>

namespace remokey {

class StatusWindow {
public:
    StatusWindow();
    ~StatusWindow();

    bool Create(HINSTANCE hInstance, int nCmdShow);

    void UpdateStatus(const std::wstring& status);

    HWND GetHandle() const { return hwnd_; }

    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    // 自定义消息：请求退出程序（由右键菜单触发）
    static constexpr UINT WM_REMOKEY_EXIT = WM_APP + 1;

    // 当前监听端口（由 main.cpp 设置，供"地址和端口"窗口使用）
    void SetListenPort(int port) { listenPort_ = port; }
    int  GetListenPort() const   { return listenPort_; }

private:
    bool RegisterClass(HINSTANCE hInstance);
    void Paint(HDC hdc);
    LRESULT HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    HWND hwnd_;
    HINSTANCE hInstance_;
    std::wstring statusText_;
    int listenPort_ = 8888;

    static constexpr wchar_t kClassName[]   = L"RemokeyStatusWindow";
    static constexpr wchar_t kWindowTitle[] = L"remokey";
    static constexpr int kWidth  = 180;
    static constexpr int kHeight = 36;

    static constexpr ULONG_PTR kCopyDataId = 1;
};

} // namespace remokey

#endif // STATUS_WINDOW_HPP
