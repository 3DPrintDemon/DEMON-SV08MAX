#include "../include/refresh_ui.h"
#include <time.h>

#define MB_VERSION "V1.0.0"

/*屏幕刷新锁*/
std::recursive_mutex tjc_ui_mutex;

std::string babystep;
std::string current_ip_address = "";
bool get_zoffset_flag = true;
bool auto_level_complete_flag;

bool hasEddyCb = true;
bool hasBedLevel = true;
bool hasVibration = true;
bool hasPID = true;

// bool hasProbeEddy = false;
int save_time;
extern int ota_progress;
bool is_errortip_show = false;

CHECK_STATE state = NONE;
CHECK_STATE guide_state = NONE;

typedef enum {
    EXCHANG_FIL = 0,
    EXCHANG_UNFIL
} EXCHANG_TYPE;

FIL_STATE filState = FIL_NONE;
UNFIL_STATE unfilState = UNFIL_NONE;
EXCHANG_TYPE exchangeType = EXCHANG_FIL;

// static bool is_newfil   = false;
bool is_runing   = false;
bool is_continue = true;
bool is_retry    = false;

struct timespec ms_start_time;

/* 更新页面处理 */
void *refresh_ui_pth(void *arg)
{
    clock_gettime(CLOCK_MONOTONIC, &ms_start_time); // 记录起始时间
    while (1)
    {
        // 页面刷新
        refresh_page_show();
        // 状态改变时输出调试信息
        printer_state_show();
        // 定时清理大日志
        auto_clean_big_log();
        // 定时获取状态
        auto_get_object();
        // self check
        guide_self_check();
        // check calibration
        check_calibration();
        // 延时
        usleep(100000);
    }
    pthread_exit(NULL);
}

void run_save_config() {
    run_a_gcode("SAVE_CONFIG");
}

void run_homing() {
    run_a_gcode("G28");
    set_printer_state("Printing");
    usleep(1000000); // 等待响应
}

void quad_level() {
    run_a_gcode("QUAD_GANTRY_LEVEL");  //Z_TILT_ADJUST
    set_printer_state("Printing");
    usleep(1000000); // 等待响应
}

void run_shaper() {
    run_a_gcode("SHAPER_CALIBRATE");
    set_printer_state("Printing");
    usleep(1000000); // 等待响应
}

void run_bed_mesh() {
    run_a_gcode("BED_MESH_CALIBRATE");
    set_printer_state("Printing");
    usleep(1000000); // 等待响应
}

void probeEddy()
{
    run_a_gcode("G28 X");
    run_a_gcode("G28 Y");
    run_a_gcode("Z_OFFSET_CALIBRATION METHOD=force_overlay BED_TEMP=0 EXTRUDER_TEMP=0");
    run_a_gcode("SET_GCODE_VARIABLE MACRO=_global_var VARIABLE=has_z_offset_calibrated VALUE=True");
    set_printer_state("Printing");
    usleep(1000000); // 等待响应
}

std::string getTranslation(int lang, const std::string& key) {
    // 定义一个三维映射：语言 -> 键 -> 翻译
    std::unordered_map<int, std::unordered_map<std::string, std::string>> translations = {
        {0, {{"self_check", "XYZ轴检测,涡流检测,加热检测"}, {"bed_mesh", "网床校准"}, {"vibration", "振动"}}},
        {1, {{"self_check", "XYZ Axis Detection, Eddy Current Detection, Heating Detection"}, {"bed_mesh", "Mesh Bed Calibration"}, {"vibration", "Vibration"}}},
        {2, {{"self_check", "XYZ軸検出,渦流検出,加熱検出"}, {"bed_mesh", "メッシュベッドキャリブレーション"}, {"vibration", "振動"}}}, // 日文翻译示例，实际可能需要更准确
        {3, {{"self_check", "XYZ-Achsenprüfung, Wirbelstromprüfung, Heizprüfung"}, {"bed_mesh", "Netzbettkalibrierung"}, {"vibration", "Vibration"}}},
        {4, {{"self_check", "Detección de ejes XYZ, Detección de corrientes parásitas, Detección de calentamiento"}, {"bed_mesh", "Calibración de cama de malla"}, {"vibration", "Vibración"}}},
        {5, {{"self_check", "Détection des axes XYZ, Détection par courants de Foucault, Détection de chauffage"}, {"bed_mesh", "Étalonnage du lit en filet"}, {"vibration", "Vibration"}}},
        {6, {{"self_check", "Detecção de eixos XYZ, Detecção de correntes parasitas, Detecção de aquecimento"}, {"bed_mesh", "Calibração de cama de malha"}, {"vibration", "Vibração"}}},
        {7, {{"self_check", "Rilevamento assi XYZ, Rilevamento a correnti vorticose, Rilevamento riscaldamento"}, {"bed_mesh", "Calibrazione letto reticolare"}, {"vibration", "Vibrazione"}}}
    };
 
    // 查找对应的翻译
    auto langIt = translations.find(lang);
    if (langIt != translations.end()) {
        auto keyIt = langIt->second.find(key);
        if (keyIt != langIt->second.end()) {
            return keyIt->second;
        }
    }
    // 如果没有找到对应的翻译，返回一个默认消息或空字符串
    return "";
}

void set_selfCheck_label(std::string num)
{
    // send_cmd_pco(tty_fd, "t1", num >= "1" ? "1983" : "65535");
    // send_cmd_pco(tty_fd, "t2", num >= "2" ? "1983" : "65535");
    // send_cmd_pco(tty_fd, "t3", num >= "3" ? "1983" : "65535");
    if (num == "1")
    {
        send_cmd_pco(tty_fd, "t1", "1983");
        send_cmd_txt(tty_fd, "state", getTranslation(language, "self_check"));
    }else if (num == "2") {
        send_cmd_pco(tty_fd, "t2", "1983");
        send_cmd_txt(tty_fd, "state", getTranslation(language, "bed_mesh"));
    }else if (num == "3"){
        send_cmd_pco(tty_fd, "t3", "1983");
        send_cmd_txt(tty_fd, "state", getTranslation(language, "vibration"));
    }
}

