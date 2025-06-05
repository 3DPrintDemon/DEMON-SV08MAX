#ifndef __WIFI_LIST__
#define __WIFI_LIST__


#include <iostream>
#include <thread>
#include <chrono>
#include <cstring>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <wpa_ctrl.h>
#include <vector>

#include "ui.h"
#include "event.h"
#include "refresh_ui.h"
#include "mks_wpa_cli.h"
#include "mks_net_info.h"




extern std::string get_wifi_name;        // 要操作的wifi名称
extern std::string current_connected_ssid_name;  // 当前连接的wifi名称
extern char ip_address[INET_ADDRSTRLEN]; // IP地址
extern char mac_address[18];             // MAC地址

extern std::string page_wifi_ssid_list[WIFI_LIST_NUMBER]; // 当前页wifi名称列表
extern bool page_wifi_psk_list[WIFI_LIST_NUMBER];         // 当前页wifi是否需要密码列表
extern int page_wifi_level_list[WIFI_LIST_NUMBER];   // 当前页wifi信号强度列表

bool connectToWiFi(const std::string& ssid, const std::string& password);

void get_display_wifi_list(int pages);
void wifi_list_init();
void wifi_list_scan();
void click_next_page();
void click_previous_page();
void refresh_page_wifi_list();
void click_wifi_list_ssid(int index);
void set_ssid_psk(char *psk);
void set_ssid_none_psk();
void get_wlan0_status();
void refresh_wifi_level();
void update_wifilist_after_connected();







#endif
