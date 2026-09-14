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

private:
    Config() = default;
    Config(const Config&) = delete;
    Config& operator=(const Config&) = delete;

    std::string listenAddress_ = "0.0.0.0";
    int         listenPort_    = 8888;
};

} // namespace remokey

#endif // CONFIG_HPP
