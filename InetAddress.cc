#include "InetAddress.h"
#include "Logger.h"

#include <cstring>

InetAddress::InetAddress(uint16_t port, std::string ip){
    memset(&addr_, 0, sizeof(addr_));
    
    addr_.sin_family = AF_INET;  // IPv4
    addr_.sin_port = htons(port);  // 端口转网络字节序
    
    // inet_pton：将字符串 IP 转换为网络字节序，返回 0 表示格式错误，-1 表示协议错误
    int ret = inet_pton(AF_INET, ip.c_str(), &addr_.sin_addr);
    if (ret != 1) {
        if (ret == 0) {
            LOG_FATAL("Invalid IP address format %s", ip.c_str());
        } else {  // ret == -1
            LOG_FATAL("Unsupported address family for IP");
        }
    }   
}
std::string InetAddress::toIp() const{
    char buf[64] = {0};
    inet_ntop(AF_INET, &addr_.sin_addr, buf, sizeof buf);
    return buf;
}
std::string InetAddress::toIpPort() const{
    char buf[64] = {0};
    inet_ntop(AF_INET, &addr_.sin_addr, buf, sizeof buf);
    size_t end = strlen(buf);
    uint16_t port = ntohs(addr_.sin_port);
    sprintf(buf + end, ":%u", port);
    return buf;
}
uint16_t InetAddress::toPort() const{
    return ntohs(addr_.sin_port);
}
