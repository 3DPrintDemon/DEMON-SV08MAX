#include <fstream>

#include "include/websocket_client.h"
#include "include/MoonrakerAPI.h"
#include "include/MakerbaseIPC.h"
#include "include/MakerbaseSerial.h"
#include "include/process_messages.h"
#include "include/MakerbasePanel.h"
#include "include/MakerbaseParseIni.h"

#include "include/KlippyGcodes.h"
#include "include/file_list.h"
#include "include/mks_log.h"
#include "include/mks_init.h"
#include "include/mks_gpio.h"
#include "include/mks_update.h"
#include "include/refresh_ui.h"
#include "include/ui.h"
#include "include/send_msg.h"
#include "include/KlippyRest.h"
#include "include/mks_wpa_cli.h"
#include "include/event.h"
#include "include/mks_hardware_test.h"



int main(int argc, char **argv)
{
    mkslog_print("===================START APPLICATION===============\n");
    if (access("/dev/sda", F_OK) == 0)
    {
        if (access("/dev/sda1", F_OK) == 0)
        {
            if (access("/home/sovol/printer_data/gcodes/sda1", F_OK) != 0)
            {
                system("/usr/bin/systemctl --no-block restart makerbase-automount@sda1.service");
            }
        }
    }

    // 检测是否有垃圾文件夹
    if (open("/home/sovol/.cache/vscode-cpptools", O_RDWR) != -1)
    {
        system("sudo rm -rf /home/sovol/.cache/vscode-cpptools");
    }

    //判断是否进入测试模式
    if_test_mode();
    if(test_mode && (access("/home/sovol/众创klipper.tft", F_OK) == 0))
    {
        system("rm /home/sovol/众创klipper.tft -rf");
    }
    else
    {
        system("/home/sovol/uart; mv /home/sovol/众创klipper.tft /home/sovol/众创klipper.tft.bak");
    }

    pthread_t refresh_ui_thread;
    pthread_t update_check_thread;
    pthread_t sled1_thread;
    pthread_t wpa_cli_unsolicited_event;
    pthread_t sent_jpg_thread;
    pthread_t check_udisk_status;
    pthread_create(&sled1_thread, NULL, sled1_ctrl, NULL); // 创建led1控制线程
    // pthread_create(&update_check_thread, NULL, update_check, NULL);         // 创建检测U盘更新固件
    pthread_create(&wpa_cli_unsolicited_event, NULL, handle_message, NULL); // 创建WiFi处理线程
    pthread_create(&sent_jpg_thread, NULL, sent_jpg_thread_handle, NULL);   // 创建预览图线程
    pthread_create(&check_udisk_status, NULL, check_udisk_status_thread_handle, NULL); 

    // 初始化websocket
    WebSocketClient ws_client;
    ep = &ws_client;
    // 在单独的线程中启动WebSocket客户端
    std::thread client_thread([&ws_client]()
                              {
                                  ws_client.connect("ws://localhost:7125/websocket"); // 指定WebSocket服务器地址
                              });
    while (!(ep->get_status()))
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }

    mkslog_print("Websocket connected\n");
    /* 创建刷新的线程 */
    std::thread message_thread(json_parse, std::ref(ws_client));
    // 重启固件
    reset_firmware();

    mkslog_print("Reset firmware finished\n");
    // 创建屏幕刷新线程
    pthread_create(&refresh_ui_thread, NULL, refresh_ui_pth, NULL);

    // 初始化屏幕串口
    int fd; // 串口文件描述符, sv08: ttyS3, sv08max: ttyS4
    if ((fd = open("/dev/ttyS4", O_RDWR | O_NDELAY | O_NOCTTY)) < 0)
    {
        mkslog_print("Open tty failed\n");
    }
    else
    {
        mkslog_print("Open tty success\n");
        tty_fd = fd;
        // 配置串口（波特率115200，8位数据位，无奇偶校验，停止位1）
        set_option(fd, 115200, 8, 'N', 1);
        try
        {
            fcntl(fd, F_SETFL, FNDELAY);
            usleep(3000);
            page_to(TJC_PAGE_STARTING);
            page_to(TJC_PAGE_STARTING);
        }
        catch (const std::exception &e)
        {
            std::cerr << "Page main error, " << e.what() << '\n';
        }
    }

    // 打开wpa客户端
    mks_wpa_cli_open_connection();
    mks_wpa_enable_network(); // 使能wpa
    mks_wpa_scan_scan_r(); // 扫描wifi

    // 轮询处理屏幕交互信息
    int count;
    char buff[4096];
    while (1)
    {
        if ((count = read(fd, buff, sizeof(buff))) > 0)
        {
            char *cmd = buff;
            parse_cmd_msg_from_tjc_screen(cmd); //
            memset(buff, 0, sizeof(buff));
        }
        usleep(5000);
    }
    close(fd);
    return 0;
}
