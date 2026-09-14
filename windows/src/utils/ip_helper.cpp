#include "ip_helper.hpp"

#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <windows.h>

#include <algorithm>
#include <cstring>
#include <string>
#include <vector>

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "iphlpapi.lib")

namespace remokey {

namespace {

std::string WideToUtf8(const wchar_t* w) {
    if (!w) return {};
    int len = WideCharToMultiByte(CP_UTF8, 0, w, -1, nullptr, 0, nullptr, nullptr);
    if (len <= 0) return {};
    std::string s(len - 1, '\0');
    WideCharToMultiByte(CP_UTF8, 0, w, -1, s.data(), len, nullptr, nullptr);
    return s;
}

bool IsLoopback(const std::string& ip) {
    return ip.rfind("127.", 0) == 0;
}

bool IsApipa(const std::string& ip) {
    // 169.254.x.x 自动私有地址，通常是没拿到 DHCP 的网卡
    return ip.rfind("169.254.", 0) == 0;
}

// 判断适配器描述是否像虚拟网卡
bool LooksVirtual(const std::string& desc) {
    static const char* kVirtualKeywords[] = {
        "vmware", "virtualbox", "vbox", "hyper-v", "hyperv",
        "wsl", "docker", "vethernet", "loopback", "tap-",
        "tun", "wireguard", "openvpn", "zerotier", "tailscale",
        "npcap", "bluetooth", "teredo", "isatap",
        "microsoft wi-fi direct", "wi-fi direct",
        "virtual", "vpn", "pseudo"
    };

    std::string lower = desc;
    std::transform(lower.begin(), lower.end(), lower.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

    for (auto* kw : kVirtualKeywords) {
        if (lower.find(kw) != std::string::npos) return true;
    }
    return false;
}

// 判断适配器描述是否像物理有线/无线网卡
bool LooksPhysical(const std::string& desc) {
    static const char* kPhysicalKeywords[] = {
        "ethernet", "以太网", "wlan", "wi-fi", "wifi", "无线",
        "realtek", "intel", "broadcom", "qualcomm", "atheros",
        "gigabit", "fast ethernet", "802.11"
    };

    std::string lower = desc;
    std::transform(lower.begin(), lower.end(), lower.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

    for (auto* kw : kPhysicalKeywords) {
        if (lower.find(kw) != std::string::npos) return true;
    }
    return false;
}

} // namespace

std::string GetPrimaryOutboundIPv4() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        return {};
    }

    SOCKET sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock == INVALID_SOCKET) {
        WSACleanup();
        return {};
    }

    sockaddr_in remote{};
    remote.sin_family      = AF_INET;
    remote.sin_port        = htons(53);      // 任意远端端口，不会真正发包
    inet_pton(AF_INET, "8.8.8.8", &remote.sin_addr);

    std::string result;
    if (connect(sock, reinterpret_cast<sockaddr*>(&remote), sizeof(remote)) == 0) {
        sockaddr_in local{};
        int len = sizeof(local);
        if (getsockname(sock, reinterpret_cast<sockaddr*>(&local), &len) == 0) {
            char buf[INET_ADDRSTRLEN] = {};
            if (inet_ntop(AF_INET, &local.sin_addr, buf, sizeof(buf))) {
                result = buf;
            }
        }
    }

    closesocket(sock);
    WSACleanup();
    return result;
}

std::vector<LocalAddress> CollectLocalIPv4() {
    std::vector<LocalAddress> result;

    const std::string primary = GetPrimaryOutboundIPv4();

    ULONG flags = GAA_FLAG_SKIP_ANYCAST
                | GAA_FLAG_SKIP_MULTICAST
                | GAA_FLAG_SKIP_DNS_SERVER;

    ULONG family = AF_INET;
    ULONG bufLen = 16 * 1024;
    std::vector<BYTE> buffer(bufLen);

    ULONG ret = GetAdaptersAddresses(family, flags, nullptr,
                                     reinterpret_cast<IP_ADAPTER_ADDRESSES*>(buffer.data()),
                                     &bufLen);
    if (ret == ERROR_BUFFER_OVERFLOW) {
        buffer.resize(bufLen);
        ret = GetAdaptersAddresses(family, flags, nullptr,
                                   reinterpret_cast<IP_ADAPTER_ADDRESSES*>(buffer.data()),
                                   &bufLen);
    }
    if (ret != NO_ERROR) {
        // 失败时至少把出口 IP 返回
        if (!primary.empty()) {
            result.push_back({ primary, "默认出口", 0 });
        }
        return result;
    }

    auto* adapters = reinterpret_cast<IP_ADAPTER_ADDRESSES*>(buffer.data());

    for (auto* a = adapters; a != nullptr; a = a->Next) {
        // 只保留已启用的适配器
        if (a->OperStatus != IfOperStatusUp) continue;

        std::string desc = WideToUtf8(a->Description);
        bool virtualAdapter = LooksVirtual(desc);
        bool physicalAdapter = LooksPhysical(desc);

        for (auto* ua = a->FirstUnicastAddress; ua != nullptr; ua = ua->Next) {
            if (!ua->Address.lpSockaddr) continue;
            if (ua->Address.lpSockaddr->sa_family != AF_INET) continue;

            auto* sin = reinterpret_cast<sockaddr_in*>(ua->Address.lpSockaddr);
            char ipBuf[INET_ADDRSTRLEN] = {};
            if (!inet_ntop(AF_INET, &sin->sin_addr, ipBuf, sizeof(ipBuf))) continue;

            std::string ip = ipBuf;
            if (IsLoopback(ip)) continue;
            if (IsApipa(ip))    continue;

            int metric = 1000;

            if (!primary.empty() && ip == primary) {
                metric = 0;                 // 默认出口，最高优先级
            } else if (physicalAdapter && !virtualAdapter) {
                metric = 100;               // 物理网卡
            } else if (virtualAdapter) {
                metric = 500;               // 虚拟网卡靠后
            } else {
                metric = 300;               // 其它
            }

            // 结合接口 metric 做次级排序
            metric += static_cast<int>(a->Ipv4Metric);

            LocalAddress la;
            la.ip      = ip;
            la.adapter = desc.empty() ? std::string("未知适配器") : desc;
            la.metric  = metric;
            result.push_back(std::move(la));
        }
    }

    // 去重（同一 IP 只保留 metric 最小的那条）
    std::sort(result.begin(), result.end(),
              [](const LocalAddress& a, const LocalAddress& b) {
                  if (a.ip != b.ip) return a.ip < b.ip;
                  return a.metric < b.metric;
              });
    result.erase(std::unique(result.begin(), result.end(),
                             [](const LocalAddress& a, const LocalAddress& b) {
                                 return a.ip == b.ip;
                             }),
                 result.end());

    // 按 metric 升序
    std::sort(result.begin(), result.end(),
              [](const LocalAddress& a, const LocalAddress& b) {
                  return a.metric < b.metric;
              });

    return result;
}

} // namespace remokey
