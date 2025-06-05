#include "../include/mks_wpa_cli.h"
#include "../include/mks_log.h"
#include "../include/ui.h"
#include "../include/send_msg.h"
#include "../include/mks_net_info.h"
#include <vector>

static struct wpa_ctrl *ctrl_conn;
static struct wpa_ctrl *mon_conn;


extern int current_page_id;                     // 当前页面的id号
extern std::string current_connected_ssid_name; // 当前连接的wifi名称

uint32_t wifi_list_total_pages;   // 总页数
uint32_t wifi_list_current_pages; // 当前页数

struct mks_wifi_status_result_t status_result;

std::vector<std::string> ssid_list; // 所有wifi名称列表
std::vector<int> level_list;        // 所有wifi是否需要密码列表
std::vector<bool> psk_list;         // 所有wifi信号强度列表
std::vector<int> frequency_list; 

WPA0_STATUS wpa_state;

// wifi服务端线程
void *handle_message(void *arg)
{
    char path[64] = {"\0"};
    sprintf(path, "/var/run/wpa_supplicant/wlan0");

    while (1)
    {
        if (access(path, F_OK) == 0)
        {
            break;
        }
        usleep(100000);
    }

    mon_conn = wpa_ctrl_open(path);

    if (!mon_conn)
    {
        MKSLOG_YELLOW("监听wpa线程失败");
        pthread_exit(NULL);
    }
    else
    {
        MKSLOG_YELLOW("开启mon_conn成功");
        wpa_ctrl_attach(mon_conn); // 注册为控制接口的事件监视器
    }

    while (1)
    {
        if (wpa_ctrl_pending(mon_conn) > 0) // 检查是否存在挂起
        {
            char buf[4096];
            size_t len = sizeof(buf) - 1;
            if (wpa_ctrl_recv(mon_conn, buf, &len) == 0)
            {
                buf[len] = '\0';
                // MKSLOG_YELLOW("收到wpa回调信息: \n%s", buf);
                if (strstr(buf, "CTRL-EVENT-SCAN-RESULTS") != NULL)
                {
                    // MKSLOG_HIGHLIGHT("已获取到了扫描的结果");
                }
                else if (strstr(buf, "CTRL-EVENT-DISCONNECTED") != NULL)
                {
                    MKSLOG_HIGHLIGHT("已经断开wifi链接");
                    wpa_state = DISCONNECTED;
                }
                else if (strstr(buf, "CTRL-EVENT-CONNECTED") != NULL)
                {
                    MKSLOG_HIGHLIGHT("成功连接上wifi");
                    wpa_state = CONNECTED;
                    system("dhcpcd wlan0");
                    update_wifilist_after_connected();                    
                }
                else if (strstr(buf, WPS_EVENT_AP_AVAILABLE) != NULL)
                {
                    // MKSLOG("Available WPS AP found in scan results.");
                }
                else if (strstr(buf, "pre-shared key may be incorrect") != NULL) // 密码错误
                {
                    MKSLOG_HIGHLIGHT("密码错误");
                    wpa_state = CONNECT_FAILED;
                }
                else if (strstr(buf, "CONN_FAILED") != NULL) // 连接失败
                {
                    MKSLOG_HIGHLIGHT("连接失败");
                    wpa_state = CONNECT_FAILED;
                }
                else if (strstr(buf, "Associated with") != NULL)
                {
                    MKSLOG_RED("握手握手握手");
                }
            }
        }
        else
        {
            usleep(30000);
        }
    }
}

// 打开wpa客户端
int mks_wpa_cli_open_connection()
{
    char path[64] = {"\0"};
    sprintf(path, "/var/run/wpa_supplicant/wlan0");
    ctrl_conn = wpa_ctrl_open(path);

    if (ctrl_conn)
    {
        MKSLOG_RED("成功连接wpa connection");
        return 0;
    }
    else
    {
        MKSLOG_RED("failed to open wpa connection");
        return -1;
    }
}