void guide_self_check()
{
    if (guide_state == NONE)
    {
        return;
    }
    send_cmd_txt(tty_fd, "ext", std::to_string(current_extruder_temperature) + "℃" + "/" + std::to_string(current_extruder_target) + "℃");
    send_cmd_txt(tty_fd, "bed", std::to_string(current_heater_bed_temperature) + "℃" + "/" + std::to_string(current_heater_bed_target) + "℃");
    switch(guide_state) {
        case SELF_CHECK:    //XYZ轴检测,涡流检测,加热检测
            if (!is_runing) {
                probeEddy();
                run_a_gcode("G1 Z10 F1500");
                run_a_gcode("M104 S120");
                run_a_gcode("M190 S45");
                is_runing = true;
                set_selfCheck_label("1");
            }
            if (idle_timeout_state == "Ready") {
                send_cmd_pco(tty_fd, "t1", "1000");
                MKSLOG_YELLOW("++++++++++Self check finished!!!++++++++++");
                guide_state = BED_LEVEL;
                is_runing = false;
            }
            break;

        case BED_LEVEL:
            if (!is_runing) {
                MKSLOG_YELLOW("++++++++++bed_level start!!!++++++++++");
                run_bed_mesh();
                is_runing = true;
                set_selfCheck_label("2");
            }
            if (idle_timeout_state == "Ready") {
                send_cmd_pco(tty_fd, "t2", "1000");
                MKSLOG_YELLOW("++++++++++bed_level finished!!!++++++++++");
                guide_state = VIBRATION;
                is_runing = false;
            }
            break;

        case VIBRATION:
            if (!is_runing) {
                MKSLOG_YELLOW("++++++++++vibration start!!!++++++++++");
                run_homing();
                run_shaper();
                is_runing = true;
                set_selfCheck_label("3");
            }
            if (idle_timeout_state == "Ready") {
                send_cmd_pco(tty_fd, "t3", "1000");
                MKSLOG_YELLOW("++++++++++run_shaper_xy finished!!!++++++++++");
                guide_state = COMPLETE;
                is_runing = false;
            }
            break;

        case COMPLETE:
            MKSLOG_YELLOW("++++++++++state = COMPLETE++++++++++");
            create_guide_file(GUIDE_PATH);
            page_to(TJC_PAGE_SAVING);
            guide_state = NONE;
            run_save_config();
            break;

        case NONE:
            break;

        default:
            break;
    }
}

void check_calibration()
{
    // MKSLOG_YELLOW("++++++++++state:%d, idle_state:%s, webhooks_state:%s++++++", state+1, idle_timeout_state.c_str(), webhooks_state.c_str());
    switch(state) {
        case EDDY_CALIBRATION:
            if (hasEddyCb)
            {
                if (!is_runing) {
                    page_to(TJC_PAGE_Z_CALIBRAING);
                    // if (!hasProbeEddy)
                    // {
                    MKSLOG_YELLOW("++++++++++eddy level++++++++++");
                    probeEddy();
                    // }else {
                    //     MKSLOG_YELLOW("++++++++++quad level home++++++++++");
                    //     run_homing();
                    // }
                    is_runing = true;
                }
                if (idle_timeout_state == "Ready") {
                    MKSLOG_YELLOW("++++++++++quad level finished!!!++++++++++");
                    state = BED_LEVEL;
                    is_runing = false;
                }
            }else {
                state = BED_LEVEL;
            }
            break;
        case BED_LEVEL:
            if (hasBedLevel)
            {
                if (!is_runing) {
                    MKSLOG_YELLOW("++++++++++bed_level start!!!++++++++++");
                    page_to(TJC_PAGE_AUTO_LEVELING);
                    run_bed_mesh();
                    is_runing = true;
                }
                if (idle_timeout_state == "Ready") {
                    MKSLOG_YELLOW("++++++++++bed_level finished!!!++++++++++");
                    state = VIBRATION;
                    is_runing = false;
                }
            }else {
                state = VIBRATION;
            }
            break;
        case VIBRATION:
            if (hasVibration)
            {
                if (!is_runing) {
                    MKSLOG_YELLOW("++++++++++vibration start!!!++++++++++");
                    page_to(TJC_PAGE_VIBRATION);
                    run_homing();
                    run_shaper();
                    is_runing = true;
                }
                if (idle_timeout_state == "Ready") {
                    MKSLOG_YELLOW("++++++++++run_shaper_xy finished!!!++++++++++");
                    state = PID;
                    is_runing = false;
                }
            }else {
                state = PID;
            }
            break;
        case PID:
            if (hasPID)
            {
                if (!is_runing) {
                    page_to(TJC_PAGE_PID);
                    run_a_gcode("PID_CALIBRATE HEATER=extruder TARGET=200");
                    run_a_gcode("PID_CALIBRATE HEATER=heater_bed TARGET=60");
                    set_printer_state("Printing");
                    is_runing = true;
                    usleep(1000000); // 等待响应
                }
                if (printer_pid_finished && idle_timeout_state == "Ready") {
                    MKSLOG_YELLOW("++++++++++printer_pid_finished: %d++++++++++", printer_pid_finished);
                    state = COMPLETE;
                    is_runing = false;
                }
            }else {
                state = COMPLETE;
            }
            break;
        case COMPLETE:
            MKSLOG_YELLOW("++++++++++state = COMPLETE++++++++++");
            create_guide_file(GUIDE_PATH);
            page_to(TJC_PAGE_SAVING);
            state = NONE;
            run_save_config();
            break;

        case NONE:
            break;

        default:
            break;
    }
}

