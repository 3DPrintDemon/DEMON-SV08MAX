#include "../include/ui.h"
#include <sys/statvfs.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <filesystem>
#include <cerrno>
#include <atomic>
#include <chrono>

int tty_fd = -1;      // 屏幕串口描述符
int current_page_id;  // 当前页面的id号
int previous_page_id; // 上一页面的id号
int filament_change_type = 0;

/*打印操作页面步进值*/
float operate_zoffset_step = 0.01;
float operate_percent_step = 1;
float printing_babystep = 0.0;

/*移动页面步进值*/
float move_step = 1.0;
/*调平界面zoffset步进值*/
float level_zoffset_step = 0.01;
float level_zoffset_value;
/*wifi界面*/
bool wifi_enable = true;
bool led_enable = true;

bool is_long_out = false;

/*新增sled控制*/
bool sled1 = true;
bool sled2 = false;
bool sled3 = false;
bool open_beep_flag = true;
bool open_timelapse_flag = false; // 是否开启延时摄影
bool check_filament_flag = false;

float prev_current_fan_speed;
float prev_generic_fan1_speed;
float prev_generic_fan2_speed;
float prev_generic_fan3_speed;

std::string copyProgress = "0%";
std::string copySpeed = "0 MB/s";

int language;

//解析屏幕交互
void parse_cmd_msg_from_tjc_screen(char *cmd)
{
    uint32_t event_id = cmd[0];
    MKSLOG_GREEN("0x%x 0x%x 0x%x 0x%x ", cmd[0], cmd[1], cmd[2], cmd[3]);
    switch (event_id)
    {
    case 0x65:
        tjc_event_clicked_handler(cmd[1], cmd[2], cmd[3]); // 按下事件处理
        break;

    case 0x70:
        tjc_event_wifi_keyboard(cmd); // wifi键盘返回值处理
        break;

    case 0x71:
        tjc_event_setted_handler(cmd[1], cmd[2], cmd[3], cmd[4]); // 进度条、数字键盘返回值处理
        break;
    
    case 0x91:
        page_to(current_page_id); // 用于屏幕热插拔恢复
        break;

    case 0xfe:
        get_0xfe = true;
        // MKSLOG_RED("0x%x 0x%x 0x%x 0x%x ", cmd[0], cmd[1], cmd[2], cmd[3]);
        break;

    case 0x04:
        get_0x04 = true;
        MKSLOG_RED("0x%x", cmd[0]);
        break;

    case 0x05:
        get_0x05 = true;
        // MKSLOG_RED("0x%x", cmd[0]);
        break;

    case 0x06:
        get_0x06 = true;
        // MKSLOG_RED("0x%x 0x%x 0x%x 0x%x ", cmd[0], cmd[1], cmd[2], cmd[3]);
        break;

    case 0xfd:
        get_0xfd = true;
        // MKSLOG_RED("0x%x 0x%x 0x%x 0x%x ", cmd[0], cmd[1], cmd[2], cmd[3]);
        break;

    default:
        MKSLOG_RED("0x%x 0x%x 0x%x 0x%x ", cmd[0], cmd[1], cmd[2], cmd[3]);
        break;
    }
    cmd = NULL;
}

void page_to_wifi_list()
{
    if (is_errortip_show)
    {
        page_to(TJC_PAGE_ERROR_NETWORK);
    }else {
        if (file_exists(GUIDE_PATH))
        {
            page_to(TJC_PAGE_WIFI_LIST);                    // 跳转到wifi页面
        }else {
            page_to(TJC_PAGE_GUIDE_WIFI);
        }
    }
}

/* 界面跳转函数 */
void page_to(int page_id)
{
    if (tjc_ui_mutex.try_lock())
    {
        MKSLOG_GREEN("page to→ %d", page_id);
        if (page_id != current_page_id)
        {
            previous_page_id = current_page_id;
            current_page_id = page_id;
        }
        send_cmd_page(tty_fd, std::to_string(page_id));
        tjc_ui_mutex.unlock();
    }
}


/* 强制界面跳转函数，不等待页面刷新 */
void force_page_to(int page_id)
{
    tjc_ui_mutex.unlock();
    MKSLOG_GREEN("page to→ %d", page_id);
    if (page_id != current_page_id)
    {
        previous_page_id = current_page_id;
        current_page_id = page_id;
    }
    send_cmd_page(tty_fd, std::to_string(page_id));
}

void tjc_file_copy_tip_handler(int widget_id)
{
    switch (widget_id)
    {
    case ABOUT_YES:
        page_to(TJC_PAGE_FILE_LIST);
        file_list_init();
        break;
    }
}

// void tjc_print_filament_handler(int widget_id)
// {
//     switch (widget_id)
//     {
//     case PRINTING_FILAMENT_BACK:
//         page_to(TJC_PAGE_PRINTING);
//         break;
//     case PRINTING_FILAMENT_IN:
//         if (current_print_state == "printing")
//         {
//             MKSLOG_HIGHLIGHT("暂停打印");
//             run_a_gcode("PAUSE");
//             page_to(TJC_PAGE_PAUSEING);
//         }else{
//             run_a_gcode("LOAD_FILAMENT");
//             page_to(TJC_PAGE_LOADING);
//         }
//         break;
//     case PRINTING_FILAMENT_OUT:
//         is_long_out = false;
//         page_to(TJC_PAGE_IF_CHANGE_FILAMENT);
//         break;

//     case PRINTING_FILAMENT_LONG_OUT:
//         is_long_out = true;
//         page_to(TJC_PAGE_IF_CHANGE_FILAMENT);
//         break;
//     }
// }


void tjc_if_fil_success_refresh(int widget_id)
{
    switch (widget_id)
    {
        case YES:
            is_retry = false;
            filState = FIL_COMPLETE;
            page_to(TJC_PAGE_FIL_FILAMENT);
            break;

        case NO:
            is_retry = true;
            filState = FIL_NEW_FILAMENT;
            page_to(TJC_PAGE_FIL_FILAMENT);
            break;
    }
}

// confirm fill the filament ?
void tjc_sure_fill_in_refreash(int widget_id)
{
    switch (widget_id)
    {
        case YES:
            is_continue = true;
            if (!filament_switch_sensor_filament_detected)
            {
                is_runing = false;
            }
            page_to(TJC_PAGE_FIL_FILAMENT);
            break;
    }
}

