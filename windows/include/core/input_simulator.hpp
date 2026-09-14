#ifndef INPUT_SIMULATOR_HPP
#define INPUT_SIMULATOR_HPP

#include <windows.h>
#include <string>
#include <vector>
#include "message_types.hpp"

namespace remokey {

class InputSimulator {
public:
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
};

} // namespace remokey

#endif // INPUT_SIMULATOR_HPP