void tjc_printing_pauseing_refresh()
{
    if (current_print_state == "paused") {
        MKSLOG_GREEN("current_print_state paused!!!");
        run_a_gcode("LOAD_FILAMENT");
        page_to(TJC_PAGE_LOADING);
    }
}

/* 界面刷新 */
void refresh_page_show()
{
    if (tjc_ui_mutex.try_lock())
    {
        tjc_error_refresh(); // 报错信息
        print_finish_check();

        switch (current_page_id)
        {
        case TJC_PAGE_STARTING:
            tjc_starting_refresh();
            break;

        case TJC_PAGE_GUIDE_LANGUAGE:
            tjc_guide_language_refresh();
            break;

        case TJC_PAGE_GUIDE_WIFI:
        case TJC_PAGE_ERROR_NETWORK:
            // tjc_guide_wifi_refresh();
            tjc_wifi_list_refresh();
            break;

        case TJC_PAGE_IP_POP:
            tjc_ip_pop_refresh();
            break;

        case TJC_PAGE_MAIN:
            tjc_main_refresh();
            break;

        case TJC_PAGE_FILE_LIST:
            tjc_file_list_refresh();
            break;

        case TJC_PAGE_NO_UDISK:
            tjc_no_udisk_refresh();
            break;

        case TJC_PAGE_PREVIEW:
            tjc_preview_refresh();
            break;

        case TJC_PAGE_PRINTING:
            tjc_printing_refresh();
            break;

        case TJC_PAGE_OPERATE:
            tjc_operate_refresh();
            break;

        case TJC_PAGE_LOADING:
            tjc_loading_refresh();
            break;

        case TJC_PAGE_UNLOADING:
            tjc_unloading_refresh();
            break;

        case TJC_PAGE_MOVE:
            tjc_move_refresh();
            break;

        case TJC_PAGE_HOMING:
            tjc_homing_refresh();
            break;

        case TJC_PAGE_STOPING:
            tjc_stoping_refresh();
            break;

        case TJC_PAGE_FILAMENT:
            // tjc_filament_refresh();
            break;

        case TJC_PAGE_HEATING:
            tjc_heating_refresh();
            break;

        case TJC_PAGE_LOADING2:
            tjc_loading2_refresh();
            break;

        case TJC_PAGE_UNLOADING2:
            tjc_unloading2_refresh();
            break;

        case TJC_PAGE_FAN:
            tjc_fan_refresh();
            break;

        case TJC_PAGE_ABOUT:
            tjc_about_refresh();
            break;

        case TJC_PAGE_WIFI_LIST:
            tjc_wifi_list_refresh();
            break;

        case TJC_PAGE_WIFI_CONNECTING:
            tjc_wifi_connecting_refresh();
            break;

        case TJC_PAGE_LEVEL_MODE:
            tjc_level_mode_refresh();
            break;

        case TJC_PAGE_Z_CALIBRAING:
            // tjc_z_calibrating_refresh();
            break;

        case TJC_PAGE_HEATED_BED:
            tjc_head_bed_refresh();
            break;

        case TJC_PAGE_Z_INIT:
            tjc_z_init_refresh();
            break;

        case TJC_PAGE_ZOFFSET:
            tjc_zoffset_refresh();
            break;

        case TJC_PAGE_HEATED_BED_2:
            tjc_head_bed_2_refresh();
            break;

        case TJC_PAGE_SAVING:
            tjc_saving_refresh();
            break;

        case TJC_PAGE_AUTO_LEVELING:
            // tjc_auto_leveling_refresh();
            break;

        case TJC_PAGE_MKS_HARDWARE_TEST:
            tjc_mks_test_refresh();
            break;
        
        case TJC_PAGE_UPDATING:
            tjc_ota_update();
            break;
        
        case TJC_PAGE_FAN_LED:
        case TJC_PAGE_FAN_LED_NOCB:
            tjc_fan_led_refresh();
            break;

        case TJC_PAGE_CALIBRATION:
            break;
        
        case TJC_PAGE_OBICO:
            tjc_obico_refresh();
            break;

        case TJC_PAGE_PAUSEING:
            tjc_printing_pauseing_refresh();
            break;            

        case TJC_PAGE_FIL_FILAMENT:
            tjc_change_filament_refresh();
            break;

        case TJC_PAGE_UNFIL_FILAMENT:
            tjc_change_unfilament_refresh();
            break;

        }
        tjc_ui_mutex.unlock();
    }
}

void tjc_guide_language_refresh()
{
}

void tjc_guide_wifi_refresh()
{
}

void tjc_ip_pop_refresh()
{
    static int start_time = time(NULL);
    if (time_differ(1, start_time))
    {
        start_time = time(NULL);
        if (get_interface_info("eth0", ip_address, mac_address) == false) // 获取以太网接口信息
        {
            get_interface_info("wlan0", ip_address, mac_address); // 获取wifi-wlan0接口信息
        }
        send_cmd_txt(tty_fd, "t0", "IP: " + std::string(ip_address)); // 显示IP地址
        send_cmd_txt(tty_fd, "t1", "SSID: " + current_connected_ssid_name); // 显示SSID
    }
}

void tjc_ota_update()
{
    send_cmd_val(tty_fd, "j0", std::to_string(ota_progress));
    send_cmd_txt(tty_fd, "t1", std::to_string(ota_progress) + "%");
    if (ota_progress == 100) {
        send_cmd_txt(tty_fd, "t1", "installing...");
    }else if (ota_progress == -1) {
        send_cmd_txt(tty_fd, "t1", "MD5 Error");
    }else if (ota_progress == -2) {
        send_cmd_txt(tty_fd, "t1", "deb install failed");
    }
}

void tjc_obico_refresh()
{
    send_cmd_txt(tty_fd, "t0", one_time_passcode);
    send_cmd_txt(tty_fd, "qr0", one_time_passlink);
}

