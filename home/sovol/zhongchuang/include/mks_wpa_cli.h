#ifndef MKS_WPA_CLI_H
#define MKS_WPA_CLI_H

#include <iostream>
#include <thread>
#include <chrono>
#include <cstring>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <wpa_ctrl.h>
#include <vector>


#define WIFI_LIST_NUMBER 4


extern uint32_t wifi_list_total_pages;   // 总页数
extern uint32_t wifi_list_current_pages; // 当前页数

extern std::vector<std::string> ssid_list; // 所有wifi名称列表
extern std::vector<int> level_list;        // 所有wifi是否需要密码列表
extern std::vector<bool> psk_list;         // 所有wifi信号强度列表
extern std::vector<int> frequency_list;

typedef enum
{
    SCANNING,
    DISCONNECTED,
    CONNECTED,
    CONNECT_FAILED,
    SAVED,
}WPA0_STATUS;
extern WPA0_STATUS wpa_state;



struct mks_wifi_status_result_t 
{
    char ack[1024];
    char bssid[18];
    int freq;
    char ssid[128];       
    int id;
    char mode[16];
    char pairwise_cipher[16];
    char group_cipher[16];
    char key_mgmt[16];
    char wpa_state[32];
    char ip_address[18];
    char address[64];
    char uuid[64];
};
extern struct mks_wifi_status_result_t status_result;

void *handle_message(void *arg);
int mks_wpa_scan_scan_r();
int parse_scan_results(char* scan_results);
size_t printf_decode(unsigned char *buf, size_t maxlen, const char *str);
int hex2byte(const char *hex);
static int hex2num(char c);
int mks_wpa_set_ssid(char *ssid);
int mks_wpa_set_psk(char *psk);

int mks_wpa_enable_network();
int mks_save_config();
int mks_disconnect();
int mks_disable_network();
int mks_reconfigure();
int mks_reconnect();
int mks_status();
void parse_status(char *buffer);
int mks_wpa_cli_open_connection();
int mks_wpa_cli_close_connection();
void top_current_wifi();
static void result_get(char *str, char *key, char *val, int val_len);

#endif
