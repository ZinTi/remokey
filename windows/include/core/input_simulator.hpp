#ifndef INPUT_SIMULATOR_HPP
#define INPUT_SIMULATOR_HPP

#include <windows.h>
#include <string>
#include <vector>
#include "message_types.hpp"

namespace remokey {

// 文本输入方式
enum class InputMode {
    POST_CHAR,   // 默认：PostMessage(WM_CHAR) 投递字符，绕过 IME 与键盘事件链
    SEND_INPUT   // SendInput(KEYEVENTF_UNICODE) 注入键盘事件，经过 IME
};

class InputSimulator {
public:
    // 设置 / 获取当前文本输入方式（默认 WM_CHAR）
    static void      SetInputMode(InputMode mode);
    static InputMode GetInputMode();

    // 发送Unicode文本
    static bool SendText(const std::wstring& text);
    
    // 发送功能按键
    static bool SendKey(KeyCode key);
    
    // 发送系统指令
    static bool SendCommand(CommandCode cmd);

private:
    // 构建Unicode输入数组
    static std::vector<INPUT> BuildUnicodeInputs(const std::wstring& text);
    
    // 发送输入数组
    static bool SendInputs(const std::vector<INPUT>& inputs);
    
    // 发送单个按键
    static bool SendSingleKey(WORD vk);
    
    // 发送组合键
    static bool SendHotkey(WORD vk, bool ctrl = false, bool shift = false, bool alt = false);

    // 文本发送的两种实现
    static bool SendTextViaWMChar(const std::wstring& text);
    static bool SendTextViaSendInput(const std::wstring& text);
};

} // namespace remokey

#endif // INPUT_SIMULATOR_HPP