// 关闭wpa客户端
int mks_wpa_cli_close_connection()
{
    wpa_ctrl_close(ctrl_conn);
    ctrl_conn = NULL;
    MKSLOG_RED("断开wpa connection");
    return 0;
}

// 扫描wifi
int mks_wpa_scan_scan_r()
{
    char path[64] = {"\0"};

    sprintf(path, "/var/run/wpa_supplicant/wlan0");

    if (!ctrl_conn)
    {
        MKSLOG("Open wpa control interfaces failed!\n");
        return -3;
    }

    char replyBuff[4096] = {"\0"};
    size_t reply_len;

    int ret;
    reply_len = sizeof(replyBuff) - 1;
    ret = wpa_ctrl_request(ctrl_conn, "SCAN", strlen("SCAN"), replyBuff, &reply_len, NULL);
    if (-2 == ret)
    {
        MKSLOG_YELLOW("Command timed out.");
        return ret;
    }
    else if (ret < 0)
    {
        MKSLOG_YELLOW("Command failed.");
        return ret;
    }
    else if (0 == ret)
    {
        replyBuff[reply_len] = '\0';
        MKSLOG("%s", replyBuff);
        sleep(3);
    }

    memset(replyBuff, 0x00, sizeof(replyBuff));
    reply_len = sizeof(replyBuff) - 1;
    ret = wpa_ctrl_request(ctrl_conn, "SCAN_RESULTS", strlen("SCAN_RESULTS"), replyBuff, &reply_len, NULL);

    if (ret == -2)
    {
        MKSLOG_RED("Command timed out.");
        return ret;
    }
    else if (ret < 0)
    {
        MKSLOG_RED("Command failed.");
        return ret;
    }
    // 解析扫描结果
    else if (ret == 0)
    {
        replyBuff[reply_len] = '\0';
        MKSLOG("+++SCAN_RESULTS: %s", replyBuff);
        parse_scan_results(replyBuff);
    }

    return ret;
}

// 置顶当前连接wifi
void top_current_wifi()
{
    for (int j = 0; j < ssid_list.size(); j++)
    {
        if (current_connected_ssid_name == ssid_list[j])
        {
            std::string ssid_temp = ssid_list[j];
            int level_tmp = level_list[j];
            bool psk_tmp = psk_list[j];
            int frequency_tmp = frequency_list[j];

            ssid_list[j] = ssid_list[0];
            level_list[j] = level_list[0];
            psk_list[j] = psk_list[0];
            frequency_list[j] = frequency_list[0];

            ssid_list[0] = ssid_temp;
            level_list[0] = level_tmp;
            psk_list[0] = psk_tmp;
            frequency_list[0] = frequency_tmp;
        }
    }
}

// 解析扫描结果
int parse_scan_results(char *scan_results)
{
    ssid_list.clear();
    level_list.clear();
    psk_list.clear();
    frequency_list.clear();
    
    char buffer[4096];
    strcpy(buffer, scan_results);
    char *lines[128] = {0};
    int num_lines = 0;
    char *line = strtok(buffer, "\n");
    while (line != NULL)
    {
        lines[num_lines++] = line;
        line = strtok(NULL, "\n");
    }

    for (int i = 1; i < num_lines; ++i)
    {
        char *fields[5] = {0};
        int num_fields = 0;
        char ssid_line[256] = {0};
        memset(ssid_line, 0x00, sizeof(ssid_line));
        strcpy(ssid_line, lines[i]);
        int ssid_line_index = 0;

        char *field = strtok(lines[i], " \t");

        while (field != NULL)
        {
            if (4 == num_fields)
            {
                ssid_line_index = field - lines[i];
            }

            fields[num_fields++] = field;
            field = strtok(NULL, " \t");
            if (5 == num_fields)
            {
                break;
            }
        }

        if (num_fields < 5)
        {
            printf("Invalid scan result: %s\n", lines[i]);
            continue;
        }
        else
        {
            unsigned char ssid_name[192];
            printf_decode(ssid_name, 192, ssid_line + ssid_line_index);
            if (ssid_name[0] != '\x00')
            {
                ssid_list.push_back((char *)ssid_name);
                level_list.push_back(atoi(fields[2]));
                frequency_list.push_back(atoi(fields[1]));
                if (strstr(fields[3], "WPA"))
                {
                    psk_list.push_back(true);
                }
                else
                {
                    psk_list.push_back(false);
                }
            }
        }
    }

    // for (int k = 0; k < ssid_list.size(); k++)
    // {
    //     std::cout << "ssid :" << ssid_list[k] << std::endl;
    //     std::cout << "level :" << level_list[k] << std::endl;
    //     std::cout << "psk :" << psk_list[k] << std::endl;
    // }
    top_current_wifi();

    // 计算总页数
    if (0 == ssid_list.size() % WIFI_LIST_NUMBER)
    {
        wifi_list_total_pages = ssid_list.size() / WIFI_LIST_NUMBER - 1;
        if (wifi_list_total_pages < 0)
        {
            wifi_list_total_pages = 0;
        }
    }
    else
    {
        wifi_list_total_pages = ssid_list.size() / WIFI_LIST_NUMBER;
    }
    return 0;
}