/* 串口屏按下事件处理函数 */
void tjc_event_clicked_handler(int page_id, int widget_id, int type_id)
{
    // 蜂鸣器
    if (open_beep_flag)
    {
        send_cmd_beep(tty_fd, "100");
    }

    // 导航栏与菜单栏处理
    tjc_menu_handler(widget_id);

    switch (page_id)
    {
    case TJC_PAGE_MKS_HARDWARE_TEST:
        tjc_mks_test_handle(widget_id);
        break;

    case TJC_PAGE_ERROR_RESTART:
        tjc_error_restart_handler(widget_id);
        break;

    case TJC_PAGE_ERROR_RESET_FILE:
        tjc_error_reset_file_handler(widget_id);
        break;

    case TJC_PAGE_ERR_TIPS:
        tjc_error_tips_handler(widget_id);
        break;

    case TJC_PAGE_MOVE_ERROR:
        tjc_move_error_handler(widget_id);
        break;

    case TJC_PAGE_GUIDE_LANGUAGE:
        tjc_guide_language_handler(widget_id, type_id);
        break;

    case TJC_PAGE_GUIDE_WIFI:
    case TJC_PAGE_ERROR_NETWORK:
        // tjc_guide_wifi_handler(widget_id);
        tjc_wifi_list_handler(widget_id);
        break;

    case TJC_PAGE_GUIDE_CALIBRATION:
        tjc_guide_calibration_handler(widget_id);
        break;

    case TJC_PAGE_IP_POP:
        tjc_ip_pop_handler(widget_id);
        break;

    case TJC_PAGE_IF_REPRINT:
        tjc_if_reprint(widget_id);
        break;

    case TJC_PAGE_MAIN:
        tjc_main_handler(widget_id);
        break;

    case TJC_PAGE_FILE_LIST:
        tjc_file_list_handler(widget_id);
        break;

    case TJC_PAGE_TIMELAPSE_LIST:
        tjc_timelapse_list_handler(widget_id);
        break;

    case TJC_PAGE_IF_EXPORT:
        tjc_if_export_handler(widget_id);
        break;

    case TJC_PAGE_EXPORT_SUCCEED:
        tjc_export_succeed_handler(widget_id);
        break;

    case TJC_PAGE_EXPORT_FAILED:
        tjc_export_failed_handler(widget_id);
        break;

    case TJC_PAGE_IF_DELETE_TIMELAPSE_FILE:
        tjc_if_delete_timelapse_file_handler(widget_id);
        break;

    case TJC_PAGE_IF_DELETE_FILE:
        tjc_if_delete_file_handler(widget_id);
        break;

    case TJC_PAGE_PREVIEW:
        tjc_preview_handler(widget_id);
        break;

    case TJC_PAGE_PRINTING:
        tjc_printing_handler(widget_id);
        break;

    case TJC_PAGE_OPERATE:
        tjc_operate_handler(widget_id);
        break;

    case TJC_PAGE_IF_CHANGE_FILAMENT:
        tjc_if_change_filament_handler(widget_id);
        break;

    case TJC_PAGE_FILAMENT_ERROR:
        tjc_filament_error_handler(widget_id);
        break;

    case TJC_PAGE_UNLOAD_FINISH:
        tjc_unload_finish_handler(widget_id);
        break;

    case TJC_PAGE_LOAD_BEGIN:
        tjc_load_begin_handler(widget_id);
        break;

    case TJC_PAGE_NO_FILAMENT:
        tjc_no_filament_handler(widget_id);
        break;

    case TJC_PAGE_LOAD_FINISH:
        tjc_load_finish_handler(widget_id);
        break;

    case TJC_PAGE_LOAD_CLEAR:
        tjc_load_clear_handler(widget_id);
        break;

    case TJC_PAGE_IF_STOP:
        tjc_if_stop_handler(widget_id);
        break;

    case TJC_PAGE_PRINT_FILISH:
        tjc_print_finish_handler(widget_id);
        break;

    case TJC_PAGE_MOVE:
        tjc_move_handler(widget_id);
        break;

    case TJC_PAGE_PRINT_FILAMENT:
    case TJC_PAGE_FILAMENT:
        tjc_filament_handler(widget_id);
        break;

    case TJC_PAGE_HEAT_TIP:
        tjc_heat_tip_handler(widget_id);
        break;

    case TJC_PAGE_FAN:
        // tjc_fan_hangler(widget_id);
        break;

    case TJC_PAGE_LOAD_CLEAR2:
        tjc_load_clear2_handler(widget_id);
        break;

    case TJC_PAGE_KEYBOARD:
        tjc_keyboard_handler(widget_id);
        break;

    case TJC_PAGE_SYSTEM:
        tjc_system_handler(widget_id);
        break;

    case TJC_PAGE_LANGUGAE:
        tjc_language_handler(widget_id);
        break;

    case TJC_PAGE_IF_RESET:
        tjc_if_factory_handler(widget_id);
        break;

    case TJC_PAGE_IF_UPDATE:
        tjc_if_update_handler(widget_id);
        break;

    case TJC_PAGE_ABOUT:
        tjc_about_hander(widget_id);
        break;

    case TJC_PAGE_WIFI_LIST:
        tjc_wifi_list_handler(widget_id);
        break;

    case TJC_PAGE_WIFI_KB:
        tjc_wifi_kb_handler(widget_id);
        break;

    case TJC_PAGE_WIFI_SUCCEED:
        tjc_wifi_connect_succeed(widget_id);
        break;

    case TJC_PAGE_WIFI_FAILED:
        tjc_wifi_connect_failed(widget_id);
        break;

    case TJC_PAGE_LEVEL_MODE:
        tjc_level_mode_handler(widget_id);
        break;

    case TJC_PAGE_ZOFFSET:
        tjc_zoffset_handler(widget_id);
        break;   
    
    case TJC_PAGE_FAN_LED:
    case TJC_PAGE_FAN_LED_NOCB:
        tjc_fan_led_handler(widget_id);
        break;    
        
    case TJC_PAGE_CALIBRATION:
        tjc_calibration_handler(widget_id);
        break;

    case TJC_PAGE_OBICO:
        tjc_obico_handler(widget_id);
        break;

    case TJC_PAGE_REPRINT_TIP:
        tjc_reprint_tip_handler(widget_id);
        break;

    case TJC_PAGE_FILE_COPYING_ERR:
        tjc_file_copy_tip_handler(widget_id);
        break;

    // case TJC_PAGE_PRINT_FILAMENT:
    //     tjc_print_filament_handler(widget_id);
    //     break;

    case TJC_PAGE_OBICO_UNAVAIABLE:
        switch (widget_id)
        {
        case IP_POP_OK:
            page_to(TJC_PAGE_SYSTEM);
            break;
        }
        break;

    case TJC_PAGE_PLUG_TIP:
        page_to(TJC_PAGE_PRINTING);
        break;

    case TJC_PAGE_WINDING_TIP:
        page_to(TJC_PAGE_PRINTING);
        break;

    case TJC_PAGE_PAUSE_TIP:
        page_to(TJC_PAGE_PRINTING);
        break;

    case TJC_PAGE_SURE_FILL_IN:         // step 2: ensure filament in
        tjc_sure_fill_in_refreash(widget_id);
        break;

    case TJC_PAGE_IF_FIL_SUCCESS:       // step 3: if need retry
        tjc_if_fil_success_refresh(widget_id);
        break;

    case TJC_PAGE_CHAMBER_TIP:
        switch (widget_id) {
            case YES:
                page_to(previous_page_id);
                break;
        }
        break;
    }
}

void reset_prev_fan_value()
{
    prev_current_fan_speed = 0.0;
    prev_generic_fan1_speed = 0.0;
    prev_generic_fan2_speed = 0.0;
    prev_generic_fan3_speed = 0.0;
}

void tjc_fan_led_handler(int widget_id)
{
    switch (widget_id) {
    case TJC_FAN_LED_CLOSE:
        reset_prev_fan_value();
        page_to(previous_page_id);
        break;
    case TJC_FAN_LED_SWITCH:
        led_enable = !led_enable;
        led1_on_off();
        break;
    }
}

void tjc_calibration_handler(int widget_id)
{
    switch (widget_id) {
    case CALIBRATION_BACK:
        page_to(TJC_PAGE_SYSTEM);
        break;
    case CALIBRATION_OK:
        MKSLOG_YELLOW("hasEddyCb:%d, hasBedLevel:%d, hasVibration:%d, hasPID:%d", hasEddyCb, hasBedLevel, hasVibration, hasPID);
        state = EDDY_CALIBRATION;
        break;
    case CALIBRATION_GANTRY:
        hasEddyCb = !hasEddyCb;
        send_cmd_picc_picc2(tty_fd, "b2", hasEddyCb ? "96" : "97", hasEddyCb ? "96" : "97");
        break;
    case CALIBRATION_Z:
        hasBedLevel = !hasBedLevel;
        send_cmd_picc_picc2(tty_fd, "b3", hasBedLevel ? "96" : "97", hasBedLevel ? "96" : "97");
        break;
    case CALIBRATION_VIBRATION:
        hasVibration = !hasVibration;
        send_cmd_picc_picc2(tty_fd, "b4", hasVibration ? "96" : "97", hasVibration ? "96" : "97");
        break;
    case CALIBRATION_PID:
        hasPID = !hasPID;
        send_cmd_picc_picc2(tty_fd, "b5", hasPID ? "96" : "97", hasPID ? "96" : "97");
        break;
    }
}

