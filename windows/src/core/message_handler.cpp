#include "message_handler.hpp"
#include "input_simulator.hpp"
#include <sstream>
#include <cctype>
#include <string>

namespace remokey {

MessageHandler::MessageHandler() = default;
MessageHandler::~MessageHandler() = default;

void MessageHandler::SetStatusCallback(StatusCallback cb) {
    statusCallback_ = std::move(cb);
}

bool MessageHandler::HandleMessage(const std::string& message) {
    std::string type, content;

    if (!ParseJson(message, type, content)) {
        return HandleText(message);
    }

    switch (ParseMessageType(type)) {
        case MessageType::TEXT: return HandleText(content);
        case MessageType::KEY:  return HandleKey(content);
        case MessageType::CMD:  return HandleCommand(content);
        default:
            UpdateStatus(L"未知消息类型");
            return false;
    }
}

bool MessageHandler::HandleText(const std::string& content) {
    std::wstring wtext = Utf8ToWide(content);
    if (wtext.empty()) return false;

    // 累计字数（以 Unicode 码点计，粗略用 wstring 长度）
    totalChars_ += wtext.size();

    // 完全避开 swprintf 的格式化陷阱，用 wstring 拼接，杜绝乱码
    std::wstring status = L"已输入 " + std::to_wstring(totalChars_) + L" 字";
    UpdateStatus(status);

    return InputSimulator::SendText(wtext);
}

bool MessageHandler::HandleKey(const std::string& content) {
    KeyCode key = ParseKeyCode(content);
    if (key == KeyCode::UNKNOWN) {
        UpdateStatus(L"未知按键");
        return false;
    }

    static const wchar_t* kNames[] = {
        L"BACKSPACE", L"ENTER", L"TAB",
        L"LEFT", L"RIGHT", L"UP", L"DOWN",
        L"MENU"
    };

    int idx = static_cast<int>(key);
    if (idx >= 0 && idx < static_cast<int>(sizeof(kNames) / sizeof(kNames[0]))) {
        // 只显示操作本身，不含字数
        UpdateStatus(std::wstring(L"Key: ") + kNames[idx]);
    }

    return InputSimulator::SendKey(key);
}

bool MessageHandler::HandleCommand(const std::string& content) {
    CommandCode cmd = ParseCommandCode(content);
    if (cmd == CommandCode::UNKNOWN) {
        UpdateStatus(L"未知指令");
        return false;
    }

    static const wchar_t* kNames[] = {
        L"全选", L"复制", L"剪切", L"粘贴", L"撤销", L"重做", L"清空"
    };

    int idx = static_cast<int>(cmd);
    if (idx >= 0 && idx < static_cast<int>(sizeof(kNames) / sizeof(kNames[0]))) {
        // 只显示操作本身，不含字数
        UpdateStatus(std::wstring(L"Command: ") + kNames[idx]);
    }

    // 清空指令后重置计数
    if (cmd == CommandCode::CLEAR) {
        totalChars_ = 0;
    }

    return InputSimulator::SendCommand(cmd);
}

// ---------------------------------------------------------------
// 简单的 JSON 解析：只支持 {"type":"xxx","content":"yyy"} 形式
// 支持字符串值中的转义：\" \\ \n \r \t \/ \uXXXX
// ---------------------------------------------------------------

namespace {

bool ParseJsonString(const std::string& s, size_t& pos, std::string& out) {
    if (pos >= s.size() || s[pos] != '"') return false;
    ++pos;
    out.clear();

    while (pos < s.size()) {
        char c = s[pos];
        if (c == '"') { ++pos; return true; }
        if (c == '\\') {
            ++pos;
            if (pos >= s.size()) return false;
            char e = s[pos++];
            switch (e) {
                case '"':  out.push_back('"');  break;
                case '\\': out.push_back('\\'); break;
                case '/':  out.push_back('/');  break;
                case 'b':  out.push_back('\b'); break;
                case 'f':  out.push_back('\f'); break;
                case 'n':  out.push_back('\n'); break;
                case 'r':  out.push_back('\r'); break;
                case 't':  out.push_back('\t'); break;
                case 'u': {
                    if (pos + 4 > s.size()) return false;
                    unsigned cp = 0;
                    for (int i = 0; i < 4; ++i) {
                        char h = s[pos++];
                        cp <<= 4;
                        if (h >= '0' && h <= '9')      cp |= (h - '0');
                        else if (h >= 'a' && h <= 'f') cp |= (h - 'a' + 10);
                        else if (h >= 'A' && h <= 'F') cp |= (h - 'A' + 10);
                        else return false;
                    }
                    if (cp < 0x80) {
                        out.push_back(static_cast<char>(cp));
                    } else if (cp < 0x800) {
                        out.push_back(static_cast<char>(0xC0 | (cp >> 6)));
                        out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
                    } else {
                        out.push_back(static_cast<char>(0xE0 | (cp >> 12)));
                        out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
                        out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
                    }
                    break;
                }
                default: out.push_back(e); break;
            }
        } else {
            out.push_back(c);
            ++pos;
        }
    }
    return false;
}

void SkipWs(const std::string& s, size_t& pos) {
    while (pos < s.size() && std::isspace(static_cast<unsigned char>(s[pos]))) {
        ++pos;
    }
}

} // namespace

bool MessageHandler::ParseJson(const std::string& json, std::string& type, std::string& content) {
    size_t pos = 0;
    SkipWs(json, pos);
    if (pos >= json.size() || json[pos] != '{') return false;
    ++pos;

    type.clear();
    content.clear();

    bool gotType = false;

    while (true) {
        SkipWs(json, pos);
        if (pos >= json.size()) return false;
        if (json[pos] == '}') { ++pos; break; }
        if (json[pos] == ',') { ++pos; continue; }
        if (json[pos] != '"') return false;

        std::string key;
        if (!ParseJsonString(json, pos, key)) return false;

        SkipWs(json, pos);
        if (pos >= json.size() || json[pos] != ':') return false;
        ++pos;
        SkipWs(json, pos);

        if (pos < json.size() && json[pos] == '"') {
            std::string val;
            if (!ParseJsonString(json, pos, val)) return false;
            if (key == "type")         { type = std::move(val); gotType = true; }
            else if (key == "content") { content = std::move(val); }
        } else {
            while (pos < json.size() && json[pos] != ',' && json[pos] != '}') {
                ++pos;
            }
        }
    }

    return gotType;
}

void MessageHandler::UpdateStatus(const std::wstring& status) {
    if (statusCallback_) {
        statusCallback_(status);
    }
}

std::wstring MessageHandler::Utf8ToWide(const std::string& utf8) {
    if (utf8.empty()) return L"";

    int wlen = MultiByteToWideChar(
        CP_UTF8, 0,
        utf8.c_str(), static_cast<int>(utf8.size()),
        nullptr, 0
    );

    if (wlen <= 0) return L"";

    std::wstring wtext(wlen, L'\0');
    MultiByteToWideChar(
        CP_UTF8, 0,
        utf8.c_str(), static_cast<int>(utf8.size()),
        wtext.data(), wlen
    );

    return wtext;
}

} // namespace remokey
