#ifndef IP_HELPER_HPP
#define IP_HELPER_HPP

#include <string>
#include <vector>
#include <cstdint>

namespace remokey {

struct LocalAddress {
    std::string ip;       // 点分十进制 IPv4
    std::string adapter;  // 适配器描述（用于下拉框展示）
    int         metric;   // 越小优先级越高（内部使用）
};

// 收集本机所有已启用、非回环的 IPv4 地址，按"最可能是局域网地址"排序。
// 排序规则（优先级从高到低）：
//   1) 默认路由所在网卡（通过 UDP connect 探测出口 IP）排在最前
//   2) 物理以太网 / 无线网卡 优先于 虚拟网卡（VMware/VirtualBox/Hyper-V/WSL/Docker 等）
//   3) 同一优先级下按接口 metric 升序
std::vector<LocalAddress> CollectLocalIPv4();

// 返回最可能的出口 IP（UDP connect 8.8.8.8 得到的本机地址）；失败返回空串
std::string GetPrimaryOutboundIPv4();

} // namespace remokey

#endif // IP_HELPER_HPP