void tjc_obico_handler(int widget_id)
{
    switch (widget_id) {
        case OBICO_OK:
        page_to(TJC_PAGE_SYSTEM);
        break;
        case OBICO_REFREASH:
        run_a_gcode("OBICO_LINK_STATUS");
        break;
    }
}

/* 导航栏与菜单栏处理 */
void tjc_menu_handler(int widget_id)
{
    static uint32_t last_tools_menu = TJC_PAGE_MOVE;     // 工具栏上一个界面
    static uint32_t last_setting_menu = TJC_PAGE_SYSTEM; // 设置栏上一个界面

    switch (widget_id)
    {
    case TJC_MENU_MAIN:
        page_to(TJC_PAGE_MAIN);
        break;

    case TJC_MENU_FILE:
        if (current_page_id != TJC_PAGE_FILE_LIST)
        {
            file_list_init();
        }
        break;

    case TJC_MENU_TOOLS:
        page_to(last_tools_menu); // 返回上一次工具菜单栏
        break;

    case TJC_MENU_SETTING:
        if (last_setting_menu == TJC_PAGE_WIFI_SCANING)
        {
            wifi_list_init();
        }
        else
        {
            page_to(last_setting_menu); // 返回上一次设置菜单栏
        }
        break;

    case TJC_MENU_LOCAL:
        choose_udisk_flag = false;
        update_disk_pic();
        file_list_init();
        break;

    case TJC_MENU_UDISK:
        choose_udisk_flag = true;
        update_disk_pic();
        if (if_udisk_insert() == true) // 有U盘挂载
        {
            file_list_init();
        }
        else // 无U盘挂载
        {
            page_to(TJC_PAGE_NO_UDISK);
        }
        break;

    case TJC_MENU_TIMELAPSE:
        // timelapse_list_init();
        break;

    case TJC_MENU_MOVE:
        page_to(TJC_PAGE_MOVE);
        last_tools_menu = TJC_PAGE_MOVE;
        break;

    case TJC_MENU_FILAMENT:
        page_to(TJC_PAGE_FILAMENT);
        last_tools_menu = TJC_PAGE_FILAMENT;
        break;

    case TJC_MENU_FAN:
        // page_to(TJC_PAGE_FAN);
        // last_tools_menu = TJC_PAGE_FAN;
        break;

    case TJC_MENU_SYSTEM:
        page_to(TJC_PAGE_SYSTEM);
        last_setting_menu = TJC_PAGE_SYSTEM;
        break;

    case TJC_MENU_WIFI:
        last_setting_menu = TJC_PAGE_WIFI_SCANING;
        wifi_list_init();
        break;

    case TJC_MENU_LEVEL:
        // page_to(TJC_PAGE_LEVEL_MODE);
        // last_setting_menu = TJC_PAGE_LEVEL_MODE;
        break;
    }
}

void tjc_guide_language_handler(int widget_id, int type_id)
{
    switch (widget_id)
    {
    case GUIDE_LANGUAGE_NEXT_STEP:
        language = type_id;
        MKSLOG_YELLOW("+++ language: %d", type_id);
        wifi_list_init();
        page_to(TJC_PAGE_GUIDE_WIFI);
        break;
    }
}

void tjc_guide_wifi_handler(int widget_id)
{
    switch (widget_id)
    {
    case GUIDE_WIFI_PREV_STEP:

        break;

    case GUIDE_WIFI_PREV_PAGE:

        break;

    case GUIDE_WIFI_FINISH:

        break;
    }
}

void tjc_ip_pop_handler(int widget_id)
{
    switch (widget_id)
    {
    case IP_POP_OK:
        if (previous_page_id != TJC_PAGE_MAIN && previous_page_id != TJC_PAGE_PRINTING)
        {
            page_to(TJC_PAGE_MAIN);
        }
        else
        {
            page_to(previous_page_id);
        }
        break;
    }
}

void tjc_if_reprint(int widget_id)
{
    switch (widget_id)
    {
    case IF_PRINT_YES:
        start_plr();
        page_to(TJC_PAGE_PRINTING);
        break;

    case IF_PRINT_NO:
        cancel_plr();
        page_to(TJC_PAGE_MAIN);
        break;
    }
}

void tjc_main_handler(int widget_id)
{
    switch (widget_id)
    {
    case MAIN_EXTRUDE:
        page_to(TJC_PAGE_KEYBOARD);
        send_cmd_txt(tty_fd, "show", std::to_string(current_extruder_target));
        break;

    case MAIN_HOT_BED:
        page_to(TJC_PAGE_KEYBOARD);
        send_cmd_txt(tty_fd, "show", std::to_string(current_heater_bed_target));
        break;
    
    case MAIN_CHAMBER:
        if (has_chamber_temp)
        {
            page_to(TJC_PAGE_KEYBOARD);
            send_cmd_txt(tty_fd, "show", std::to_string(current_chamber_target));
        }else{
            page_to(TJC_PAGE_CHAMBER_TIP);
        }
        break;

    case MAIN_LED:
        if (has_chamber_fan)
        {
            MKSLOG_YELLOW("page to fan_led");
            page_to(TJC_PAGE_FAN_LED);
        }else {
            MKSLOG_YELLOW("page to fan_led_nocb");
            page_to(TJC_PAGE_FAN_LED_NOCB);
        }
        break;

    case MAIN_WIFI:
        page_to(TJC_PAGE_IP_POP);
        break;
    }
}

void tjc_file_list_handler(int widget_id)
{
    switch (widget_id)
    {
    case FILE_LIST_FILE0:
    case FILE_LIST_FILE1:
    case FILE_LIST_FILE2:
    case FILE_LIST_FILE3:
    case FILE_LIST_FILE4:
    case FILE_LIST_FILE5:
        click_dir_or_file(widget_id - FILE_LIST_FILE0);
        break;

    case FILE_LIST_PREV:
        if (page_files_current_pages > 0)
        {
            page_files_current_pages--;
            get_page_files(page_files_current_pages);
            display_page_files();
        }
        break;

    case FILE_LIST_NEXT:
        if (page_files_current_pages < page_files_total_pages)
        {
            page_files_current_pages++;
            get_page_files(page_files_current_pages);
            display_page_files();
        }
        break;

    case FILE_LIST_BACK:
        if (page_files_folder_layers > 0)
        {
            back_to_parenet_dir();
        }
        break;

    case FILE_LIST_DELETE0:
    case FILE_LIST_DELETE1:
    case FILE_LIST_DELETE2:
    case FILE_LIST_DELETE3:
    case FILE_LIST_DELETE4:
    case FILE_LIST_DELETE5:
        delete_dir_or_file(widget_id - FILE_LIST_DELETE0);
        break;
    }
}

void tjc_if_delete_file_handler(int widget_id)
{
    switch (widget_id)
    {
    case IF_DELETE_YES:
        delete_file(delete_path.c_str());
        get_page_files(page_files_current_pages);
        display_page_files();
        break;

    case IF_DELETE_NO:
        get_page_files(page_files_current_pages);
        display_page_files();
        break;
    }
}


