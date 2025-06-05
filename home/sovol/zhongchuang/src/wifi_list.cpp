#include "wifi_list.h"

std::string get_wifi_name;               // 要操作的wifi名称
std::string current_connected_ssid_name; // 当前连接的wifi名称
char ip_address[INET_ADDRSTRLEN] = {0};        // IP地址
char mac_address[18];                    // MAC地址

std::string page_wifi_ssid_list[WIFI_LIST_NUMBER]; // 当前页wifi名称列表
bool page_wifi_psk_list[WIFI_LIST_NUMBER];         // 当前页wifi是否需要密码列表
int page_wifi_level_list[WIFI_LIST_NUMBER];   // 当前页wifi信号强度列表
int page_wifi_frequency_list[WIFI_LIST_NUMBER];   // 当前页wifi信号强度列表

// 获取当前页数的wifi列表
void get_display_wifi_list(int pages)
{
    int i = 0;

    for (i = 0; i < WIFI_LIST_NUMBER; i++)
    {
        page_wifi_ssid_list[i] = "";
        page_wifi_psk_list[i] = true;
        page_wifi_level_list[i] = 0;
        page_wifi_frequency_list[i] = 2400; // default 2.4G
    }

    auto it = ssid_list.begin();
    auto it_psk = psk_list.begin();
    auto it_level = level_list.begin();
    auto it_freq = frequency_list.begin();

    for (int i = 0; i < pages * WIFI_LIST_NUMBER; i++)
    {
        it++;
        it_level++;
        it_psk++;
        it_freq++;
    }

    for (i = 0; i < WIFI_LIST_NUMBER; i++)
    {
        if (it != ssid_list.end())
        {
            page_wifi_ssid_list[i] = *it++;
            page_wifi_level_list[i] = *it_level++;
            page_wifi_psk_list[i] = *it_psk++;
            page_wifi_frequency_list[i] = *it_freq++;
        }
    }
}

std::string get_connected_wifi_ssid() {
    char buffer[128];
    std::string ssid;
    FILE* pipe = popen("iw dev wlan0 link | grep 'SSID' | awk -F ': ' '{print $2}'", "r");
    if (!pipe) return "Error";

    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        ssid = buffer;
    }
    pclose(pipe);

    // 去掉换行符
    ssid.erase(std::remove(ssid.begin(), ssid.end(), '\n'), ssid.end());
    return ssid.empty() ? "Not Connected" : ssid;
}

void update_wifilist_after_connected()
{
    current_connected_ssid_name = get_connected_wifi_ssid(); // 如果已经连接wifi，获取wifi的名字
    // mkslog_print("update_wifilist_after_connected ssid_name:");
    // mkslog_print(current_connected_ssid_name.c_str());
    // mks_wpa_scan_scan_r();
    top_current_wifi();
    if (current_page_id == TJC_PAGE_WIFI_LIST)
    {
        wifi_list_current_pages = 0;
        get_display_wifi_list(wifi_list_current_pages); // 获取当前页列表
        refresh_page_wifi_list(); 
    }
}

// 初始化wifi列表
void wifi_list_init()
{
    if (wifi_enable)
    {
        if (wpa_state == CONNECTED)
        {
            current_connected_ssid_name = get_connected_wifi_ssid(); // 如果已经连接wifi，获取wifi的名字
            MKSLOG_YELLOW("wifi_list_init, ssid: %s", current_connected_ssid_name.c_str());
        }
        else
        {
            mks_wpa_enable_network();       // 使能wpa
            current_connected_ssid_name.clear(); // 如果没连接wifi，清除掉当前已连接wifi的名字
            page_to(TJC_PAGE_WIFI_SCANING); // 跳转到wifi加载
            sleep(2);
            mks_wpa_scan_scan_r(); // 扫描wifi
        }
        get_wlan0_status();
        page_to_wifi_list();
        send_cmd_picc_picc2(tty_fd, "b0", "53", "53");  // 切换开关图标
        wifi_list_current_pages = 0;
        get_display_wifi_list(wifi_list_current_pages); // 获取当前页列表
        refresh_page_wifi_list();                       // 刷新显示列表
    }
    else
    {
        page_to_wifi_list();
        mks_disable_network(); // 失能wpa
        wifi_list_total_pages = 0;
        send_cmd_picc_picc2(tty_fd, "b0", "52", "52"); // 切换开关图标
        send_cmd_picc_picc2(tty_fd, "b3", "29", "29"); // 隐藏上一页图标
        send_cmd_picc_picc2(tty_fd, "b4", "29", "29"); // 隐藏下一页图标
        current_ip_address = "";
        send_cmd_txt(tty_fd, "t1", "IP: " + current_ip_address);
        // 清空列表
        ssid_list.clear();
        for (int i = 0; i < WIFI_LIST_NUMBER; i++)
        {
            send_cmd_txt(tty_fd, "wifi_" + std::to_string(i), "");
            send_cmd_txt(tty_fd, "t" + std::to_string(i + 5), "");
            send_cmd_picc(tty_fd, "q" + std::to_string(i), "29");
            send_cmd_picc(tty_fd, "q" + std::to_string(i + WIFI_LIST_NUMBER), "29");
        }
    }
}