void tjc_fan_led_refresh()
{
    // led状态
    if (sled1)
    {
        send_cmd_picc_picc2(tty_fd, "b1", "95", "98");
        // send_cmd_val(tty_fd, "sw0", std::to_string(1));
    }
    else
    {
        send_cmd_picc_picc2(tty_fd, "b1", "98", "95");
        // send_cmd_val(tty_fd, "sw0", std::to_string(0));
    }
    if (prev_current_fan_speed != current_fan_speed) {
        send_cmd_val(tty_fd, "h0", std::to_string((int)(current_fan_speed * 100 + 0.5)));
        send_cmd_val(tty_fd, "n0", std::to_string((int)(current_fan_speed * 100 + 0.5)));
        prev_current_fan_speed = current_fan_speed;
    }
    if (prev_generic_fan1_speed != generic_fan1_speed) {
        send_cmd_val(tty_fd, "h1", std::to_string((int)(generic_fan1_speed * 100 + 0.5)));
        send_cmd_val(tty_fd, "n1", std::to_string((int)(generic_fan1_speed * 100 + 0.5)));
        prev_generic_fan1_speed = generic_fan1_speed;
    }
    if (prev_generic_fan2_speed != generic_fan2_speed) {
        send_cmd_val(tty_fd, "h2", std::to_string((int)(generic_fan2_speed * 100 + 0.5)));
        send_cmd_val(tty_fd, "n2", std::to_string((int)(generic_fan2_speed * 100 + 0.5)));
        prev_generic_fan2_speed = generic_fan2_speed;
    }
    if (prev_generic_fan3_speed != generic_fan3_speed) {
        send_cmd_val(tty_fd, "h3", std::to_string((int)(generic_fan3_speed * 100 + 0.5)));
        send_cmd_val(tty_fd, "n3", std::to_string((int)(generic_fan3_speed * 100 + 0.5)));
        prev_generic_fan3_speed = generic_fan3_speed;
    }
}

void tjc_main_refresh()
{
    if (time_differ_ms2(500.0, ms_start_time))
    {
        // mkslog_print("tjc_main_refresh");
        // 温度曲线
        send_cmd_txt(tty_fd, "extruder_temp", std::to_string(current_extruder_temperature) + "/");
        send_cmd_txt(tty_fd, "hot_bed_temp", std::to_string(current_heater_bed_temperature) + "/");
        send_cmd_txt(tty_fd, "chamber_temp", std::to_string(current_chamber_temperature) + "/");
        send_cmd_txt(tty_fd, "extru_target", std::to_string(current_extruder_target));
        send_cmd_txt(tty_fd, "bed_target", std::to_string(current_heater_bed_target));
        send_cmd_txt(tty_fd, "chamber_target", std::to_string(current_chamber_target));

        send_cmd_add(tty_fd, "s0.id", "0", std::to_string((int)round(((double)current_extruder_temperature + 15.0) * (255.0 / 315.0))));
        // send_cmd_add(tty_fd, "s0.id", "0", "0");
        send_cmd_add(tty_fd, "s0.id", "1", std::to_string((int)round(((double)current_heater_bed_temperature + 15.0) * (255.0 / 315.0))));
        // send_cmd_add(tty_fd, "s0.id", "1", "0");
    }
    // 刷新wifi信号强度
    refresh_wifi_level();
}

void tjc_file_list_refresh()
{
}

void tjc_no_udisk_refresh()
{
}

void tjc_preview_refresh()
{
    if (open_timelapse_flag)
    {
        send_cmd_picc(tty_fd, "q0", "79");
    }
    else
    {
        send_cmd_picc(tty_fd, "q0", "31");
    }

    send_cmd_txt(tty_fd, "t1", show_time(metadata_estimated_time)); // 显示预估时间

    std::string len = std::to_string((float)metadata_filament_total / 1000.0);
    len = len.substr(0, len.find(".") + 2) + "m";
    send_cmd_txt(tty_fd, "t0", len); // 显示预估耗材长度
}

void tjc_printing_refresh()
{
    if (time_differ_ms2(500.0, ms_start_time))
    {
        // mkslog_print("tjc_printing_refresh");
        // 暂停恢复按钮
        if (current_print_state == "printing")
        {
            send_cmd_picc_picc2(tty_fd, "b3", "33", "35");
        }
        else if (current_print_state == "paused")
        {
            send_cmd_picc_picc2(tty_fd, "b3", "34", "35");
        }
        // 温度
        send_cmd_txt(tty_fd, "extruder_temp", std::to_string(current_extruder_temperature) + "/");
        send_cmd_txt(tty_fd, "hot_bed_temp", std::to_string(current_heater_bed_temperature) + "/");
        send_cmd_txt(tty_fd, "extru_target", std::to_string(current_extruder_target));
        send_cmd_txt(tty_fd, "bed_target", std::to_string(current_heater_bed_target));
        send_cmd_txt(tty_fd, "chamber_temp", std::to_string(current_chamber_temperature) + "/");
        send_cmd_txt(tty_fd, "chamber_target", std::to_string(current_chamber_target));

        // 显示文件名
        send_cmd_txt(tty_fd, "t0", printer_print_stats_filename);
        printer_print_stats_filename_back = printer_print_stats_filename;
        // 显示剩余时间
        send_cmd_txt(tty_fd, "t2", show_time(get_cal_printing_time((int)(printing_times), metadata_estimated_time, printer_display_status_progress)));
        // 显示时间
        // 显示速度
        std::string req_speed = std::to_string(current_move_speed / 60 * current_move_speed_factor);
        send_cmd_txt(tty_fd, "t4", req_speed.substr(0, req_speed.find(".")) + "mm/s");
        // // 显示高度
        // 暂停时不刷新进度条
        if (printer_display_status_progress >= printer_display_status_progress_back)
        {
            printer_display_status_progress_back = printer_display_status_progress;
            // 显示进度条
            send_cmd_val(tty_fd, "j0", std::to_string(printer_display_status_progress));
            // 显示百分比
            send_cmd_txt(tty_fd, "t6", std::to_string(printer_display_status_progress) + "%");
        }
        //dutou, canrao check
        if (plug_status)
        {
            run_a_gcode("SET_GCODE_VARIABLE MACRO=variables VARIABLE=plug_status VALUE=False");
            page_to(TJC_PAGE_PLUG_TIP);
        }else if(winding_status) {
            run_a_gcode("SET_GCODE_VARIABLE MACRO=variables VARIABLE=winding_status VALUE=False");
            page_to(TJC_PAGE_WINDING_TIP);
        }
    }
    // 刷新wifi信号强度
    refresh_wifi_level();
}