void tjc_timelapse_list_handler(int widget_id)
{
    switch (widget_id)
    {
    case TIMELAPSE_LIST_FILE0:
    case TIMELAPSE_LIST_FILE1:
    case TIMELAPSE_LIST_FILE2:
    case TIMELAPSE_LIST_FILE3:
    case TIMELAPSE_LIST_FILE4:
        click_timelapse_file(widget_id - TIMELAPSE_LIST_FILE0);
        break;

    case TIMELAPSE_LIST_PREV:
        if (timelapse_current_pages > 0)
        {
            timelapse_current_pages--;
            get_one_page_timelapse(timelapse_current_pages); // 获取第n页的文件类型和文件名
            display_timelapse_files();                       // 显示文件列表内容
        }
        break;

    case TIMELAPSE_LIST_NEXT:
        if (timelapse_current_pages < timelapse_total_pages)
        {
            timelapse_current_pages++;
            get_one_page_timelapse(timelapse_current_pages); // 获取第n页的文件类型和文件名
            display_timelapse_files();                       // 显示文件列表内容
        }
        break;

    case TIMELAPSE_LIST_DELETE0:
    case TIMELAPSE_LIST_DELETE1:
    case TIMELAPSE_LIST_DELETE2:
    case TIMELAPSE_LIST_DELETE3:
    case TIMELAPSE_LIST_DELETE4:
        click_delete_timelapse_file(widget_id - TIMELAPSE_LIST_DELETE0);
        break;
    }
}

void tjc_if_export_handler(int widget_id)
{
    switch (widget_id)
    {
    case IF_EXPORT_YES:
        if(if_udisk_insert())
        {
            export_timelapse_file();
            page_to(TJC_PAGE_EXPORT_SUCCEED);
        }
        else
        {
            page_to(TJC_PAGE_EXPORT_FAILED);
        }
        break;

    case IF_EXPORT_NO:
        get_one_page_timelapse(timelapse_current_pages); 
        display_timelapse_files();                       
        break;
    } 
}

void tjc_export_succeed_handler(int widget_id)
{
    switch (widget_id)
    {
    case EXPORT_SUCCEED_OK:
        get_one_page_timelapse(timelapse_current_pages); 
        display_timelapse_files();                       
        break;
    } 
}

void tjc_export_failed_handler(int widget_id)
{
    switch (widget_id)
    {
    case EXPORT_FAILED_OK:
        get_one_page_timelapse(timelapse_current_pages); 
        display_timelapse_files();                       
        break;
    } 
}

void tjc_if_delete_timelapse_file_handler(int widget_id)
{
    switch (widget_id)
    {
    case IF_DELETE_YES:
        delete_file(delete_path.c_str());
        get_one_page_timelapse(timelapse_current_pages); 
        display_timelapse_files();
        break;

    case IF_DELETE_NO:
        get_one_page_timelapse(timelapse_current_pages); 
        display_timelapse_files();   
        break;
    }
}

int copyFileIfSpaceAvailable(const std::string& filePath, const std::string& destDir) {
    struct stat stat_buf;
    if (stat(filePath.c_str(), &stat_buf) != 0) {
        return -1; // 文件不存在或无法访问
    }
    std::size_t fileSize = stat_buf.st_size;

    struct statvfs vfs;
    if (statvfs(destDir.c_str(), &vfs) != 0) {
        std::cerr << "Error getting disk space info for '" << destDir << "': " 
                  << strerror(errno) << " (errno " << errno << ")" << std::endl;
        return -200 - errno;
    }
    constexpr std::size_t MIN_FREE_SPACE = 50 * 1024 * 1024;
    std::size_t freeSpace = vfs.f_bavail * vfs.f_frsize;
    if (fileSize + MIN_FREE_SPACE > freeSpace) {
        return -3;
    }

    std::string fileName = filePath.substr(filePath.find_last_of("/") + 1);
    std::string newFilePath = destDir + "/" + fileName;

    int srcFd = open(filePath.c_str(), O_RDONLY);
    if (srcFd < 0) return -4;

    int destFd = open(newFilePath.c_str(), O_WRONLY | O_CREAT | O_TRUNC, stat_buf.st_mode);
    if (destFd < 0) {
        close(srcFd);
        return -5;
    }
    char buffer[4096];
    ssize_t bytesRead;
    std::size_t bytesCopied = 0;
    
    auto startTime = std::chrono::high_resolution_clock::now();
    auto lastUpdate = startTime;

    while ((bytesRead = read(srcFd, buffer, sizeof(buffer))) > 0) {
        if (write(destFd, buffer, bytesRead) != bytesRead) {
            close(srcFd);
            close(destFd);
            return -6;
        }
        bytesCopied += bytesRead;

        // 计算进度
        int progress = static_cast<int>((bytesCopied * 100) / fileSize);
        copyProgress = std::to_string(progress) + "%";
        send_cmd_txt(tty_fd, "t2", copyProgress);

        // 计算实时速度（每秒更新一次）
        auto now = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed = now - lastUpdate;
        
        double speedMBs = (bytesCopied / 1024.0 / 1024.0) / (std::chrono::duration<double>(now - startTime).count());
        copySpeed = std::to_string(static_cast<int>(speedMBs)) + "MB/s";  // 仅保留整数
        lastUpdate = now;
        send_cmd_txt(tty_fd, "t4", copySpeed);

        // 输出进度和速度（可选）
        std::cout << "\rCopying: " << copyProgress 
                  << " | Speed: " << copySpeed << std::flush;
    }
    close(srcFd);
    close(destFd);
    return 0;
}

bool startsWith(const std::string& path, const std::string& prefix) {
    return path.substr(0, prefix.length()) == prefix;
}

std::size_t fileSize2(const std::string& filePath) {
    struct stat stat_buf;
    if (stat(filePath.c_str(), &stat_buf) == 0) {
        return stat_buf.st_size;
    }
    return 0; // 文件不存在或无法访问
}

std::string extractFilenameWithoutExtension(const std::string& path) {
    size_t lastSlash = path.find_last_of("/"); // 找到最后一个 '/'
    size_t lastDot = path.find_last_of(".");   // 找到最后一个 '.'

    // 取出 '/' 之后的部分
    std::string filename = (lastSlash == std::string::npos) ? path : path.substr(lastSlash + 1);

    // 如果有 '.' 且它在 '/' 之后，则去掉扩展名
    if (lastDot != std::string::npos && lastDot > lastSlash) {
        filename = filename.substr(0, lastDot - lastSlash - 1);
    }

    return filename;
}

