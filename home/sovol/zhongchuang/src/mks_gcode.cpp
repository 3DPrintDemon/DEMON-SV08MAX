#include <unistd.h>

#include <sstream>
#include <regex>

#include "../include/MakerbaseCommand.h"
#include "../include/MoonrakerAPI.h"
#include "../include/mks_log.h"
#include "../include/mks_gcode.h"
#include "../include/event.h"
#include "../include/refresh_ui.h"
#include "../include/mks_reprint.h"

// pid
bool printer_pid_finished = false;
std::string one_time_passcode = "";
std::string one_time_passlink = "";

// 解析web端返回信息
void parse_gcode_response(nlohmann::json params)
{
    std::string msg;

    if (params[0] != nlohmann::detail::value_t::null)
    {
        msg = params[0];
        MKSLOG("%s", msg.c_str());
    }

    if (strstr(msg.c_str(), "!! Move out of range:") != NULL)
    {
        page_to(TJC_PAGE_MOVE_ERROR);
        send_cmd_txt(tty_fd, "t0", msg);
        return;
    }

    if (strstr(msg.c_str(), "one_time_passcode: ") != NULL) {
        size_t pos = msg.find(": ");
        one_time_passcode = msg.substr(pos + 2);
        MKSLOG_YELLOW("+++one_time_passcode:%s", one_time_passcode.c_str());
    }

    if (strstr(msg.c_str(), "one_time_passlink: ") != NULL) {
        size_t pos = msg.find(": ");
        one_time_passlink = msg.substr(pos + 2);
        MKSLOG_YELLOW("+++one_time_passlink:%s", one_time_passlink.c_str());
    }

    if (strstr(msg.c_str(), "PID parameters") != NULL) {
        printer_pid_finished = true;
    }

    // 报错信息
    if (strstr(msg.c_str(), "!! ") != NULL)
    {
        MKSLOG_HIGHLIGHT("%s", msg.c_str());
        if (current_page_id != TJC_PAGE_ERR_TIPS && current_page_id != TJC_PAGE_ERROR_RESTART && current_page_id != TJC_PAGE_ERROR_RESET_FILE)
        {
            MKSLOG_YELLOW("page before err tips: %d", current_page_id);
            force_page_to(TJC_PAGE_ERR_TIPS);
            send_cmd_txt(tty_fd, "t0", msg);
        }
    }

    // Klipper启动成功
    else if (strstr(msg.c_str(), "Klipper state: Ready") != NULL)
    {
        webhooks_state = "ready";
        usleep(4000);
        sub_object_status();
    }
    // Klipper断开连接
    else if (strstr(msg.c_str(), "Klipper state: Disconnect") != NULL)
    {
        webhooks_state = "disconnected";
    }
    // Klipper关机
    else if (strstr(msg.c_str(), "Klipper state: Shutdown") != NULL)
    {
        webhooks_state = "shutdown";
    }
    // 打印结束
    else if (strstr(msg.c_str(), "Done printing file") != NULL)
    {
        current_print_state = "complete";
    }
    // 没进去调平状态直接点调整zoffset
    else if (strstr(msg.c_str(), "// Unknown command:\"TESTZ\"") != NULL)
    {
        page_to(TJC_PAGE_ERR_TIPS);
        send_cmd_txt(tty_fd, "t0", "Unknown command:TESTZ");
        level_zoffset_value = 0;
    }
    else if (strstr(msg.c_str(), "// probe: z_offset: ") != NULL)
    {
        MKSLOG_RED("得到z_offset: %s", msg.substr(20).c_str());
        float temp;
        std::stringstream ss;
        ss << msg.substr(20);
        ss >> temp;
        // temp = -temp;
        std::string value = std::to_string(temp);
        babystep = value.substr(0, value.find(".") + 4);
    }
    else if (strstr(msg.c_str(), "echo: ") != NULL)
    {
        if (get_zoffset_flag)
        {
            get_zoffset_flag = false;
            // MKSLOG_RED("得到z_offset: %s", msg.substr(6).c_str());
            float temp;
            std::stringstream ss;
            ss << msg.substr(6);
            ss >> temp;
            // temp = -temp;
            std::string value = std::to_string(temp);
            babystep = value.substr(0, value.find(".") + 4);
        }
    }
    else if (strstr(msg.c_str(), "Mesh Bed Leveling Complete") != NULL)
    {
        auto_level_complete_flag = true;
    }
    // 发起打印
    if (strstr(msg.c_str(), "File opened:") != NULL)
    {
        size_t fileOpenedPos = msg.find("File opened:");
        size_t sizePos = msg.find("Size:");
        if (fileOpenedPos != std::string::npos && sizePos != std::string::npos)
        {
            std::string extractedString = msg.substr(fileOpenedPos + strlen("File opened:"), sizePos - (fileOpenedPos + strlen("File opened:")));
            
            delet_pic("ram/160.jpg");

            current_print_state = "printing";

            get_object_status();

            // 去除前导和尾随空格
            size_t firstNonSpace = extractedString.find_first_not_of(" ");
            size_t lastNonSpace = extractedString.find_last_not_of(" ");

            if (firstNonSpace != std::string::npos && lastNonSpace != std::string::npos)
            {
                extractedString = extractedString.substr(firstNonSpace, lastNonSpace - firstNonSpace + 1);
            }
            else
            {
                extractedString.clear(); // 如果没有非空格字符，则清空提取的字符串
            }

            std::cout << "Extracted string: " << extractedString << std::endl;
            _get_gcode_metadata(extractedString);

            //备份再次打印路径
            current_select_file_path = extractedString;
            MKSLOG_HIGHLIGHT("current_select_file_path = %s", current_select_file_path.c_str());


            mkslog_print("发起打印了打印！！！");
            // 提前删除打印界面的预览图，防止进入打印页面时闪一下
            page_to(TJC_PAGE_PRINTING);
            page_to(TJC_PAGE_PRINTING);
            send_cmd_picc_picc2(tty_fd, "b3", "33", "35");
            // babysetp
            printing_babystep = 0.0;
            operate_zoffset_step = 0.01;
            operate_percent_step = 1;
            check_filament_flag = true;
            printer_display_status_progress_back = 0;

            // 流速
            current_move_extrude_factor = 1.0;
            set_printer_flow(current_move_extrude_factor * 100);
            // 打印速度
            current_move_speed_factor = 1.0;
            set_printer_speed(current_move_speed_factor * 100);

            // 解决从网页端打印本地文件的预览图显示问题
            if(strstr(extractedString.c_str(), "plr/") != NULL)
            {
                extractedString.replace(0, extractedString.find("plr/")+4, "");
            }
            std::string dir_path = getParentDirectory(extractedString);
            extractedString.replace(0, dir_path.length(), "");
            MKSLOG_BLUE("dir_path = %s", dir_path.c_str());
            MKSLOG_BLUE("extractedString = %s", extractedString.c_str());
            std::string dirPath = (std::string)"dir_path: " + dir_path + ", extractedString: " + extractedString;
            mkslog_print(dirPath.c_str());
            
            png_to_160_jpg(LOCAL_PATH + std::string("/") + dir_path, extractedString);
            begin_show_160_jpg = true;
            // run_a_gcode("PRINT_START"); // 刚开始打印发个宏
            save_reprint_parameter("IF_REPRINT", 1.0f);
            run_a_gcode("save_last_file");
        }
        else
        {
            mkslog_print("String format is not as expected.");
        }
    }
}
