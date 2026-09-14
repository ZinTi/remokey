#ifndef CONTEXT_MENU_HPP
#define CONTEXT_MENU_HPP

#include <windows.h>

namespace remokey {

// 右键菜单：在光标处弹出，返回用户选择的命令 ID；未选择返回 0
class ContextMenu {
public:
    // 命令 ID
    static constexpr UINT kCmdAbout   = 1000;
    static constexpr UINT kCmdAddr    = 1001;
    static constexpr UINT kCmdExit    = 1002;

    // 在光标位置弹出菜单，返回选中的命令 ID
    static UINT Show(HWND owner);
};

} // namespace remokey

#endif // CONTEXT_MENU_HPP