void tjc_preview_handler(int widget_id)
{
    switch (widget_id)
    {
    case PREVIEW_BACK:
        get_page_files(page_files_current_pages);
        display_page_files();
        break;

    case PREVIEW_START_PRINT:
        if (current_print_state != "standby")
        {
            run_a_gcode("SDCARD_RESET_FILE");
        }
        get_object_status();
        if (filament_switch_sensor_enabled && !filament_switch_sensor_filament_detected)
        {
            MKSLOG_HIGHLIGHT("打印前检测到无耗材");
            before_print_filament_error = true;
            page_to(TJC_PAGE_FILAMENT_ERROR);
        }
        else
        {
            // current_select_file_path = sda1/SOVOL-SV08-MAX_OrcaPlug_PLA_0.2_8m55s.gcode
            if (startsWith(current_select_file_path, "sda1/"))
            {
                page_to(TJC_PAGE_FILE_COPYING);
                usleep(500000);
                std::string srcPath = (std::string) LOCAL_PATH + "/" + current_select_file_path;
                MKSLOG_YELLOW("srcPath = %s", srcPath.c_str());

                // copy X-176x176.jpg to local
                std::string fileBaseName = extractFilenameWithoutExtension(current_select_file_path);
                std::string srcPicPath    = (std::string) UDISK_PATH + "/.thumbs/" + fileBaseName + std::string("-176x176.jpg");
                std::string srcPicPath160 = (std::string) UDISK_PATH + "/.thumbs/" + fileBaseName + std::string("-160x160.jpg");
                std::string srcPicPath64  = (std::string) UDISK_PATH + "/.thumbs/" + fileBaseName + std::string("-64x64.jpg");
                std::string destPicPath   = (std::string) LOCAL_PATH + "/.thumbs";
                MKSLOG_YELLOW("srcPicPath: %s, destPicPath: %s", srcPicPath.c_str(), destPicPath.c_str());
                
                // copy gcode to local
                int ret = copyFileIfSpaceAvailable(srcPath, LOCAL_PATH);
                if (ret == 0)  // copy success
                {
                    usleep(800000);
                    int pic_ret = copyFileIfSpaceAvailable(srcPicPath, destPicPath);
                    if (pic_ret != 0)
                    {
                        MKSLOG_YELLOW("176 pic_ret: %d", pic_ret);
                    }
                    usleep(100000);
                    int pic64_ret = copyFileIfSpaceAvailable(srcPicPath64, destPicPath);
                    if (pic64_ret != 0)
                    {
                        MKSLOG_YELLOW("64 pic_ret: %d", pic64_ret);
                    }
                    usleep(100000);
                    int pic160_ret = copyFileIfSpaceAvailable(srcPicPath160, destPicPath);
                    if (pic160_ret != 0)
                    {
                        MKSLOG_YELLOW("164 pic_ret: %d", pic160_ret);
                    }
                    usleep(400000);
                    size_t pos = current_select_file_path.find_last_of('/');
                    std::string fileName = (pos == std::string::npos) ? current_select_file_path 
                                                                       : current_select_file_path.substr(pos + 1);
                    MKSLOG_YELLOW("USB printing_path = %s", fileName.c_str());
                    start_printing(fileName);
                }else {
                    mkslog_print("copy file error");
                    page_to(TJC_PAGE_FILE_COPYING_ERR);
                }
            }else {
                MKSLOG_YELLOW("Local printing_path = %s", current_select_file_path.c_str());
                start_printing(current_select_file_path);
            }
        }
        break;

    // case PREVIEW_OPEN_TIMELAPSE:
        // open_timelapse_flag = !open_timelapse_flag;
        // if()
        // {
        //     system("sed -i 's/variable_area_detection_flag: 0/variable_area_detection_flag: 1/' /home/sovol/klipper_config/Macro.cfg; sync");
        // }
        // break;
    }
}

void tjc_printing_handler(int widget_id)
{
    MKSLOG_HIGHLIGHT("widget_id = %d", widget_id);
    switch (widget_id)
    {
    case PRINTING_LED_SWITCH:
        if (has_chamber_fan)
        {
            page_to(TJC_PAGE_FAN_LED);
        }else {
            page_to(TJC_PAGE_FAN_LED_NOCB);
        }
        break;

    case PRINTING_EXTRUDE:
        page_to(TJC_PAGE_KEYBOARD);
        send_cmd_txt(tty_fd, "show", std::to_string(current_extruder_target));
        break;
    
    case PRINTING_CHAMBER:
        if (has_chamber_temp)
        {
            page_to(TJC_PAGE_KEYBOARD);
            send_cmd_txt(tty_fd, "show", std::to_string(current_chamber_target));
        }else{
            page_to(TJC_PAGE_CHAMBER_TIP);
        }
        break;

    case PRINTING_HOT_BED:
        page_to(TJC_PAGE_KEYBOARD);
        send_cmd_txt(tty_fd, "show", std::to_string(current_heater_bed_target));
        break;

    case PRINTING_PAUSE_RESUME:
        if (current_print_state == "printing")
        {
            MKSLOG_HIGHLIGHT("暂停打印");
            run_a_gcode("PAUSE");
        }
        else if (current_print_state == "paused")
        {
            run_a_gcode("RESUME");
            check_filament_flag = true;
        }
        // send_cmd_tsw(tty_fd, "255", "0"); // 禁止屏幕触摸
        // sleep(1);
        // send_cmd_tsw(tty_fd, "255", "1"); // 使能屏幕触摸
        break;

    case PRINTING_STOP:
        page_to(TJC_PAGE_IF_STOP);
        break;

    case PRINTING_OPERATE:
        page_to(TJC_PAGE_OPERATE);
        break;

    case PRINTING_WIFI:
        page_to(TJC_PAGE_IP_POP);
        break;

    case PRINTING_FILAMENT:
        if (current_print_state == "printing") {
            page_to(TJC_PAGE_PAUSE_TIP);
        }else {
            page_to(TJC_PAGE_PRINT_FILAMENT);
        }
        break;
    }
}

void tjc_operate_handler(int widget_id)
{
    int percent = 0;

    switch (widget_id)
    {
    case OPERATE_BACK:
        page_to(TJC_PAGE_PRINTING);
        break;

    case OPERATE_0_0_1MM:
        operate_zoffset_step = 0.01;
        break;

    case OPERATE_0_1MM:
        operate_zoffset_step = 0.1;
        break;

    case OPERATE_0_5MM:
        operate_zoffset_step = 0.5;
        break;

    case OPERATE_ZOFFSET_SUB:
        set_zoffset(false, operate_zoffset_step);
        printing_babystep -= operate_zoffset_step;
        break;

    case OPERATE_ZOFFSET_ADD:
        set_zoffset(true, operate_zoffset_step);
        printing_babystep += operate_zoffset_step;
        break;

    case OPERATE_1_PERCENT:
        operate_percent_step = 1;
        break;

    case OPERATE_5_PERCENT:
        operate_percent_step = 5;
        break;

    case OPERATE_10_PERCENT:
        operate_percent_step = 10;
        break;

    case OPERATE_25_PERCENT:
        operate_percent_step = 25;
        break;

    case OPERATE_FLOW_SUB:
        percent = current_move_extrude_factor * 100 - operate_percent_step;
        if (percent < 80)
            percent = 80;
        set_printer_flow(percent);
        break;

    case OPERATE_FLOW_ADD:
        percent = current_move_extrude_factor * 100 + operate_percent_step;
        if (percent > 120)
            percent = 120;
        set_printer_flow(percent);
        break;

    case OPERATE_SPEED_SUB:
        percent = current_move_speed_factor * 100 - operate_percent_step;
        if (percent < 10)
            percent = 10;
        set_printer_speed(percent);
        break;

    case OPERATE_SPEED_ADD:
        percent = current_move_speed_factor * 100 + operate_percent_step;
        if (percent > 500)
            percent = 500;
        set_printer_speed(percent);
        break;

    case OPERATE_MODEL_FILAMENT:
        break;
    }
}

// chris todo
void tjc_if_change_filament_handler(int widget_id)
{
    switch (widget_id)
    {
    case YES:
        idle_timeout_state = "Printing";
        page_to(TJC_PAGE_UNLOADING);
        if (is_long_out)
        {
            run_a_gcode("M600");
            mkslog_print("run M600");
        }else {
            run_a_gcode("UNLOAD_FILAMENT");
        }
        break;

    case NO:
        page_to(TJC_PAGE_PRINT_FILAMENT);
        break;
    }
}


void tjc_filament_error_handler(int widget_id)
{
    switch (widget_id)
    {
    case FILAMENT_ERROR_STOP_PRINT:
        page_to(TJC_PAGE_FILAMENT);
        // if(before_print_filament_error)    //打印前没料点击换料，加热，需要弹窗
        // {
        //     file_list_init();
        //     before_print_filament_error = false;
        // }
        // else
        // {
        //     cancel_print();
        //     page_to(TJC_PAGE_STOPING);
        // }
        break;

    // case FILAMENT_ERROR_CHANGE_FILAMENT:
    //     if(before_print_filament_error)    //打印前没料点击换料，加热，需要弹窗
    //     {
    //         set_extruder_target(180);
    //         page_to(TJC_PAGE_HEATING);
    //     }
    //     else
    //     {
    //         page_to(TJC_PAGE_LOAD_BEGIN);       //打印中没料，直接退料，不需要弹窗
    //     }
    //     break;
    }
}


void tjc_unload_finish_handler(int widget_id)
{
    switch (widget_id)
    {
    case YES:
        page_to(TJC_PAGE_LOAD_BEGIN);
        break;
    }
}