// 扫描wifi列表
void wifi_list_scan()
{
    page_to(TJC_PAGE_WIFI_SCANING);                 // 跳转到wifi加载
    mks_wpa_scan_scan_r();                          // 扫描wifi
    wifi_list_current_pages = 0;
    get_display_wifi_list(wifi_list_current_pages); // 获取当前页列表
    page_to_wifi_list();
    refresh_page_wifi_list();                       // 刷新显示列表
}

// 点击下一页
void click_next_page()
{
    if (wifi_list_current_pages < wifi_list_total_pages)
    {
        wifi_list_current_pages++;
        get_display_wifi_list(wifi_list_current_pages);
        refresh_page_wifi_list();
    }
}

// 点击上一页
void click_previous_page()
{
    if (wifi_list_current_pages > 0)
    {
        wifi_list_current_pages--;
        get_display_wifi_list(wifi_list_current_pages);
        refresh_page_wifi_list();
    }
}

// 刷新页面
void refresh_page_wifi_list()
{
    std::cout << "当前是第 " << wifi_list_current_pages << " 页" << std::endl;

    // 显示上一页
    if (wifi_list_current_pages > 0)
    {
        send_cmd_picc_picc2(tty_fd, "b3", "59", "60");
    }
    else
    {
        send_cmd_picc_picc2(tty_fd, "b3", "29", "29");
    }

    // 显示下一页
    if (wifi_list_current_pages < wifi_list_total_pages)
    {
        send_cmd_picc_picc2(tty_fd, "b4", "59", "60");
    }
    else
    {
        send_cmd_picc_picc2(tty_fd, "b4", "29", "29");
    }

    // 显示wifi列表
    for (int i = 0; i < WIFI_LIST_NUMBER; i++)
    {
        if (page_wifi_ssid_list[i] != "")
        {
            // 显示wifi名称
            int wifi_freq = page_wifi_frequency_list[i];
            std::string frequency = wifi_freq >= 5000 ? "5G" : "2.4G";
            send_cmd_txt(tty_fd, "wifi_" + std::to_string(i), page_wifi_ssid_list[i] + "(" + frequency + ")");
            MKSLOG("当前页面wifi列表为：%s", page_wifi_ssid_list[i].c_str());

            //显示信号强度
            send_cmd_txt(tty_fd, "t" + std::to_string(i + 5), std::to_string(page_wifi_level_list[i]) + "dB");

            // 显示是否有密码
            if (page_wifi_psk_list[i])
            {
                send_cmd_picc(tty_fd, "q" + std::to_string(i), "54");
            }
            else
            {
                send_cmd_picc(tty_fd, "q" + std::to_string(i), "55");
            }

            // 显示wifi信号强度图标
            if (page_wifi_level_list[i] >= -55)
            {
                send_cmd_picc(tty_fd, "q" + std::to_string(i + WIFI_LIST_NUMBER), "58");
            }
            else if (page_wifi_level_list[i] < -55 && page_wifi_level_list[i] >= -77)
            {
                send_cmd_picc(tty_fd, "q" + std::to_string(i + WIFI_LIST_NUMBER), "57");
            }
            else if (page_wifi_level_list[i] < -77 && page_wifi_level_list[i] >= -88)
            {
                send_cmd_picc(tty_fd, "q" + std::to_string(i + WIFI_LIST_NUMBER), "56");
            }
            else if (page_wifi_level_list[i] < -88)
            {
                send_cmd_picc(tty_fd, "q" + std::to_string(i + WIFI_LIST_NUMBER), "55");
            }
        }
        else
        {
            send_cmd_txt(tty_fd, "wifi_" + std::to_string(i), "");
            send_cmd_txt(tty_fd, "t" + std::to_string(i + 5), "");
            send_cmd_picc(tty_fd, "q" + std::to_string(i), "29");
            send_cmd_picc(tty_fd, "q" + std::to_string(i + WIFI_LIST_NUMBER), "29");
        }
    }

    // 当前连接wifi高亮显示
    if (current_connected_ssid_name == page_wifi_ssid_list[0])
    {
        send_cmd_pco(tty_fd, "wifi_0", "1983");
        send_cmd_pco(tty_fd, "t5", "1983");
        send_cmd_picc(tty_fd, "q0", "73");
    } 
    else
    {
        send_cmd_pco(tty_fd, "wifi_0", "65535");
        send_cmd_pco(tty_fd, "t5", "65535");
    }
}

