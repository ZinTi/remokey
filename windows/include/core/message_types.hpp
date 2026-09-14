#ifndef MESSAGE_TYPES_HPP
#define MESSAGE_TYPES_HPP

#include <string>
#include <vector>
#include <cstdint>

namespace remokey {

// 消息类型枚举
enum class MessageType : uint8_t {
    TEXT = 0,      // 普通文本
    KEY = 1,       // 功能按键
    CMD = 2,       // 系统指令
    UNKNOWN = 255
};

// 功能按键枚举
enum class KeyCode : uint8_t {
    BACKSPACE = 0,
    ENTER     = 1,
    TAB       = 2,
    LEFT      = 3,
    RIGHT     = 4,
    UP        = 5,
    DOWN      = 6,
    MENU      = 7,   // 右键菜单键（VK_APPS）
    UNKNOWN   = 255
};

// 系统指令枚举
enum class CommandCode : uint8_t {
    SELECT_ALL = 0,
    COPY       = 1,
    CUT        = 2,
    PASTE      = 3,
    UNDO       = 4,
    REDO       = 5,
    CLEAR      = 6,
    UNKNOWN    = 255
};

// 解析消息类型
MessageType ParseMessageType(const std::string& type);

// 解析功能按键
KeyCode ParseKeyCode(const std::string& key);

// 解析系统指令
CommandCode ParseCommandCode(const std::string& cmd);

} // namespace remokey

#endif // MESSAGE_TYPES_HPP