void tjc_load_begin_handler(int widget_id)
{
    switch (widget_id)
    {
    case YES:
        if (filament_switch_sensor_enabled && !filament_switch_sensor_filament_detected)
        {
            page_to(TJC_PAGE_NO_FILAMENT);
        }
        else
        {
            idle_timeout_state = "Printing";
            run_a_gcode("LOAD_FILAMENT");    // DEFAULT_LOAD_FILAMENT
            page_to(TJC_PAGE_LOADING);
        }
        break;
    }
}


void tjc_no_filament_handler(int widget_id)
{
    switch (widget_id)
    {
    case YES:
        if (filament_switch_sensor_enabled && !filament_switch_sensor_filament_detected)
        {
            page_to(TJC_PAGE_NO_FILAMENT);
        }
        else
        {
            idle_timeout_state = "Printing";
            run_a_gcode("LOAD_FILAMENT");       // DEFAULT_LOAD_FILAMENT
            page_to(TJC_PAGE_FIL_FILAMENT);     // chris todo
        }
        break;
    }
}


void tjc_load_finish_handler(int widget_id)
{
    switch (widget_id)
    {
    case LOAD_FINISH_AGAIN:
        idle_timeout_state = "Printing";
        run_a_gcode("G91");
        run_a_gcode("G1 E10 F300");
        run_a_gcode("G90");
        page_to(TJC_PAGE_LOADING);
        break;

    case LOAD_FINISH_DONE:
        if(current_print_state == "paused")
        {
            run_a_gcode("G91");
            run_a_gcode("G1 E-3 F300");
            run_a_gcode("G90");
            page_to(TJC_PAGE_LOAD_CLEAR);       //打印中换料
        }
        else
        {
            set_extruder_target(0);
            page_to(TJC_PAGE_LOAD_CLEAR2);      //一键换料，打印前检测到无耗材
        }
        break;
    }
}

void tjc_load_clear_handler(int widget_id)
{
    switch (widget_id)
    {
    case LOAD_CLEAR_RESUME_PRINT:
        run_a_gcode("G91");
        run_a_gcode("G1 E3 F300");
        run_a_gcode("G90");
        run_a_gcode("RESUME");
        set_filament_sensor(true);
        check_filament_flag = true;
        page_to(TJC_PAGE_PRINTING);
        break;
    }
}

void tjc_load_clear2_handler(int widget_id)
{
    switch (widget_id)
    {
    case YES:
        if(before_print_filament_error)      //打印前没料点击换料，结束后直接开始打印
        {
            before_print_filament_error = false;
            set_filament_sensor(true);
            start_printing(current_select_file_path);
        }
        else
        {
            // page_to(TJC_PAGE_FAN);
            page_to(TJC_PAGE_FILAMENT);
        }
        break;
    }
}


void tjc_if_stop_handler(int widget_id)
{
    switch (widget_id)
    {
    case IF_STOP_YES:
        cancel_print();
        page_to(TJC_PAGE_STOPING);
        break;

    case IF_STOP_NO:
        page_to(TJC_PAGE_PRINTING);
        break;
    }
}


void tjc_print_finish_handler(int widget_id)
{
    printer_display_status_progress = 0;
    printer_display_status_progress_back = 0;
    switch (widget_id)
    {
    // case PRINT_FINISH_SAVE_ZOFFSET:
        // if (printing_babystep != (float)0.0)
        // {
        //     run_a_gcode("Z_OFFSET_APPLY_PROBE\nSAVE_CONFIG");
        //     save_time = time(NULL);
        //     page_to(TJC_PAGE_SAVING);
        // }
        // break;

    case PRINT_FINISH_PRINT_ANGIN:
        if (filament_switch_sensor_enabled && !filament_switch_sensor_filament_detected)
        {
            MKSLOG_HIGHLIGHT("打印前检测到无耗材");
            before_print_filament_error = true;
            page_to(TJC_PAGE_FILAMENT_ERROR);
        }
        else
        {
            page_to(TJC_PAGE_REPRINT_TIP);
        }
        break;

    case PRINT_FINISH_DONE:
        page_to(TJC_PAGE_MAIN);
        break;
    }
}

void tjc_reprint_tip_handler(int widget_id)
{
    switch (widget_id)
    {
    case IF_PRINT_YES:
        if (strstr(current_select_file_path.c_str(), "plr/") != NULL)
        {
            current_select_file_path.replace(0, current_select_file_path.find("plr/") + 4, "");
        }
        start_printing(current_select_file_path);
        MKSLOG_HIGHLIGHT("current_select_file_path = %s", current_select_file_path.c_str());
        break;

    case IF_PRINT_NO:
        page_to(TJC_PAGE_PRINT_FILISH);    
        break;
    }
}

void tjc_move_handler(int widget_id)
{
    switch (widget_id)
    {
    case MOVE_1MM:
        move_step = 1;
        break;

    case MOVE_5MM:
        move_step = 5;
        break;

    case MOVE_10MM:
        move_step = 10;
        break;

    case MOVE_50MM:
        move_step = 50;
        break;

    case MOVE_X_ADD:
        move_x_decrease(move_step);
        break;

    case MOVE_X_SUB:
        move_x_increase(move_step);
        break;

    case MOVE_Y_ADD:
        move_y_decrease(move_step);
        break;

    case MOVE_Y_SUB:
        move_y_increase(move_step);
        break;

    case MOVE_Z_ADD:
        move_z_increase(move_step);
        break;

    case MOVE_Z_SUB:
        move_z_decrease(move_step);
        break;

    case MOVE_XY_HOME:
        idle_timeout_state = "Printing";
        move_home_x();
        move_home_y();
        page_to(TJC_PAGE_HOMING);
        break;

    case MOVE_Z_HOME:
        idle_timeout_state = "Printing";
        move_home_z();
        page_to(TJC_PAGE_HOMING);
        break;

    case MOVE_HOME_ALL:
        idle_timeout_state = "Printing";
        move_home();
        page_to(TJC_PAGE_HOMING);
        break;

    case MOVE_UNLOCK:
        motors_off();
        break;
    }
}

// chris todo
// PLA预热，ABS预热，一键冷却
void tjc_filament_handler(int widget_id)
{
#define PLA_TH_DEFAULT 210
#define ABS_TH_DEFAULT 260
#define PLA_TB_DEFAULT 65
#define ABS_TB_DEFAULT 80
#define FILAMENT_TEMP_DEFAULT 180

    switch (widget_id)
    {
    // case FILAMENT_SET_EXTRUDE:
    //     page_to(TJC_PAGE_KEYBOARD);
    //     send_cmd_txt(tty_fd, "show", std::to_string(current_extruder_target));
    //     break;

    // case FILAMENT_SET_HOT_BED:
    //     page_to(TJC_PAGE_KEYBOARD);
    //     send_cmd_txt(tty_fd, "show", std::to_string(current_heater_bed_target));
    //     break;

    // case FILAMENT_PLA:
    //     set_extruder_target(PLA_TH_DEFAULT);
    //     set_heater_bed_target(PLA_TB_DEFAULT);
    //     break;

    // case FILAMENT_ABS:
    //     set_extruder_target(ABS_TH_DEFAULT);
    //     set_heater_bed_target(ABS_TB_DEFAULT);
    //     break;

    // case FILAMENT_COOL:
    //     set_extruder_target(0);
    //     set_heater_bed_target(0);
    //     break;

    case FILAMENT_IN:
        filament_change_type = 1;
        filState = FIL_START;
        page_to(TJC_PAGE_FIL_FILAMENT);  //TJC_PAGE_LOADING
        // if(current_extruder_temperature < FILAMENT_TEMP_DEFAULT || current_extruder_target < FILAMENT_TEMP_DEFAULT)
        // {
        //     page_to(TJC_PAGE_HEAT_TIP);     
        // }
        // else
        // {
        //     run_a_gcode("G91");
        //     run_a_gcode("G1 E5 F300");
        //     run_a_gcode("G90");
        // }
        break;

    case FILAMENT_OUT:
        filament_change_type = 2;
        unfilState = UNFIL_START;
        page_to(TJC_PAGE_UNFIL_FILAMENT);  //TJC_PAGE_UNLOADING2
        // if(current_extruder_temperature < FILAMENT_TEMP_DEFAULT || current_extruder_target < FILAMENT_TEMP_DEFAULT)
        // {
        //     page_to(TJC_PAGE_HEAT_TIP);
        // }
        // else
        // {
        //     run_a_gcode("G91");
        //     run_a_gcode("G1 E-5 F300");
        //     run_a_gcode("G90");
        // }
        break;

    case FILAMENT_LONG_OUT:
        filament_change_type = 3;
        unfilState = UNFIL_START;
        page_to(TJC_PAGE_UNFIL_FILAMENT);  //TJC_PAGE_UNLOADING2
        break;

    case FILAMENT_LONG_CLOSE:
        page_to(TJC_PAGE_PRINTING);
        break;
    }
}

