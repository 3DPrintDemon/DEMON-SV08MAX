#include "../include/mks_net_info.h"

bool get_interface_info(const char *interface_name, char *ip_address, char *mac_address) {
    struct ifaddrs *ifaddr, *ifa;
    int fd;
    struct ifreq ifr;
    bool found_ip = false, found_mac = false;

    // 初始化输出
    ip_address[0] = '\0';
    mac_address[0] = '\0';
    
    // 获取最新的 IP 地址
    if (getifaddrs(&ifaddr) == -1) {
        return false;
    }

    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr && ifa->ifa_addr->sa_family == AF_INET && 
            strcmp(ifa->ifa_name, interface_name) == 0) {
            strcpy(ip_address, inet_ntoa(((struct sockaddr_in *)ifa->ifa_addr)->sin_addr));
            found_ip = true;
            break;
        }
    }
    freeifaddrs(ifaddr);

    // 获取 MAC 地址
    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) {
        return false;
    }

    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, interface_name, IFNAMSIZ - 1);
    
    if (ioctl(fd, SIOCGIFHWADDR, &ifr) == 0) {
        sprintf(mac_address, "%02X:%02X:%02X:%02X:%02X:%02X",
                (unsigned char)ifr.ifr_hwaddr.sa_data[0],
                (unsigned char)ifr.ifr_hwaddr.sa_data[1],
                (unsigned char)ifr.ifr_hwaddr.sa_data[2],
                (unsigned char)ifr.ifr_hwaddr.sa_data[3],
                (unsigned char)ifr.ifr_hwaddr.sa_data[4],
                (unsigned char)ifr.ifr_hwaddr.sa_data[5]);
        found_mac = true;
    }
    
    close(fd);
    return found_ip && found_mac;
}
//获取接口信息
// bool get_interface_info(const char *interface_name, char *ip_address, char *mac_address) {
//     int fd;
//     struct ifreq ifr;

//     // 创建一个socket
//     fd = socket(AF_INET, SOCK_DGRAM, 0);
//     if (fd < 0) {
//         // perror("socket");
//         return false;
//     }

//     // 设置接口名
//     memset(&ifr, 0, sizeof(ifr));
//     strncpy(ifr.ifr_name, interface_name, IFNAMSIZ - 1);

//     // 获取IP地址
//     if (ioctl(fd, SIOCGIFADDR, &ifr) < 0) {
//         // perror("ioctl(SIOCGIFADDR)");
//         close(fd);
//         return false;
//     }
//     strcpy(ip_address, inet_ntoa(((struct sockaddr_in*)&ifr.ifr_addr)->sin_addr));

//     // 获取MAC地址
//     if (ioctl(fd, SIOCGIFHWADDR, &ifr) < 0) {
//         // perror("ioctl(SIOCGIFHWADDR)");
//         close(fd);
//         return false;
//     }

//     sprintf(mac_address, "%02X:%02X:%02X:%02X:%02X:%02X",
//             (unsigned char)ifr.ifr_hwaddr.sa_data[0],
//             (unsigned char)ifr.ifr_hwaddr.sa_data[1],
//             (unsigned char)ifr.ifr_hwaddr.sa_data[2],
//             (unsigned char)ifr.ifr_hwaddr.sa_data[3],
//             (unsigned char)ifr.ifr_hwaddr.sa_data[4],
//             (unsigned char)ifr.ifr_hwaddr.sa_data[5]);

//     close(fd);
//     return true;
// }


//通过获取IP地址，判断接口是否存在
bool is_interface_exist(const char *interface_name) {
    int fd;
    struct ifreq ifr;

    // 创建一个socket
    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) {
        perror("socket");
    }

    // 设置接口名
    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, interface_name, IFNAMSIZ - 1);

    // 获取接口标志信息（ip地址）
    if (ioctl(fd, SIOCGIFFLAGS, &ifr) < 0) {
        close(fd);
        return false;
    }

    close(fd);
    return true;
}