size_t printf_decode(unsigned char *buf, size_t maxlen, const char *str)
{
    const char *pos = str;
    size_t len = 0;
    int val;

    while (*pos)
    {
        if (len + 1 >= maxlen)
            break;
        switch (*pos)
        {
        case '\\':
            pos++;
            switch (*pos)
            {
            case '\\':
                buf[len++] = '\\';
                pos++;
                break;
            case '"':
                buf[len++] = '"';
                pos++;
                break;
            case 'n':
                buf[len++] = '\n';
                pos++;
                break;
            case 'r':
                buf[len++] = '\r';
                pos++;
                break;
            case 't':
                buf[len++] = '\t';
                pos++;
                break;
            case 'e':
                buf[len++] = '\033';
                pos++;
                break;
            case 'x':
                pos++;
                val = hex2byte(pos);
                if (val < 0)
                {
                    val = hex2num(*pos);
                    if (val < 0)
                        break;
                    buf[len++] = val;
                    pos++;
                }
                else
                {
                    buf[len++] = val;
                    pos += 2;
                }
                break;
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
                val = *pos++ - '0';
                if (*pos >= '0' && *pos <= '7')
                    val = val * 8 + (*pos++ - '0');
                if (*pos >= '0' && *pos <= '7')
                    val = val * 8 + (*pos++ - '0');
                buf[len++] = val;
                break;
            default:
                break;
            }
            break;
        default:
            buf[len++] = *pos++;
            break;
        }
    }
    if (maxlen > len)
        buf[len] = '\0';

    return len;
}

int hex2byte(const char *hex)
{
    int a, b;
    a = hex2num(*hex++);
    if (a < 0)
        return -1;
    b = hex2num(*hex++);
    if (b < 0)
        return -1;
    return (a << 4) | b;
}

static int hex2num(char c)
{
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'a' && c <= 'f')
        return c - 'a' + 10;
    if (c >= 'A' && c <= 'F')
        return c - 'A' + 10;
    return -1;
}

// 发送设置请求
int mks_wpa_set_ssid(char *ssid)
{
    char path[64] = {"\0"};
    char cmd[64];
    char replyBuff[2048] = {"\0"};
    size_t reply_len;
    int ret;

    if (!ctrl_conn)
    {
        MKSLOG_BLUE("Open wpa control interfaces failed!\n");
        return -3;
    }

    memset(cmd, 0x00, sizeof(cmd));
    snprintf(cmd, sizeof(cmd) - 1, "SET_NETWORK 0 ssid \"%s\"", ssid);
    MKSLOG_RED("发送cmd命令为: %s", cmd);
    memset(replyBuff, 0x00, sizeof(replyBuff));
    reply_len = sizeof(replyBuff) - 1;
    ret = wpa_ctrl_request(ctrl_conn, cmd, strlen(cmd), replyBuff, &reply_len, NULL);
    if (ret == -2)
    {
        MKSLOG_RED("Command timed out.");
        return ret;
    }
    else if (ret < 0)
    {
        MKSLOG_RED("Command failed.");
        return ret;
    }
    else if (ret == 0)
    {
        replyBuff[reply_len] = '\0';
        MKSLOG_YELLOW("返回的消息: %s", replyBuff);
        if (strstr(replyBuff, "OK") != NULL)
        {
            return ret;
        }
        else if (strstr(replyBuff, "FAIL") != NULL)
        {
            return ret;
        }
    }
    return ret;
}