// 获取第n个的wifi名
void click_wifi_list_ssid(int index)
{
    // 防止点击空列表时进入键盘
    if (page_wifi_ssid_list[index] == "")
        return;

    get_wifi_name.clear();
    get_wifi_name = page_wifi_ssid_list[index];
    int wifi_freq = page_wifi_frequency_list[index];
    std::string frequency = wifi_freq >= 5000 ? "5G" : "2.4G";

    // 有密码需要输入密码
    if (page_wifi_psk_list[index] == true)
    {
        page_to(TJC_PAGE_WIFI_KB);
        send_cmd_txt(tty_fd, "t0", get_wifi_name + "(" + frequency + ")");
    }
    else
    {
        set_ssid_none_psk();
    }
}

// 无加密时连接wifi
void set_ssid_none_psk()
{
    mks_wpa_set_ssid(const_cast<char *>(get_wifi_name.c_str())); // 发送请求
    // elegoo_wpa_set_key_mgmt_none();
    // elegoo_wpa_select_network_0();
    // elegoo_wpa_reassociate();
}

// 有加密时连接wifi
void set_ssid_psk(char *psk)
{
    mks_wpa_set_ssid(const_cast<char *>(get_wifi_name.c_str())); // 发送请求
    mks_wpa_set_psk(psk);                                        // 输入密码
    wpa_state = DISCONNECTED;
    mks_disconnect(); // 断开连接
}

// 获取wpa状态
void get_wlan0_status()
{
    mks_status();

    // printf("%s: ack:\n%s", __FUNCTION__, status_result.ack);
    // printf("%s: bssid:%s\n", __FUNCTION__, status_result.bssid);
    // printf("%s: freq:%d\n", __FUNCTION__, status_result.freq);
    // printf("%s: ssid:%s\n", __FUNCTION__, status_result.ssid);
    // printf("%s: id:%d\n", __FUNCTION__, status_result.id);
    // printf("%s: mode:%s\n", __FUNCTION__, status_result.mode);
    // printf("%s: pairwise_cipher:%s\n", __FUNCTION__, status_result.pairwise_cipher);
    // printf("%s: group_cipher:%s\n", __FUNCTION__, status_result.group_cipher);
    // printf("%s: key_mgmt:%s\n", __FUNCTION__, status_result.key_mgmt);
    // printf("%s: wpa_state:%s\n", __FUNCTION__, status_result.wpa_state);
    // printf("%s: ip_address:%s\n", __FUNCTION__, status_result.ip_address);
    // printf("%s: address:%s\n", __FUNCTION__, status_result.address);
    // printf("%s: uuid:%s\n", __FUNCTION__, status_result.uuid);
}

bool connectToWiFi(const std::string& ssid, const std::string& password) {
    // 构建 nmcli 命令
    current_connected_ssid_name.clear();
    std::string cmd = "nmcli dev wifi connect \"" + ssid + "\" password \"" + password + "\"";

    // 执行命令并捕获返回值
    int ret = std::system(cmd.c_str());

    // 判断连接是否成功
    if (ret == 0) {
        std::cout << "Wi-Fi 连接成功: " << ssid << std::endl;
        return true;
    } else {
        std::cout << "Wi-Fi 连接失败: " << ssid << std::endl;
        return false;
    }
}

//刷新全局wifi信号强度
void refresh_wifi_level()
{
    static int start_time = time(NULL);
    if (time_differ(3, start_time))
    {
        start_time = time(NULL);
        get_wlan0_status();
        if (strstr(status_result.wpa_state, "COMPLETED") != NULL)
        {
            // 显示wifi信号强度图标
            if(status_result.freq == 0)
            {
                send_cmd_picc(tty_fd, "q8", "17");
            }
            else if (status_result.freq >= -55)
            {
                send_cmd_picc(tty_fd, "q8", "21");
            }
            else if (status_result.freq < -55 && status_result.freq >= -77)
            {
                send_cmd_picc(tty_fd, "q8", "20");
            }
            else if (status_result.freq < -77 && status_result.freq >= -88)
            {
                send_cmd_picc(tty_fd, "q8", "19");
            }
            else if (status_result.freq < -88)
            {
                send_cmd_picc(tty_fd, "q8", "18");
            }
        }
        else if (strstr(status_result.wpa_state, "INACTIVE") != NULL)
        {
            send_cmd_picc(tty_fd, "q8", "17");
        }
        else
        {
            send_cmd_picc(tty_fd, "q8", "17");
        }
    }
}