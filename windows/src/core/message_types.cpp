#include "message_types.hpp"

namespace remokey {

MessageType ParseMessageType(const std::string& type) {
    if (type == "text") return MessageType::TEXT;
    if (type == "key")  return MessageType::KEY;
    if (type == "cmd")  return MessageType::CMD;
    return MessageType::UNKNOWN;
}

KeyCode ParseKeyCode(const std::string& key) {
    if (key == "backspace") return KeyCode::BACKSPACE;
    if (key == "enter")     return KeyCode::ENTER;
    if (key == "tab")       return KeyCode::TAB;
    if (key == "left")      return KeyCode::LEFT;
    if (key == "right")     return KeyCode::RIGHT;
    if (key == "up")        return KeyCode::UP;
    if (key == "down")      return KeyCode::DOWN;
    if (key == "menu")      return KeyCode::MENU;
    return KeyCode::UNKNOWN;
}

CommandCode ParseCommandCode(const std::string& cmd) {
    if (cmd == "select_all") return CommandCode::SELECT_ALL;
    if (cmd == "copy")       return CommandCode::COPY;
    if (cmd == "cut")        return CommandCode::CUT;
    if (cmd == "paste")      return CommandCode::PASTE;
    if (cmd == "undo")       return CommandCode::UNDO;
    if (cmd == "redo")       return CommandCode::REDO;
    if (cmd == "clear")      return CommandCode::CLEAR;
    return CommandCode::UNKNOWN;
}

} // namespace remokey