// 输入密码
int mks_wpa_set_psk(char *psk)
{
    char path[64] = {"\0"};
    char cmd[64];
    char replyBuff[2048] = {"\0"};
    size_t reply_len;
    int ret;

    if (!ctrl_conn)
    {
        MKSLOG_BLUE("Open wpa control interfaces failed!\n");
        return -3;
    }

    memset(cmd, 0x00, sizeof(cmd));
    sprintf(cmd, "SET_NETWORK 0 psk \"%s\"", psk);
    MKSLOG_RED("发送cmd命令为: %s", cmd);
    reply_len = sizeof(replyBuff) - 1;
    ret = wpa_ctrl_request(ctrl_conn, cmd, strlen(cmd), replyBuff, &reply_len, NULL);
    if (0 == ret)
    {
        replyBuff[reply_len] = '\0';
        MKSLOG_YELLOW("返回的消息: %s", replyBuff);
    }
    else if (ret = -2)
    {
        MKSLOG_RED("Command timed out. %s", replyBuff);
        return ret;
    }
    else if (ret < 0)
    {
        MKSLOG_RED("Command failed.");
        return ret;
    }
    return ret;
}

// 保存配置
int mks_save_config()
{
    char cmd[64] = {"\0"};
    char replyBuff[2048] = {"\0"};
    size_t reply_len;
    int ret;

    if (!ctrl_conn)
    {
        MKSLOG_RED("Open wpa control interfaces failed!");
        return -3;
    }

    /* wpa_ctrl_request, save_config */
    reply_len = sizeof(replyBuff) - 1;
    sprintf(cmd, "SAVE_CONFIG");
    memset(replyBuff, 0x00, sizeof(replyBuff));
    reply_len = sizeof(replyBuff) - 1;
    ret = wpa_ctrl_request(ctrl_conn, cmd, strlen(cmd), replyBuff, &reply_len, NULL);
    if (ret == -2)
    {
        MKSLOG_RED("Command timed out.");
        return ret;
    }
    else if (ret < 0)
    {
        MKSLOG_RED("Command failed.");
    }
    else if (ret == 0)
    {
        replyBuff[reply_len] = '\0';
        // MKSLOG_YELLOW("返回的消息: %s", replyBuff);

        if (strstr(replyBuff, "OK") != NULL)
        {
            MKSLOG_YELLOW("保存wifi配置成功");
            system("sync");
            wpa_state = SAVED;
        }
    }

    return ret;
}

//
int mks_reconfigure()
{
    char cmd[] = "RECONFIGURE";
    char replyBuff[2048];
    size_t reply_len = sizeof(replyBuff) - 1;

    if (!ctrl_conn)
    {
        MKSLOG_RED("Open wpa control interfaces failed!");
        return -3;
    }
    MKSLOG_RED("发送cmd命令为: %s", cmd);
    int ret = wpa_ctrl_request(ctrl_conn, cmd, strlen(cmd), replyBuff, &reply_len, NULL);

    if (ret == -2)
    {
        MKSLOG_RED("Command timed out.");
    }
    else if (ret < 0)
    {
        MKSLOG_RED("Command failed.");
    }
    else if (ret == 0)
    {
        replyBuff[reply_len] = '\0';
        MKSLOG_YELLOW("返回的消息: %s", replyBuff);
    }

    return ret;
}

