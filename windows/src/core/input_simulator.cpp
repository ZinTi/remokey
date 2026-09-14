#include "input_simulator.hpp"
#include <cstring>

namespace remokey {

bool InputSimulator::SendText(const std::wstring& text) {
    if (text.empty()) return false;
    
    auto inputs = BuildUnicodeInputs(text);
    return SendInputs(inputs);
}

bool InputSimulator::SendKey(KeyCode key) {
    switch (key) {
        case KeyCode::BACKSPACE: return SendSingleKey(VK_BACK);
        case KeyCode::ENTER:     return SendSingleKey(VK_RETURN);
        case KeyCode::TAB:       return SendSingleKey(VK_TAB);
        case KeyCode::LEFT:      return SendSingleKey(VK_LEFT);
        case KeyCode::RIGHT:     return SendSingleKey(VK_RIGHT);
        case KeyCode::UP:        return SendSingleKey(VK_UP);
        case KeyCode::DOWN:      return SendSingleKey(VK_DOWN);
        case KeyCode::MENU:      return SendSingleKey(VK_APPS);
        default:                 return false;
    }
}

bool InputSimulator::SendCommand(CommandCode cmd) {
    switch (cmd) {
        case CommandCode::SELECT_ALL:
            return SendHotkey('A', true, false, false);
        case CommandCode::COPY:
            return SendHotkey('C', true, false, false);
        case CommandCode::CUT:
            return SendHotkey('X', true, false, false);
        case CommandCode::PASTE:
            return SendHotkey('V', true, false, false);
        case CommandCode::UNDO:
            return SendHotkey('Z', true, false, false);
        case CommandCode::REDO:
            return SendHotkey('Y', true, false, false);
        case CommandCode::CLEAR:
            return SendHotkey('A', true, false, false) && SendSingleKey(VK_DELETE);
        default:
            return false;
    }
}

std::vector<INPUT> InputSimulator::BuildUnicodeInputs(const std::wstring& text) {
    std::vector<INPUT> inputs;
    inputs.reserve(text.size() * 2);
    
    for (wchar_t ch : text) {
        INPUT inputDown = {};
        inputDown.type       = INPUT_KEYBOARD;
        inputDown.ki.wVk     = 0;
        inputDown.ki.wScan   = static_cast<WORD>(ch);
        inputDown.ki.dwFlags = KEYEVENTF_UNICODE;
        inputDown.ki.time    = 0;
        inputDown.ki.dwExtraInfo = 0;
        inputs.push_back(inputDown);
        
        INPUT inputUp = inputDown;
        inputUp.ki.dwFlags |= KEYEVENTF_KEYUP;
        inputs.push_back(inputUp);
    }
    
    return inputs;
}

bool InputSimulator::SendInputs(const std::vector<INPUT>& inputs) {
    if (inputs.empty()) return false;
    
    UINT sent = SendInput(
        static_cast<UINT>(inputs.size()),
        const_cast<INPUT*>(inputs.data()),
        sizeof(INPUT)
    );
    
    return sent == inputs.size();
}

bool InputSimulator::SendSingleKey(WORD vk) {
    INPUT inputs[2] = {};
    
    inputs[0].type    = INPUT_KEYBOARD;
    inputs[0].ki.wVk  = vk;
    inputs[0].ki.dwFlags = 0;
    
    inputs[1].type    = INPUT_KEYBOARD;
    inputs[1].ki.wVk  = vk;
    inputs[1].ki.dwFlags = KEYEVENTF_KEYUP;
    
    return SendInput(2, inputs, sizeof(INPUT)) == 2;
}

bool InputSimulator::SendHotkey(WORD vk, bool ctrl, bool shift, bool alt) {
    std::vector<INPUT> inputs;
    
    if (ctrl) {
        INPUT in = {};
        in.type = INPUT_KEYBOARD;
        in.ki.wVk = VK_CONTROL;
        inputs.push_back(in);
    }
    if (shift) {
        INPUT in = {};
        in.type = INPUT_KEYBOARD;
        in.ki.wVk = VK_SHIFT;
        inputs.push_back(in);
    }
    if (alt) {
        INPUT in = {};
        in.type = INPUT_KEYBOARD;
        in.ki.wVk = VK_MENU;
        inputs.push_back(in);
    }
    
    INPUT keyDown = {};
    keyDown.type = INPUT_KEYBOARD;
    keyDown.ki.wVk = vk;
    inputs.push_back(keyDown);
    
    INPUT keyUp = keyDown;
    keyUp.ki.dwFlags = KEYEVENTF_KEYUP;
    inputs.push_back(keyUp);
    
    if (alt) {
        INPUT in = {};
        in.type = INPUT_KEYBOARD;
        in.ki.wVk = VK_MENU;
        in.ki.dwFlags = KEYEVENTF_KEYUP;
        inputs.push_back(in);
    }
    if (shift) {
        INPUT in = {};
        in.type = INPUT_KEYBOARD;
        in.ki.wVk = VK_SHIFT;
        in.ki.dwFlags = KEYEVENTF_KEYUP;
        inputs.push_back(in);
    }
    if (ctrl) {
        INPUT in = {};
        in.type = INPUT_KEYBOARD;
        in.ki.wVk = VK_CONTROL;
        in.ki.dwFlags = KEYEVENTF_KEYUP;
        inputs.push_back(in);
    }
    
    return SendInputs(inputs);
}

} // namespace remokey
