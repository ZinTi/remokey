#include "status_window.hpp"
#include <dwmapi.h>
#include <string>
#include "context_menu.hpp"
#include "about_window.hpp"
#include "address_window.hpp"

namespace remokey {

StatusWindow::StatusWindow() : hwnd_(nullptr), hInstance_(nullptr) {
    statusText_ = L"就绪";
}

StatusWindow::~StatusWindow() {
    if (hwnd_) {
        DestroyWindow(hwnd_);
        hwnd_ = nullptr;
    }
}

bool StatusWindow::RegisterClass(HINSTANCE hInstance) {
    WNDCLASSEXW wc = {};
    wc.cbSize        = sizeof(wc);
    wc.style         = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = hInstance;
    wc.hCursor       = LoadCursorW(nullptr, IDC_ARROW);
    wc.hbrBackground = CreateSolidBrush(RGB(24, 24, 27));
    wc.lpszClassName = kClassName;

    return RegisterClassExW(&wc) != 0;
}

bool StatusWindow::Create(HINSTANCE hInstance, int nCmdShow) {
    hInstance_ = hInstance;

    if (!RegisterClass(hInstance)) {
        return false;
    }

    int x = 20;
    int y = 20;

    hwnd_ = CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TOOLWINDOW,
        kClassName,
        kWindowTitle,
        WS_POPUP,
        x, y,
        kWidth, kHeight,
        nullptr, nullptr, hInstance, this
    );

    if (!hwnd_) {
        return false;
    }

    SetLayeredWindowAttributes(hwnd_, 0, static_cast<BYTE>(200), LWA_ALPHA);

    DWM_WINDOW_CORNER_PREFERENCE preference = DWMWCP_ROUNDSMALL;
    DwmSetWindowAttribute(hwnd_, DWMWA_WINDOW_CORNER_PREFERENCE,
                          &preference, sizeof(preference));

    ShowWindow(hwnd_, nCmdShow);
    UpdateWindow(hwnd_);

    return true;
}

void StatusWindow::UpdateStatus(const std::wstring& status) {
    statusText_ = status;
    if (hwnd_) {
        InvalidateRect(hwnd_, nullptr, TRUE);
    }
}

LRESULT CALLBACK StatusWindow::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    StatusWindow* self = nullptr;

    if (msg == WM_NCCREATE) {
        auto* cs = reinterpret_cast<CREATESTRUCTW*>(lParam);
        self = static_cast<StatusWindow*>(cs->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
    } else {
        self = reinterpret_cast<StatusWindow*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    }

    if (self) {
        return self->HandleMessage(hwnd, msg, wParam, lParam);
    }

    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

LRESULT StatusWindow::HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            Paint(hdc);
            EndPaint(hwnd, &ps);
            return 0;
        }

        case WM_COPYDATA: {
            auto* cds = reinterpret_cast<COPYDATASTRUCT*>(lParam);
            if (cds && cds->dwData == kCopyDataId && cds->lpData && cds->cbData > 0) {
                const wchar_t* p = static_cast<const wchar_t*>(cds->lpData);
                size_t count = cds->cbData / sizeof(wchar_t);
                while (count > 0 && p[count - 1] == L'\0') --count;
                std::wstring text(p, count);

                constexpr size_t kMaxLen = 60;
                if (text.size() > kMaxLen) {
                    text = text.substr(0, kMaxLen) + L"…";
                }
                statusText_ = std::move(text);
                InvalidateRect(hwnd, nullptr, TRUE);
            }
            return TRUE;
        }

        case WM_LBUTTONDOWN: {
            ReleaseCapture();
            SendMessageW(hwnd, WM_NCLBUTTONDOWN, HTCAPTION, 0);
            return 0;
        }

        case WM_RBUTTONUP: {
            UINT cmd = ContextMenu::Show(hwnd);
            if (cmd == ContextMenu::kCmdAbout) {
                AboutWindow::Show(hwnd);
            } else if (cmd == ContextMenu::kCmdAddr) {
                AddressWindow::Show(hwnd, listenPort_);
            } else if (cmd == ContextMenu::kCmdExit) {
                PostMessageW(hwnd, WM_REMOKEY_EXIT, 0, 0);
            }
            return 0;
        }

        case WM_REMOKEY_EXIT: {
            // 先 PostQuitMessage 让主消息循环退出；
            // main.cpp 在退出后还会调用 exit(0) 彻底结束进程。
            PostQuitMessage(0);
            return 0;
        }

        case WM_DESTROY:
            return 0;
    }

    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

void StatusWindow::Paint(HDC hdc) {
    RECT rc;
    GetClientRect(hwnd_, &rc);

    HBRUSH bg = CreateSolidBrush(RGB(24, 24, 27));
    FillRect(hdc, &rc, bg);
    DeleteObject(bg);

    HFONT hFont = CreateFontW(
        -12, 0, 0, 0,
        FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY,
        DEFAULT_PITCH | FF_DONTCARE,
        L"Microsoft YaHei UI"
    );

    HGDIOBJ oldFont = SelectObject(hdc, hFont);

    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, RGB(220, 220, 225));

    RECT textRc = rc;
    textRc.left   += 10;
    textRc.right  -= 10;
    textRc.top    += 6;
    textRc.bottom -= 6;

    const wchar_t* displayText =
        statusText_.empty() ? L"就绪" : statusText_.c_str();

    DrawTextW(hdc, displayText, -1, &textRc,
              DT_LEFT | DT_TOP | DT_SINGLELINE | DT_END_ELLIPSIS);

    SelectObject(hdc, oldFont);
    DeleteObject(hFont);
}

} // namespace remokey