// 断开连接
int mks_disconnect()
{
    char cmd[] = "DISCONNECT";
    char replyBuff[2048];
    size_t reply_len = sizeof(replyBuff) - 1;

    if (!ctrl_conn)
    {
        MKSLOG_RED("Open wpa control interfaces failed!");
        return -3;
    }
    MKSLOG_RED("发送cmd命令为: %s", cmd);
    int ret = wpa_ctrl_request(ctrl_conn, cmd, strlen(cmd), replyBuff, &reply_len, NULL);

    if (ret == -2)
    {
        MKSLOG_RED("Command timed out.");
    }
    else if (ret < 0)
    {
        MKSLOG_RED("Command failed.");
    }
    else if (ret == 0)
    {
        replyBuff[reply_len] = '\0';
        MKSLOG_YELLOW("返回的消息: %s", replyBuff);
    }
    return ret;
}

// 开启wifi
int mks_wpa_enable_network()
{
    char cmd[64];
    char replyBuff[2048] = {"\0"};
    size_t reply_len;
    int ret;

    if (!ctrl_conn)
    {
        MKSLOG_BLUE("Open wpa control interfaces failed!\n");
        return -3;
    }

    system("ip link set wlan0 up");

    // 启用网络
    memset(cmd, 0x00, sizeof(cmd));
    snprintf(cmd, sizeof(cmd) - 1, "ENABLE_NETWORK 0");
    memset(replyBuff, 0x00, sizeof(replyBuff));
    reply_len = sizeof(replyBuff) - 1;
    ret = wpa_ctrl_request(ctrl_conn, cmd, strlen(cmd), replyBuff, &reply_len, NULL);
    
    if (ret < 0)
    {
        MKSLOG_RED("ENABLE_NETWORK 0 failed.");
        return ret;
    }
    else
    {
        replyBuff[reply_len] = '\0';
        MKSLOG_YELLOW("ENABLE_NETWORK 返回的消息: %s", replyBuff);
    }
    // 立即重新关联（尝试连接）
    memset(cmd, 0x00, sizeof(cmd));
    snprintf(cmd, sizeof(cmd) - 1, "REASSOCIATE");
    memset(replyBuff, 0x00, sizeof(replyBuff));
    reply_len = sizeof(replyBuff) - 1;
    ret = wpa_ctrl_request(ctrl_conn, cmd, strlen(cmd), replyBuff, &reply_len, NULL);
    
    if (ret < 0)
    {
        MKSLOG_RED("REASSOCIATE failed.");
        return ret;
    }
    else
    {
        replyBuff[reply_len] = '\0';
        MKSLOG_YELLOW("REASSOCIATE 返回的消息: %s", replyBuff);
    }
    return ret;
}

// 关闭wifi
int mks_disable_network()
{
    char cmd[] = "DISCONNECT"; //"DISABLE_NETWORK 0";
    char replyBuff[2048];

    system("ip link set wlan0 down");
    return 0;

    size_t reply_len = sizeof(replyBuff) - 1;

    if (!ctrl_conn)
    {
        MKSLOG_RED("Open wpa control interfaces failed!");
        return -3;
    }
    MKSLOG_RED("发送cmd命令为: %s", cmd);
    int ret = wpa_ctrl_request(ctrl_conn, cmd, strlen(cmd), replyBuff, &reply_len, NULL);

    if (ret == -2)
    {
        MKSLOG_RED("Command timed out.");
    }
    else if (ret < 0)
    {
        MKSLOG_RED("Command failed.");
    }
    else if (ret == 0)
    {
        replyBuff[reply_len] = '\0';
        MKSLOG_YELLOW("返回的消息: %s", replyBuff);
    }

    return ret;
}

// 重新连接
int mks_reconnect()
{
    char cmd[] = "RECONNECT";
    char replyBuff[2048];
    size_t reply_len = sizeof(replyBuff) - 1;

    if (!ctrl_conn)
    {
        MKSLOG_RED("Open wpa control interfaces failed!");
        return -3;
    }
    MKSLOG_RED("发送cmd命令为: %s", cmd);
    int ret = wpa_ctrl_request(ctrl_conn, cmd, strlen(cmd), replyBuff, &reply_len, NULL);

    if (ret == -2)
    {
        MKSLOG_RED("Command timed out.");
    }
    else if (ret < 0)
    {
        MKSLOG_RED("Command failed.");
    }
    else if (ret == 0)
    {
        replyBuff[reply_len] = '\0';
        MKSLOG_YELLOW("mks_reconnect返回的消息: %s", replyBuff);
    }

    return ret;
}