void tjc_operate_refresh()
{
    // 显示zoffset步进值
    send_cmd_val(tty_fd, "zoffset_step", std::to_string((uint32_t)((operate_zoffset_step + 0.001) * 100.0)));
    // 显示百分比步进值
    send_cmd_val(tty_fd, "percent_step", std::to_string((uint32_t)operate_percent_step));
    // 显示zoffset值
    std::string z_offset = std::to_string(printer_gcode_move_homing_origin[2]);
    z_offset = z_offset.substr(0, z_offset.find(".") + 3) + "mm";
    send_cmd_txt(tty_fd, "t1", z_offset);
    // 显示流量速度
    send_cmd_txt(tty_fd, "t2", std::to_string((int)(current_move_extrude_factor * 100 + 0.5)) + "%");
    // 显示打印速度
    send_cmd_txt(tty_fd, "t3", std::to_string((int)(current_move_speed_factor * 100 + 0.5)) + "%");

    static int start_time = time(NULL);
    if (time_differ(1, start_time))
    {
        start_time = time(NULL);
        send_cmd_val(tty_fd, "n0", std::to_string((int)(current_fan_speed * 100 + 0.5)));
        send_cmd_val(tty_fd, "h0", std::to_string((int)(current_fan_speed * 100 + 0.5)));
    }
}

void tjc_unloading_refresh()
{
    sleep(1);
    if (filament_switch_sensor_enabled)
    {
        set_filament_sensor(false);
    }
    else if (idle_timeout_state != "Printing" && filament_switch_sensor_enabled == false)
    {
        set_filament_sensor(true);
        page_to(TJC_PAGE_UNLOAD_FINISH); // 打印中换料、一键换料
    }
}

void tjc_loading_refresh()
{
    sleep(1);
    if (filament_switch_sensor_enabled)
    {
        MKSLOG_GREEN("set_filament_sensor false");
        set_filament_sensor(false);
    }
    else if (idle_timeout_state != "Printing" && filament_switch_sensor_enabled == false)
    {
        MKSLOG_GREEN("set_filament_sensor true and finish");
        set_filament_sensor(true);
        page_to(TJC_PAGE_LOAD_FINISH); // 打印中换料、一键换料
    }
}

void tjc_stoping_refresh()
{
    if (idle_timeout_state == "Ready" && current_print_state == "standby")
    {
        MKSLOG_HIGHLIGHT("处理已结束");
        page_to(TJC_PAGE_PRINT_FILISH);
        send_cmd_txt(tty_fd, "t0", printer_print_stats_filename_back);
        send_cmd_txt(tty_fd, "t1", show_time((int)(printing_times)));
        begin_show_finish_jpg = true;
        if (printing_babystep == (float)0.0)
        {
            send_cmd_aph(tty_fd, "b1", "60");
        }
        else
        {
            send_cmd_aph(tty_fd, "b1", "127");
        }
    }
}

void tjc_move_refresh()
{
    // 步进值
    send_cmd_val(tty_fd, "move_step", std::to_string((int)move_step));
    // 是否已解锁
    if (printer_toolhead_homed_axes == "")
    {
        send_cmd_txt(tty_fd, "t2", "ON");
        send_cmd_picc_picc2(tty_fd, "b14", "42", "43");
    }
    else
    {
        send_cmd_txt(tty_fd, "t2", "OFF");
        send_cmd_picc_picc2(tty_fd, "b14", "40", "41");
    }
}

void tjc_homing_refresh()
{
    if (idle_timeout_state != "Printing")
    {
        page_to(TJC_PAGE_MOVE);
    }
}

void tjc_filament_refresh()
{
    // 显示目标温度
    send_cmd_txt(tty_fd, "t2", std::to_string(current_extruder_temperature) + "℃" + " / " + std::to_string(current_extruder_target) + "℃");
    send_cmd_txt(tty_fd, "t3", std::to_string(current_heater_bed_temperature) + "℃" + " / " + std::to_string(current_heater_bed_target) + "℃");
}

void change_filament_title_state(std::string num)
{
    send_cmd_pco(tty_fd, "t0", num == "0" ? "1983" : "65535");
    send_cmd_pco(tty_fd, "t1", num == "1" ? "1983" : "65535");
    send_cmd_pco(tty_fd, "t2", num == "2" ? "1983" : "65535");
    send_cmd_pco(tty_fd, "t3", num == "3" ? "1983" : "65535");
    if (num == "0")
    {
        send_cmd_picc(tty_fd, "q0", "103");
    }else if (num == "1") {
        send_cmd_picc(tty_fd, "q0", "104");
    }else if (num == "2") {
        send_cmd_picc(tty_fd, "q0", "105");
    }
}

