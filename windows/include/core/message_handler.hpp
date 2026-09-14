#ifndef MESSAGE_HANDLER_HPP
#define MESSAGE_HANDLER_HPP

#include <string>
#include <functional>
#include <windows.h>
#include "message_types.hpp"

namespace remokey {

// 状态回调函数类型
using StatusCallback = std::function<void(const std::wstring&)>;

class MessageHandler {
public:
    MessageHandler();
    ~MessageHandler();

    // 设置状态回调
    void SetStatusCallback(StatusCallback cb);

    // 处理接收到的消息
    bool HandleMessage(const std::string& message);

    // 处理文本消息（供WebSocket直接调用）
    bool HandleText(const std::string& content);

    // 处理功能按键
    bool HandleKey(const std::string& content);

    // 处理系统指令
    bool HandleCommand(const std::string& content);

private:
    // 解析JSON消息
    bool ParseJson(const std::string& json, std::string& type, std::string& content);

    // 更新状态显示
    void UpdateStatus(const std::wstring& status);

    // 将UTF-8转换为UTF-16
    std::wstring Utf8ToWide(const std::string& utf8);

    StatusCallback statusCallback_;

    // 累计已发送的字符数（用于状态显示）
    size_t totalChars_ = 0;
};

} // namespace remokey

#endif // MESSAGE_HANDLER_HPP