// 获取状态
int mks_status()
{
    char cmd[] = "STATUS";
    char replyBuff[2048];
    size_t reply_len = sizeof(replyBuff) - 1;
    char *p_str;

    if (!ctrl_conn)
    {
        MKSLOG_RED("Open wpa control interfaces failed!");
        return -3;
    }
    // MKSLOG_RED("发送cmd命令为: %s", cmd);
    int ret = wpa_ctrl_request(ctrl_conn, cmd, strlen(cmd), replyBuff, &reply_len, NULL);

    if (ret == -2)
    {
        MKSLOG_RED("Command timed out.");
    }
    else if (ret < 0)
    {
        MKSLOG_RED("Command failed.");
    }
    else if (ret == 0)
    {
        replyBuff[reply_len] = '\0';
        // MKSLOG_YELLOW("mks_status返回的消息: %s", replyBuff);

        // 解析status返回信息
        parse_status((char *)replyBuff);
        if (strstr(replyBuff, "wpa_state=COMPLETED")) // 已连接
        {
            wpa_state = CONNECTED;
            // MKSLOG_HIGHLIGHT("当前状态为已连接");
        }
        else if (strstr(replyBuff, "wpa_state=DISCONNECTED"))
        {
            wpa_state = DISCONNECTED;
            // MKSLOG_HIGHLIGHT("当前状态为已断开连接");
        }
    }

    return ret;
}

// 解析"STATUS"指令返回字符串
void parse_status(char *buffer)
{
    struct mks_wifi_status_result_t *result = &status_result;
    char val[512] = {0};
    char *p_buffer;

    result_get(buffer, (char *)"bssid", result->bssid, sizeof(result->bssid));
    result_get(buffer, (char *)"freq", val, sizeof(val));
    result->freq = strtol(val, NULL, 10);

    p_buffer = strstr(buffer, "bssid"); // 避免ssid匹配为bssid的字符串
    if (p_buffer != NULL)
    {
        result_get(p_buffer + strlen("bssid"), (char *)"ssid", result->ssid, sizeof(result->ssid));
    }
    result_get(buffer, (char *)"id", val, sizeof(val));
    result->id = strtol(val, NULL, 10);
    result_get(buffer, (char *)"mode", result->mode, sizeof(result->mode));
    result_get(buffer, (char *)"pairwise_cipher", result->pairwise_cipher, sizeof(result->pairwise_cipher));
    result_get(buffer, (char *)"group_cipher", result->group_cipher, sizeof(result->group_cipher));
    result_get(buffer, (char *)"key_mgmt", result->key_mgmt, sizeof(result->key_mgmt));
    result_get(buffer, (char *)"wpa_state", result->wpa_state, sizeof(result->wpa_state));
    result_get(buffer, (char *)"ip_address", result->ip_address, sizeof(result->ip_address));
    p_buffer = strstr(buffer, "ip_address"); // 避免address匹配为ip_address的字符串
    if (p_buffer != NULL)
    {
        result_get(p_buffer + strlen("ip_address"), (char *)"address", result->address, sizeof(result->address));
    }
    result_get(buffer, (char *)"uuid", result->uuid, sizeof(result->uuid));
}

// 获取参数
static void result_get(char *str, char *key, char *val, int val_len)
{
    char *s;

    if (!(s = strstr(str, key)))
    {
        return;
    }

    if (!(s = strchr(s, '=')))
    {
        return;
    }

    s++;

    while (*s != '\n' && *s != '\0' && val_len > 1)
    {
        *val++ = *s++;
        val_len--;
    }
    *val = '\0';
}
