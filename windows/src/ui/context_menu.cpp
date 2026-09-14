#include "context_menu.hpp"

namespace remokey {

UINT ContextMenu::Show(HWND owner) {
    HMENU hMenu = CreatePopupMenu();
    if (!hMenu) return 0;

    AppendMenuW(hMenu, MF_STRING, kCmdAbout, L"ABOUT 关于");
    AppendMenuW(hMenu, MF_STRING, kCmdAddr,  L"地址和端口");
    AppendMenuW(hMenu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(hMenu, MF_STRING, kCmdExit,  L"EXIT 退出");

    POINT pt;
    GetCursorPos(&pt);

    SetForegroundWindow(owner);

    UINT cmd = TrackPopupMenu(
        hMenu,
        TPM_RIGHTBUTTON | TPM_RETURNCMD | TPM_BOTTOMALIGN | TPM_LEFTALIGN,
        pt.x, pt.y,
        0, owner, nullptr
    );

    DestroyMenu(hMenu);

    PostMessageW(owner, WM_NULL, 0, 0);

    return cmd;
}

} // namespace remokey