void tjc_change_filament_refresh() 
{
    if (filState == FIL_NONE) {
        return;
    }
    if (current_print_state == "paused")
    {
        send_cmd_txt(tty_fd, "t4", std::to_string(current_extruder_temperature) + "℃");
    }else{
        send_cmd_txt(tty_fd, "t4", std::to_string(current_extruder_temperature) + "℃" + "->" + std::to_string(current_extruder_target) + "℃");
    }
    
    switch (filState)
    {
        case FIL_START:
        {
            MKSLOG_YELLOW("+++ FIL_START +++");
            filState = FIL_EXTRUDER_HOT;
            if (current_print_state != "paused") {
                std::string gcode = set_ext_temp(UNLOAD_FILAMENT_TEMP, 0);
                run_a_gcode(gcode);
            }
        }
            break;
        case FIL_EXTRUDER_HOT:
            MKSLOG_YELLOW("+++filState:%d, extruder_temperature:%d", filState, current_extruder_temperature);
            change_filament_title_state("0");
            if (current_print_state == "paused") {
                filState = FIL_POSITION;
            }
            else if (current_extruder_temperature == UNLOAD_FILAMENT_TEMP) {
                filState = FIL_POSITION;
            }
            break;
        case FIL_POSITION:
            if (!is_runing) {
                MKSLOG_YELLOW("+++filState:%d, sensor_filament_detected:%d", filState, filament_switch_sensor_filament_detected);
                change_filament_title_state("1");
                usleep(500000);
                page_to(TJC_PAGE_SURE_FILL_IN); // 如果没有耗材，提示插入耗材 
                is_runing = true;
            }

            if (filament_switch_sensor_filament_detected) {
                if(current_page_id == TJC_PAGE_FIL_FILAMENT) {
                    filState = FIL_NEW_FILAMENT;
                    is_runing = false;
                }
            }
            break;
        case FIL_NEW_FILAMENT:
            if (!is_runing) {
                change_filament_title_state("2");
                if (is_retry) {
                    is_retry = false;
                    run_a_gcode("_CONTINUE_LOAD_FILAMENT");
                    MKSLOG_YELLOW("+++ CONTINUE LOAD_FILAMENT +++");
                }else {
                    run_a_gcode("LOAD_FILAMENT");
                    MKSLOG_YELLOW("+++ LOAD_FILAMENT +++");
                }

                is_runing = true;
                set_printer_state("Printing");
            }
            if (idle_timeout_state == "Ready") {
                MKSLOG_YELLOW("+++filState:%d, end filament", filState);
                is_runing = false;
                page_to(TJC_PAGE_IF_FIL_SUCCESS);  // if continue filament
            }
            break;
        case FIL_COMPLETE:
            MKSLOG_YELLOW("+++filState:%d, complete", filState);
            filState = FIL_NONE;
            change_filament_title_state("3");
            usleep(500000);
            if (current_print_state != "paused")
            {
                run_a_gcode(set_ext_temp(0, 0));
                page_to(TJC_PAGE_FILAMENT);
            }else{
                page_to(TJC_PAGE_PRINTING);
            }
            break;
        default:
            break;
    }
}

void change_unfilament_title_state(std::string num)
{
    send_cmd_pco(tty_fd, "t0", num == "0" ? "1983" : "65535");
    send_cmd_pco(tty_fd, "t1", num == "1" ? "1983" : "65535");
    send_cmd_pco(tty_fd, "t2", num == "2" ? "1983" : "65535");
    if (num == "0")
    {
        send_cmd_picc(tty_fd, "q0", "106");
    }else if (num == "1") {
        send_cmd_picc(tty_fd, "q0", "107");
    }else if (num == "2") {
        send_cmd_picc(tty_fd, "q0", "108");
    }
}

void tjc_change_unfilament_refresh()
{
    if (unfilState == UNFIL_NONE) {
        return;
    }
    send_cmd_txt(tty_fd, "t3", std::to_string(current_extruder_temperature) + "℃" + "->" + std::to_string(current_extruder_target) + "℃");
    
    switch (unfilState)
    {
        case UNFIL_START:
        {
            MKSLOG_YELLOW("+++ UNFIL_START current_print_state:%s+++", current_print_state.c_str());
            unfilState = UNFIL_EXTRUDER_HOT;
            if (current_print_state != "paused") {
                std::string gcode = set_ext_temp(UNLOAD_FILAMENT_TEMP, 0);
                run_a_gcode(gcode);
            }
        }
            break;
        case UNFIL_EXTRUDER_HOT:
            MKSLOG_YELLOW("+++unfilState:%d, extruder_temperature:%d", unfilState, current_extruder_temperature);
            change_unfilament_title_state("0");
            if (current_print_state == "paused") {
                unfilState = UNFIL_RUNNING;
            }
            else if (current_extruder_temperature == UNLOAD_FILAMENT_TEMP) {
                unfilState = UNFIL_RUNNING;
            }
            break;

        case UNFIL_RUNNING:
            if (!is_runing) {
                if (filament_change_type == 2) {
                    MKSLOG_YELLOW("+++ UNLOAD_FILAMENT +++");
                    run_a_gcode("UNLOAD_FILAMENT");
                }else if (filament_change_type == 3) {
                    MKSLOG_YELLOW("+++ UNLOAD BUFFER_LONG_UNLOAD_FILAMENT +++");
                    run_a_gcode("BUFFER_LONG_UNLOAD_FILAMENT");
                }
                change_unfilament_title_state("1");
                is_runing = true;
                set_printer_state("Printing");
            }
            if (idle_timeout_state == "Ready") {
                MKSLOG_YELLOW("+++unfilState:%d, end filament", unfilState);
                is_runing = false;
                unfilState = UNFIL_COMPLETE;
            }
            break;
        
        case UNFIL_COMPLETE:
            MKSLOG_YELLOW("+++unfilState:%d, complete", unfilState);
            unfilState = UNFIL_NONE;
            change_unfilament_title_state("2");
            usleep(500000);
            if (current_print_state != "paused")
            {
                run_a_gcode(set_ext_temp(0, 0));
                page_to(TJC_PAGE_FILAMENT);
            }else{
                page_to(TJC_PAGE_PRINT_FILAMENT);
            }
            break;
    }
}

