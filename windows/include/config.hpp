#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <string>

namespace remokey {

// 应用配置：从 exe 同目录下的 remokey_config.ini 读取
class Config {
public:
    static Config& Instance();

    // 从 exe 所在目录加载 remokey_config.ini
    void Load();

    const std::string& ListenAddress() const { return listenAddress_; }
    int                ListenPort()    const { return listenPort_; }

    // 文本输入方式（字符串形式）："wm_char"（默认）或 "sendinput"
    // 由 main.cpp 解析为 remokey::InputMode 并传给 InputSimulator
    const std::string& InputMode() const { return inputMode_; }

private:
    Config() = default;
    Config(const Config&) = delete;
    Config& operator=(const Config&) = delete;

    std::string listenAddress_ = "0.0.0.0";
    int         listenPort_    = 8888;
    std::string inputMode_     = "wm_char";   // 默认 WM_CHAR 模式
};

} // namespace remokey

#endif // CONFIG_HPP
