#include "config.hpp"
#include "utils/path_utils.hpp"
#include <windows.h>
#include <string>
#include <algorithm>
#include <cctype>

namespace remokey {

namespace {

std::string Trim(const std::string& s) {
    size_t b = 0, e = s.size();
    while (b < e && std::isspace(static_cast<unsigned char>(s[b]))) ++b;
    while (e > b && std::isspace(static_cast<unsigned char>(s[e - 1]))) --e;
    return s.substr(b, e - b);
}

// 由配置解析直接使用，不能再标 [[maybe_unused]]
std::string ToLower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return s;
}

// 读取 ini 中 [section] 下的 key，返回 true 表示找到
bool ReadIniString(const std::wstring& filePath,
                   const std::wstring& section,
                   const std::wstring& key,
                   std::wstring& out) {
    std::wstring buf(4096, L'\0');
    DWORD n = GetPrivateProfileStringW(
        section.c_str(), key.c_str(), L"",
        buf.data(), static_cast<DWORD>(buf.size()),
        filePath.c_str());
    if (n == 0) return false;
    out.assign(buf.data(), n);
    return true;
}

} // namespace

Config& Config::Instance() {
    static Config instance;
    return instance;
}

void Config::Load() {
    // 重置为默认值
    listenAddress_ = "0.0.0.0";
    listenPort_    = 8888;
    inputMode_     = "wm_char";

    std::wstring iniPath = utils::JoinExeDir(L"remokey_config.ini");

    // -------------------- [server] address --------------------
    std::wstring waddr;
    if (ReadIniString(iniPath, L"server", L"address", waddr)) {
        std::string addr(waddr.begin(), waddr.end());
        addr = Trim(addr);
        if (!addr.empty()) {
            listenAddress_ = addr;
        }
    }

    // -------------------- [server] port --------------------
    std::wstring wport;
    if (ReadIniString(iniPath, L"server", L"port", wport)) {
        try {
            int port = std::stoi(wport);
            if (port > 0 && port <= 65535) {
                listenPort_ = port;
            }
        } catch (...) {
            // 保持默认值
        }
    }

    // -------------------- [input] mode --------------------
    // 只接受 "wm_char" 和 "sendinput" 两个值（大小写不敏感）；
    // 其他值（含键不存在）一律保持默认 "wm_char"。
    std::wstring wmode;
    if (ReadIniString(iniPath, L"input", L"mode", wmode)) {
        std::string mode(wmode.begin(), wmode.end());
        mode = ToLower(Trim(mode));
        if (mode == "wm_char" || mode == "sendinput") {
            inputMode_ = mode;
        }
        // 其他值忽略，保持默认值
    }
}

} // namespace remokey
