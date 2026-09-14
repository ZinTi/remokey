#include "about_window.hpp"
#include <string>

namespace remokey {

namespace {

constexpr const wchar_t* kAboutText =
    L"Remokey v0.1.0\n\n"
    L"远程键盘输入。";

} // namespace

bool AboutWindow::EnsureClassRegistered(HINSTANCE hInstance) {
    static bool registered = false;
    if (registered) return true;

    WNDCLASSEXW wc = {};
    wc.cbSize        = sizeof(wc);
    wc.style         = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = hInstance;
    wc.hCursor       = LoadCursorW(nullptr, IDC_ARROW);
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_BTNFACE + 1);
    wc.lpszClassName = kClassName;

    registered = RegisterClassExW(&wc) != 0;
    return registered;
}

void AboutWindow::Show(HWND owner) {
    HINSTANCE hInstance = reinterpret_cast<HINSTANCE>(
        GetWindowLongPtrW(owner, GWLP_HINSTANCE));
    if (!EnsureClassRegistered(hInstance)) return;

    RECT ownerRc{};
    GetWindowRect(owner, &ownerRc);

    // 默认居中于 owner
    int x = ownerRc.left + ((ownerRc.right - ownerRc.left) - kWidth) / 2;
    int y = ownerRc.top  + ((ownerRc.bottom - ownerRc.top) - kHeight) / 2;

    // -----------------------------------------------------------------------
    // 钳制到工作区（排除任务栏），确保标题栏始终可见、可拖动。
    // 优先取 owner 所在显示器的工作区；失败时回退到主显示器工作区。
    // -----------------------------------------------------------------------
    HMONITOR hMon = MonitorFromWindow(owner, MONITOR_DEFAULTTONEAREST);
    MONITORINFO mi{};
    mi.cbSize = sizeof(mi);
    RECT work{};
    if (hMon && GetMonitorInfoW(hMon, &mi)) {
        work = mi.rcWork;
    } else {
        SystemParametersInfoW(SPI_GETWORKAREA, 0, &work, 0);
    }

    int maxX = work.right  - kWidth;
    int maxY = work.bottom - kHeight;
    if (maxX < work.left) maxX = work.left;   // 窗口比工作区还宽时至少贴左
    if (maxY < work.top)  maxY = work.top;    // 窗口比工作区还高时至少贴上

    if (x < work.left) x = work.left;
    if (y < work.top)  y = work.top;
    if (x > maxX)      x = maxX;
    if (y > maxY)      y = maxY;

    HWND hwnd = CreateWindowExW(
        WS_EX_DLGMODALFRAME,
        kClassName,
        kWindowTitle,
        WS_CAPTION | WS_SYSMENU | WS_POPUP,
        x, y, kWidth, kHeight,
        owner, nullptr, hInstance, nullptr);

    if (!hwnd) return;

    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);

    EnableWindow(owner, FALSE);
    MSG msg{};
    while (IsWindow(hwnd) && GetMessageW(&msg, nullptr, 0, 0) > 0) {
        if (!IsDialogMessageW(hwnd, &msg)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }
    EnableWindow(owner, TRUE);
    SetActiveWindow(owner);
}

LRESULT CALLBACK AboutWindow::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);

            HFONT hFont = CreateFontW(
                -13, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
                L"Microsoft YaHei UI");
            HGDIOBJ oldFont = SelectObject(hdc, hFont);
            SetBkMode(hdc, TRANSPARENT);

            RECT rc = { 16, 16, kWidth - 16, kHeight - 16 };
            DrawTextW(hdc, kAboutText, -1, &rc,
                      DT_LEFT | DT_TOP | DT_WORDBREAK | DT_NOPREFIX);

            SelectObject(hdc, oldFont);
            DeleteObject(hFont);
            EndPaint(hwnd, &ps);
            return 0;
        }

        case WM_CLOSE:
            DestroyWindow(hwnd);
            return 0;

        case WM_DESTROY:
            return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

} // namespace remokey