void tjc_heat_tip_handler(int widget_id)
{
    switch (widget_id)
    {
    case YES:
        page_to(TJC_PAGE_FILAMENT);
        break;
    }
}


// void tjc_fan_hangler(int widget_id)
// {
//     switch (widget_id)
//     {
//     case FAN_LED:
//         led1_on_off();
//         break;

//     case FAN_BEEP:
//         open_beep_flag = !open_beep_flag;
//         break;

//     case FAN_CHANGE_FILAMENT:
//         idle_timeout_state = "Printing";
//         set_extruder_target(250);
//         page_to(TJC_PAGE_HEATING);
//         break;
//     }
// }

void tjc_keyboard_handler(int widget_id)
{
    switch (widget_id)
    {
    case KEYBOARD_BACK:
        page_to(previous_page_id);
        break;
    }
}

void reset_calibration_value()
{
    hasEddyCb = true;
    hasBedLevel = true;
    hasVibration = true;
    hasPID = true;
}

void tjc_system_handler(int widget_id)
{
    switch (widget_id)
    {
    case SYSTEM_LANGUAGE:
        page_to(TJC_PAGE_LANGUGAE);
        break;

    case SYSTEM_RESET:
        page_to(TJC_PAGE_IF_RESET);
        break;

    case SYSTEM_ABOUT:
        page_to(TJC_PAGE_ABOUT);
        firmware_version_check();
        break;

    case SYSTEM_CALIBRATION:
        reset_calibration_value();
        page_to(TJC_PAGE_CALIBRATION);
        break;
    
    case SYSTEM_OBICO:
        if (ip_address[0] != '\0')
        {
            system("sudo systemctl restart moonraker-obico");
            run_a_gcode("OBICO_LINK_STATUS");
            page_to(TJC_PAGE_OBICO);
        }else {
            page_to(TJC_PAGE_OBICO_UNAVAIABLE);
        }
        break;
    }
}

void tjc_language_handler(int widget_id)
{
    switch (widget_id)
    {
    case LANGUAGE_BACK:
        page_to(TJC_PAGE_SYSTEM);
        break;
    }
}

void tjc_if_factory_handler(int widget_id)
{
    switch (widget_id)
    {
    case IF_RESET_YES:
    {
        // hasProbeEddy = false;
        system("cp -p /home/sovol/patch/config/*.cfg /home/sovol/printer_data/config/");
        system("cp -p /home/sovol/patch/config/*.conf /home/sovol/printer_data/config/");
        system("rm /home/sovol/printer_data/database/*");
        deleteOneFile(GUIDE_PATH);
        // deleteOneFile(MKSLOG_PATH);
        system("sync");

        page_to(TJC_PAGE_STARTING);
        send_cmd_language(tty_fd, "1");
        send_cmd_wepo(tty_fd, "1", "100");
        tjc_ui_mutex.try_lock();
        system("sync");
        system("sync");
        reset_firmware();
        system("sudo systemctl restart moonraker");
        system("sudo systemctl restart mainsail");
        system("sudo systemctl restart crowsnest");
        system("sudo systemctl restart moonraker-obico");
        system("sudo systemctl restart makerbase-client");
        break;
    }

    case IF_RESET_NO:
        page_to(TJC_PAGE_SYSTEM);
        break;
    }
}

void tjc_if_update_handler(int widget_id)
{
    switch (widget_id)
    {
    case IF_UPDATE_YES:
        page_to(TJC_PAGE_UPDATING);
        // udisk_update();  chris todo
        create_download_thread();
        break;

    case IF_UPDATE_NO:
        page_to(TJC_PAGE_MAIN);
        break;
    }
}

void tjc_about_hander(int widget_id)
{
    switch (widget_id)
    {
    case ABOUT_YES:
        page_to(TJC_PAGE_SYSTEM);
        break;
    case ABOUT_UPDATE:
        if (newVersion != curVersion)
        {
            page_to(TJC_PAGE_IF_UPDATE);
        }else{
            // send_cmd_txt(tty_fd, "t4", "Latest Version");   //judge in screen
        }
        break;
    }
}

void tjc_wifi_list_handler(int widget_id)
{
    switch (widget_id)
    {
    case WIFI_LIST_SWITCH:
        wifi_enable = !wifi_enable;
        wifi_list_init();
        break;

    case WIFI_LIST_PREV_PAGE:
        click_previous_page();
        break;

    case WIFI_LIST_NEXT_PAGE:
        click_next_page();
        break;

    case WIFI_LIST_0:
    case WIFI_LIST_1:
    case WIFI_LIST_2:
    case WIFI_LIST_3:
        if (wifi_enable) {
            click_wifi_list_ssid(widget_id - WIFI_LIST_0);
        }
        break;

    case WIFI_LIST_SCANE:
        if (wifi_enable)
        {
            wifi_list_scan();
        }
        break;

    case WIFI_LIST_PREV:
        page_to(TJC_PAGE_GUIDE_LANGUAGE);
        break;

    case WIFI_LIST_NEXT:
        if (ip_address[0] != '\0')
        {
            page_to(TJC_PAGE_GUIDE_CALIBRATION);
        }
        break;

    case WIFI_SKIP:
        page_to(TJC_PAGE_GUIDE_CALIBRATION);
        break;

    case WIFI_BACK:
        page_to(TJC_PAGE_ERROR_RESTART);
        break;
    }
}

void tjc_guide_calibration_handler(int widget_id)
{
    switch (widget_id)
    {
    case GUIDE_CALIBRATION_SKIP:
        create_guide_file(GUIDE_PATH);
        page_to(TJC_PAGE_MAIN);
        break;
    case GUIDE_CALIBRATION_BACK:
        page_to(TJC_PAGE_GUIDE_WIFI);
        break;
    case GUIDE_CALIBRATION_CONFIRM:
        page_to(TJC_PAGE_GUIDE_SELFCHECK);
        guide_state = SELF_CHECK;
        break;
    }
}

void tjc_wifi_kb_handler(int widget_id)
{
    switch (widget_id)
    {
    case WIFI_KB_CANCEL:
        page_to_wifi_list();
        break;
    }
}

void tjc_wifi_connect_succeed(int widget_id)
{
    switch (widget_id)
    {
    case WIFI_SUCCEED_YES:
        // wifi_list_init();
        page_to_wifi_list();
        update_wifilist_after_connected();
        mks_save_config();
        break;
    }
}

void tjc_wifi_connect_failed(int widget_id)
{
    switch (widget_id)
    {
    case WIFI_FAILED_YES:
        mks_reconfigure();
        wifi_list_init();
        break;
    }
}