void tjc_heating_refresh()
{
    // 等待加热完成
    send_cmd_txt(tty_fd, "extrude_temp", std::to_string(current_extruder_temperature) + "/");
    send_cmd_txt(tty_fd, "extrude_target", std::to_string(current_extruder_target));
    
    if(before_print_filament_error)         //打印前检测到没料
    {
        if (current_extruder_temperature == 180)
        {
            MKSLOG_YELLOW("tjc_heating_refresh 1");
            run_a_gcode("UNLOAD_FILAMENT");
            if (filament_change_type >= 1 && filament_change_type <= 3) {
                page_to(TJC_PAGE_UNLOADING2);
            }else {
                page_to(TJC_PAGE_UNLOADING);
            }
            filament_change_type = 0;
        }
    }
    else if(current_extruder_target == UNLOAD_FILAMENT_TEMP)    //一键换料
    {
        if (current_extruder_temperature == UNLOAD_FILAMENT_TEMP)
        {
            MKSLOG_YELLOW("tjc_heating_refresh 2");
            idle_timeout_state = "Printing";
            if (filament_change_type == 1)
            {
                run_a_gcode("LOAD_FILAMENT");
                page_to(TJC_PAGE_LOADING);
            }else if (filament_change_type == 2)
            {
                run_a_gcode("UNLOAD_FILAMENT");
                page_to(TJC_PAGE_UNLOADING2);
            }else if (filament_change_type == 3)
            {
                run_a_gcode("BUFFER_LONG_UNLOAD_FILAMENT");
                page_to(TJC_PAGE_UNLOADING2);
            }else {
                run_a_gcode("UNLOAD_FILAMENT");
                page_to(TJC_PAGE_UNLOADING);
            }
            filament_change_type = 0;
        }
    }
}

void tjc_unloading2_refresh()
{
    sleep(1);
    if (filament_switch_sensor_enabled)
    {
        set_filament_sensor(false);
    }
    else if (idle_timeout_state != "Printing" && filament_switch_sensor_enabled == false)
    {
        set_filament_sensor(true);
        page_to(TJC_PAGE_FILAMENT);
    }
}

void tjc_loading2_refresh()
{
    sleep(1);
    if (filament_switch_sensor_enabled)
    {
        set_filament_sensor(false);
    }
    else if (idle_timeout_state != "Printing" && filament_switch_sensor_enabled == false)
    {
        set_filament_sensor(true);
        page_to(TJC_PAGE_FILAMENT);
    }
}

void tjc_fan_refresh()
{
    // led状态
    if (sled1)
    {
        send_cmd_picc_picc2(tty_fd, "b3", "47", "47");
    }
    else
    {
        send_cmd_picc_picc2(tty_fd, "b3", "46", "46");
    }
    // 蜂鸣器
    if (open_beep_flag)
    {
        send_cmd_picc_picc2(tty_fd, "b4", "47", "47");
    }
    else
    {
        send_cmd_picc_picc2(tty_fd, "b4", "46", "46");
    }
    // 显示风扇速度
    static int start_time = time(NULL);
    if (time_differ(1, start_time))
    {
        start_time = time(NULL);
        send_cmd_val(tty_fd, "n1", std::to_string((int)(current_fan_speed * 100 + 0.5)));
        send_cmd_val(tty_fd, "h1", std::to_string((int)(current_fan_speed * 100 + 0.5)));
    }
}

// chris todo
void tjc_about_refresh()
{
    static int start_time = time(NULL);
    if (time_differ(1, start_time))
    {
        send_cmd_txt(tty_fd, "t2", curVersion);  //local
        send_cmd_txt(tty_fd, "t6", newVersion);  //remote
    }
}

void tjc_wifi_list_refresh()
{
    static int start_time = time(NULL);
    if (time_differ(1, start_time))
    {
        start_time = time(NULL);
        if (get_interface_info("eth0", ip_address, mac_address) == false) // 获取以太网接口信息
        {
            get_interface_info("wlan0", ip_address, mac_address); // 获取wifi-wlan0接口信息
        }
        if (current_ip_address != std::string(ip_address) && wifi_enable)
        {
            update_wifilist_after_connected();
            current_ip_address = std::string(ip_address);
        }
        send_cmd_txt(tty_fd, "t1", "IP: " + current_ip_address);   // 显示IP地址
        send_cmd_txt(tty_fd, "t2", "MAC: " + std::string(mac_address)); // 显示MAC地址
    }
}

void tjc_wifi_connecting_refresh()
{
    if (wpa_state == CONNECTED)
    {
        page_to(TJC_PAGE_WIFI_SUCCEED);
    }
    else if (wpa_state == CONNECT_FAILED)
    {
        page_to(TJC_PAGE_WIFI_FAILED);
    }
    else
    {
        static int start_time = time(NULL), start_time2 = time(NULL);
        if (time_differ(3, start_time))
        {
            start_time = time(NULL);
            mks_reconnect();
        }
        if (time_differ(2, start_time2))
        {
            start_time2 = time(NULL);
            mks_status(); // 获取连接状态
        }
    }
}

void tjc_level_mode_refresh()
{
}

// void tjc_z_calibrating_refresh()
// {
//     if (idle_timeout_state != "Printing")
//     {
//         page_to(TJC_PAGE_LEVEL_MODE);
//     }
// }

