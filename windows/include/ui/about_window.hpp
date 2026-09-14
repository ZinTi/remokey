#ifndef ABOUT_WINDOW_HPP
#define ABOUT_WINDOW_HPP

#include <windows.h>

namespace remokey {

// 简单的"关于"对话框窗口
class AboutWindow {
public:
    // 以 owner 为父窗口弹出模态关于框
    static void Show(HWND owner);

private:
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    static bool EnsureClassRegistered(HINSTANCE hInstance);

    static constexpr wchar_t kClassName[]  = L"RemokeyAboutWindow";
    static constexpr wchar_t kWindowTitle[] = L"ABOUT 关于";
    static constexpr int kWidth  = 300;
    static constexpr int kHeight = 160;
    static constexpr int kCmdOk  = 1;
};

} // namespace remokey

#endif // ABOUT_WINDOW_HPP