// chris todo
void tjc_level_mode_handler(int widget_id)
{
    switch (widget_id)
    {
    case LEVEL_MODE_Z_TITL:
        idle_timeout_state = "Printing";
        run_a_gcode("Z_TILT_CALIBRATION");
        page_to(TJC_PAGE_Z_CALIBRAING);
        break;

    case LEVEL_MODE_ZOFFSET:
        idle_timeout_state = "Printing";
        set_extruder_target(150);
        run_a_gcode("PROBE_CALIBRATE");
        page_to(TJC_PAGE_HEATED_BED);
        level_zoffset_value = 0;
        break;

    case LEVEL_MODE_AUTO_LEVELING:
        idle_timeout_state = "Printing";
        run_a_gcode("heated_bed");
        page_to(TJC_PAGE_HEATED_BED_2);
        break;
    }
}

void tjc_zoffset_handler(int widget_id)
{
    switch (widget_id)
    {
    case ZOFFSET_0_01MM:
        level_zoffset_step = 0.01;
        break;

    case ZOFFSET_0_1MM:
        level_zoffset_step = 0.1;
        break;

    case ZOFFSET_1MM:
        level_zoffset_step = 1;
        break;

    case ZOFFSET_ADD:
        level_zoffset_value += level_zoffset_step;
        run_a_gcode("TESTZ z=+" + std::to_string(level_zoffset_step));
        break;

    case ZOFFSET_SUB:
        level_zoffset_value -= level_zoffset_step;
        run_a_gcode("TESTZ z=-" + std::to_string(level_zoffset_step));
        break;

    case ZOFFSET_ABORT:
        run_a_gcode("ABORT");
        page_to(TJC_PAGE_LEVEL_MODE);
        break;

    case ZOFFSET_SAVE:
        run_a_gcode("ACCEPT");
        run_a_gcode("SAVE_CONFIG");
        save_time = time(NULL);
        page_to(TJC_PAGE_SAVING);
        break;
    }
}


void tjc_error_restart_handler(int widget_id)
{
    switch (widget_id)
    {
    case ERROR_RESTART_KLIPPER:
        is_errortip_show = false;
        reset_klipper();
        page_to(TJC_PAGE_STARTING);
        break;

    case ERROR_RESTART_FIRMWARE:
        is_errortip_show = false;
        guide_state = NONE;
        state = NONE;
        reset_firmware();
        page_to(TJC_PAGE_STARTING);
        break;

    case ERROR_NETWORK_MANAGE:
        wifi_list_init();
        page_to(TJC_PAGE_ERROR_NETWORK);
        break;
    }
}

void tjc_error_reset_file_handler(int widget_id)
{
    switch (widget_id)
    {
    case ERROR_RESET_FILE:
        run_a_gcode("SDCARD_RESET_FILE");
        page_to(TJC_PAGE_MAIN);
        break;
    }
}

void tjc_error_tips_handler(int widget_id)
{
    switch (widget_id)
    {
    case ERR_TIPS_YES:
        switch (previous_page_id)
        {
        case TJC_PAGE_HOMING:
            page_to(TJC_PAGE_MOVE);
            break;

        case TJC_PAGE_STARTING:
        case TJC_PAGE_SAVING:
        case TJC_PAGE_STOPING:
            page_to(TJC_PAGE_MAIN);
            break;

        case TJC_PAGE_HEATING:
        case TJC_PAGE_Z_CALIBRAING:
        case TJC_PAGE_HEATED_BED:
        case TJC_PAGE_Z_INIT:
        case TJC_PAGE_HEATED_BED_2:
        case TJC_PAGE_AUTO_LEVELING:
            // page_to(TJC_PAGE_LEVEL_MODE); chris todo
            page_to(TJC_PAGE_MAIN);
            break;

        default:
            page_to(previous_page_id); 
            break;
        }
        break;
    }
}


void tjc_move_error_handler(int widget_id)
{
    switch (widget_id)
    {
    case MOVE_ERROR_YES:
        page_to(previous_page_id);
        break;

    case MOVE_ERROR_RESTART:
        reset_firmware();
        page_to(TJC_PAGE_STARTING);
        break;
    }
}

/* 进度条、数字键盘返回值处理 */
void tjc_event_setted_handler(int page_id, int widget_id, unsigned char first, unsigned char second)
{
    std::cout << "page_id" << page_id << std::endl;
    std::cout << "widget_id" << widget_id << std::endl;
    std::cout << "first" << first << std::endl;
    std::cout << "second" << second << std::endl;

    int number = (int)((second << 8) + first);
    if (open_beep_flag)
    {
        send_cmd_beep(tty_fd, "100");
    }


    switch (page_id)
    {
    // 主页设置温度
    case TJC_PAGE_MAIN:
        if (widget_id == MAIN_EXTRUDE)
        {
            set_extruder_target(number);
        }
        else if (widget_id == MAIN_HOT_BED)
        {
            set_heater_bed_target(number);
        }
        else if (widget_id == MAIN_CHAMBER)
        {
            MKSLOG_BLUE("run main set_chamber_target:%d", number);
            set_chamber_target(number);
        }
        page_to(TJC_PAGE_MAIN);
        break;

    // 打印界面设置温度
    case TJC_PAGE_PRINTING:
        if (widget_id == PRINTING_EXTRUDE)
        {
            set_extruder_target(number);
        }
        else if (widget_id == PRINTING_HOT_BED)
        {
            set_heater_bed_target(number);
        }
        else if (widget_id == PRINTING_CHAMBER)
        {
            MKSLOG_BLUE("run print set_chamber_target:%d", number);
            set_chamber_target(number);
        }
        page_to(TJC_PAGE_PRINTING);
        break;

    // 换料界面设置温度
    case TJC_PAGE_FILAMENT:
        if (widget_id == FILAMENT_SET_EXTRUDE)
        {
            set_extruder_target(number);
        }
        else if (widget_id == FILAMENT_SET_HOT_BED)
        {
            set_heater_bed_target(number);
        }
        page_to(TJC_PAGE_FILAMENT);
        break;

    // 模型风扇滑块
    case TJC_PAGE_OPERATE:
        // if (widget_id == OPERATE_MODEL_FAN)
        // {
            // std::string gcode_str;
            // gcode_str = "M108 P" + std::to_string(((float)number)/100.0);
            // run_a_gcode(gcode_str);
            // run_a_gcode(set_fan_speed(number));
        // }
        break;

    // LED、模型风扇滑块
    case TJC_PAGE_FAN:
        if (widget_id == FAN_MODEL_FAN)
        {
            // run_a_gcode(set_fan_speed(number));
        }
        break;

    case TJC_PAGE_FAN_LED:
    case TJC_PAGE_FAN_LED_NOCB:
        if (widget_id == FAN_0)
        {
            run_a_gcode(set_fan_speed("0", number));
        }else if (widget_id == FAN_1) {
            run_a_gcode(set_fan_speed("1", number));
        }else if (widget_id == FAN_2) {
            run_a_gcode(set_fan_speed("2", number));
        }else if (widget_id == FAN_3) {
            run_a_gcode(set_fan_speed("3", number));
        }
        break;
    }
}

/* 获取wifi键盘返回值 */
void tjc_event_wifi_keyboard(char *cmd)
{
    if (open_beep_flag)
    {
        send_cmd_beep(tty_fd, "100");
    }

    char *psk = &cmd[2];
    page_to(TJC_PAGE_WIFI_CONNECTING);
    // set_ssid_psk(psk);
    // 尝试连接 Wi-Fi
    wpa_state = DISCONNECTED;
    mks_disconnect(); // 断开连接
    std::string psk_str(psk);
    if (connectToWiFi(get_wifi_name, psk_str)) {
        std::cout << "成功连接到 Wi-Fi 网络。" << std::endl;
    } else {
        std::cout << "连接 Wi-Fi 网络失败。" << std::endl;
        wpa_state = CONNECT_FAILED;
    }
}