void tjc_head_bed_refresh()
{
    sleep(1);
    send_cmd_txt(tty_fd, "extrude_temp", std::to_string(current_extruder_temperature) + "/");
    send_cmd_txt(tty_fd, "extrude_target", std::to_string(current_extruder_target));
    send_cmd_txt(tty_fd, "hot_bed_temp", std::to_string(current_heater_bed_temperature) + "/");
    send_cmd_txt(tty_fd, "bed_target", std::to_string(current_heater_bed_target));
    // chris todo
    if (current_heater_bed_temperature >= current_heater_bed_target && current_extruder_temperature >= current_extruder_target)
    {
        idle_timeout_state = "Printing";
        page_to(TJC_PAGE_Z_INIT);
    }
}

void tjc_z_init_refresh()
{
    sleep(1);
    if (idle_timeout_state != "Printing")
    {
        page_to(TJC_PAGE_ZOFFSET);
    }
}

void tjc_zoffset_refresh()
{
    char txt_buf[30] = {0};
    // 显示手动设置步进值
    send_cmd_val(tty_fd, "zoffset_step", std::to_string((uint32_t)((level_zoffset_step + 0.001) * 100.0)));
    // 显示x轴手动设置值
    sprintf(txt_buf, "%.2f", level_zoffset_value);
    send_cmd_txt(tty_fd, "t1", txt_buf);
}

void tjc_head_bed_2_refresh()
{
    sleep(1);
    get_object_status();
    send_cmd_txt(tty_fd, "hot_bed_temp", std::to_string(current_heater_bed_temperature) + "/");
    send_cmd_txt(tty_fd, "bed_target", std::to_string(current_heater_bed_target));
    if (current_heater_bed_temperature == current_heater_bed_target)
    {
        auto_level_complete_flag = false;
        run_a_gcode("BED_MESH_CALIBRATE");
        page_to(TJC_PAGE_AUTO_LEVELING);
    }
}

void tjc_saving_refresh()
{
    sleep(5);
    if (webhooks_state == "ready")
    {
        system("sync");
        mkslog_print("tjc_saving_refresh. page to main");
        page_to(TJC_PAGE_MAIN);
    }
    else if (time_differ(20, save_time))
    {
        run_a_gcode("FIRMWARE_RESTART");
        save_time = time(NULL);
    }
}

// void tjc_auto_leveling_refresh()
// {
//     sleep(2);
//     if (auto_level_complete_flag == true)
//     {
//         run_a_gcode("SAVE_CONFIG");
//         save_time = time(NULL);
//         page_to(TJC_PAGE_SAVING);
//     }
// }

/* 报错信息 */
void tjc_error_refresh()
{
    // 打印出错进入重置文件页面，只进入一次
    if (webhooks_state == "ready" && current_print_state == "error")
    {
        if (current_page_id != TJC_PAGE_ERROR_RESET_FILE)
        {
            page_to(TJC_PAGE_ERROR_RESET_FILE);
            show_error_message();
        }
    }
    // 进入重启界面，只进入一次
    else if (webhooks_state == "shutdown" || webhooks_state == "error")
    {
        // 屏蔽正在保存中的所有错误
        if (current_page_id == TJC_PAGE_SAVING || current_page_id == TJC_PAGE_UPDATING)
            return;

        if (current_page_id != TJC_PAGE_ERROR_RESTART && is_errortip_show == false)
        {
            if (webhooks_state_message.find("FIRMWARE_RESTART") != std::string::npos)
            {
                is_errortip_show = true;
                page_to(TJC_PAGE_ERROR_RESTART);
                show_error_message();
            }
            // 来不及发错误信息的情况
            // if (webhooks_state_message == "")
            // {
            //     if (current_extruder_temperature < 0 || current_heater_bed_temperature < 0)
            //     {
            //         webhooks_state_message = "!! MCU 'mcu' shutdown: ADC out of range";
            //     }
            // }
            // show_error_message();
        }
    }

    // web端点击重启后屏幕也跳转
    if ((current_page_id == TJC_PAGE_ERROR_RESTART) || (current_page_id == TJC_PAGE_ERROR_RESET_FILE))
    {
        if (webhooks_state == "ready" && current_print_state != "error")
        {
            mkslog_print("tjc_error_refresh. page to main");
            is_errortip_show = false;
            page_to(TJC_PAGE_MAIN);
        }
    }
}

void tjc_starting_refresh()
{
    sleep(2);
    // std::string wcState = (std::string)"webhooks_state: " + webhooks_state + ", current_print_state: " + current_print_state;
    // mkslog_print(wcState.c_str());
    if ((webhooks_state == "ready") && (current_print_state == "standby"))
    {
        if (test_mode)
        {
            test_init();
            page_to(TJC_PAGE_MKS_HARDWARE_TEST);
            return;
        }

        if (!file_exists(GUIDE_PATH))  // first start up
        {
            page_to(TJC_PAGE_GUIDE_LANGUAGE);
        } else {
            if (if_need_reprint("/home/sovol/plr_parameter.txt") == true)
            {
                mkslog_print("tjc_starting_refresh. page to reprint");
                page_to(TJC_PAGE_IF_REPRINT);
            }
            else
            {
                mkslog_print("tjc_starting_refresh. page to main");
                page_to(TJC_PAGE_MAIN);
            }
        }
    }
}

// 计算剩余时间
int get_cal_printing_time(int print_time, int estimated_time, int progress)
{
    int left_time;
    int total_time = 0;
    if (progress <= 10)
    {
        total_time = estimated_time;
        left_time = total_time - print_time;
    }
    else if (progress > 10)
    {
        total_time = (print_time) * 100 / (progress);
        left_time = total_time - print_time;
    }
    return left_time;
}

/* 重启klipper */
void reset_klipper()
{
    webhooks_state_message = "";
    webhooks_state = "";
    idle_timeout_state = "";
    current_print_state = "";
    run_a_gcode("RESTART");
}

/* 重启客户端 */
void reset_firmware()
{
    webhooks_state_message = "";
    webhooks_state = "";
    idle_timeout_state = "";
    current_print_state = "";
    run_a_gcode("FIRMWARE_RESTART");
}
