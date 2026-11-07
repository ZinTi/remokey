#include "input_simulator.hpp"
#include <cstring>
#include <atomic>

namespace remokey {

namespace {

// 当前文本输入方式。
// 用 atomic<int> 而非 atomic<InputMode>：enum class 底层类型在某些编译器版本
// 上作为模板参数支持度不一，转成 int 存储绝对安全。
// 用 atomic 是因为 Web 服务器回调线程与主线程都可能调用 SendText。
std::atomic<int> g_inputMode{static_cast<int>(InputMode::POST_CHAR)};

// 获取前台窗口真正的焦点控件（跨进程可用）
HWND GetFocusWindowImpl() {
    HWND hwnd = GetForegroundWindow();
    if (!hwnd) return nullptr;

    DWORD tid = GetWindowThreadProcessId(hwnd, nullptr);
    if (tid == 0) return hwnd;

    GUITHREADINFO gti{};
    gti.cbSize = sizeof(gti);
    if (GetGUIThreadInfo(tid, &gti) && gti.hwndFocus) {
        return gti.hwndFocus;
    }
    return hwnd;
}

} // namespace

void InputSimulator::SetInputMode(InputMode mode) {
    g_inputMode.store(static_cast<int>(mode), std::memory_order_relaxed);
}

InputMode InputSimulator::GetInputMode() {
    return static_cast<InputMode>(g_inputMode.load(std::memory_order_relaxed));
}

bool InputSimulator::SendText(const std::wstring& text) {
    if (text.empty()) return false;

    if (GetInputMode() == InputMode::POST_CHAR) {
        return SendTextViaWMChar(text);
    }
    return SendTextViaSendInput(text);
}

// ---------------------------------------------------------------------------
// WM_CHAR 模式：把成型字符直接投递到焦点控件，绕过 IME 与键盘事件链
//   适用：Chromium / CEF / 浏览器 / Notepad 等文本编辑场景
//   不触发 keydown / keypress，因此输入联想/实时校验可能不工作
//   不受 IME 影响：无论搜狗、微软拼音还是英文输入法，字符原样送达
// ---------------------------------------------------------------------------
bool InputSimulator::SendTextViaWMChar(const std::wstring& text) {
    HWND hFocus = GetFocusWindowImpl();
    if (!hFocus) return false;

    for (wchar_t ch : text) {
        // 编辑控件的标准换行是 \r，统一转换
        if (ch == L'\n') ch = L'\r';
        // PostMessage 异步投递，FIFO 严格保序，天然不阻塞发送线程
        PostMessageW(hFocus, WM_CHAR, static_cast<WPARAM>(ch), 1);
    }

    // 栅栏：等待目标窗口处理完前面所有 PostMessage 消息。
    // 避免紧接着的 SendKey(ENTER) / SendCommand 走 SendInput 时出现
    // "字符还堵在目标队列里、回车已经先到" 的顺序错乱。
    // 使用 SendMessageTimeout 而非 SendMessage：目标无响应时 200ms 内超时返回，
    // 不会让 remokey 一直卡死。
    DWORD_PTR result = 0;
    SendMessageTimeoutW(hFocus, WM_NULL, 0, 0,
                        SMTO_ABORTIFHUNG | SMTO_BLOCK, 200, &result);

    return true;
}

// ---------------------------------------------------------------------------
// SendInput 模式：通过 KEYEVENTF_UNICODE 注入键盘事件，经过 IME 处理
//   适用：需要触发完整事件链（keydown/keypress/keyup）的场景、
//         游戏/自绘控件（WM_CHAR 对其无效）等
//   风险：微软拼音在 TSF 客户端（如 Chromium）上会把注入的码点当成
//         composition 输入，导致丢字或汉字被替换为标点
// ---------------------------------------------------------------------------
bool InputSimulator::SendTextViaSendInput(const std::wstring& text) {
    auto inputs = BuildUnicodeInputs(text);
    return SendInputs(inputs);
}

// ---------------------------------------------------------------------------
// 以下为原有实现，保持不变
// ---------------------------------------------------------------------------

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
