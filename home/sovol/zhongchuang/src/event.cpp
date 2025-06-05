#include "../include/event.h"


WebSocketClient *ep;

int clock_for_check_time = 0; // 检测是否打印结束周期

extern float printer_bed_mesh_mesh_min[2];
extern float printer_bed_mesh_mesh_max[2];
extern float printer_bed_mesh_profiles_mks_points[6][6];
extern float printer_bed_mesh_profiles_mks_mesh_params_tension;
extern float printer_bed_mesh_profiles_mks_mesh_params_mesh_x_pps;
extern std::string printer_bed_mesh_profiles_mks_mesh_params_algo;
extern float printer_bed_mesh_profiles_mks_mesh_params_min_x;
extern float printer_bed_mesh_profiles_mks_mesh_params_min_y;
extern float printer_bed_mesh_profiles_mks_mesh_params_x_count;
extern float printer_bed_mesh_profiles_mks_mesh_params_y_count;
extern float printer_bed_mesh_profiles_mks_mesh_params_mesh_y_pps;
extern float printer_bed_mesh_profiles_mks_mesh_params_max_x;
extern float printer_bed_mesh_profiles_mks_mesh_params_max_y;

extern float printer_caselight_value;
extern float printer_caselight1_value;
extern float printer_caselight2_value;
extern float printer_caselight3_value;
extern bool printer_pause_resume_is_paused;

// oobe开箱引导
bool mks_oobe_enabled = false;
bool current_mks_oobe_enabled = false;

std::recursive_mutex sub_mutex;
std::recursive_mutex get_obj_mutex;

/*新增sled控制*/
extern bool sled1;
extern bool sled2;
extern bool sled3;

int time_differ(int duration, int start_time)
{
    int tmp;
    tmp = time(NULL);
    tmp = tmp - start_time;
    if (tmp > duration)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

int time_differ_ms(long duration, long start_time)
{
    struct timeval tv1;
    gettimeofday(&tv1, NULL);
    long tmp;
    tmp = tv1.tv_sec * 1000 + tv1.tv_usec / 1000;
    tmp = tmp - start_time;
    if (tmp > duration)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

bool time_differ_ms2(double duration_ms, struct timespec &start_time) {
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now); // 获取当前时间

    double elapsed_ms = (now.tv_sec - start_time.tv_sec) * 1000.0 +
                        (now.tv_nsec - start_time.tv_nsec) / 1000000.0;

    if (elapsed_ms > duration_ms) {
        start_time = now; // **重置起始时间**
        return true;
    }
    return false;
}

// 自动清理大日志
void auto_clean_big_log()
{
    static int start_time;
    if (time_differ(1749, start_time))
    {
        start_time = time(NULL);
        system("find /home/sovol/printer_data/logs/ -type f -name '*.log' -size +200M -delete");
    }
}

// 自动获取参数
void auto_get_object()
{
    // if (get_flag != true) return； chris TODO
    
    static int start_time;
    if (time_differ(5, start_time))
    {
        start_time = time(NULL);
        get_object_status();
    }
}

// 状态改变时输出调试信息
void printer_state_show()
{
    // 保存上一次状态
    static std::string current_print_state_back;
    static std::string idle_timeout_state_back;
    static std::string webhooks_state_back;
    static WPA0_STATUS wpa_state_back;

    if (current_print_state_back != current_print_state)
    {
        MKSLOG_GREEN("current_print_state 发生变化: %s", current_print_state.c_str());
        current_print_state_back = current_print_state;
    }
    if (idle_timeout_state_back != idle_timeout_state)
    {
        MKSLOG_GREEN("idle_timeout_state 发生变化: %s", idle_timeout_state.c_str());
        idle_timeout_state_back = idle_timeout_state;
    }
    if (webhooks_state_back != webhooks_state)
    {
        MKSLOG_GREEN("webhooks_state 发生变化: %s", webhooks_state.c_str());
        webhooks_state_back = webhooks_state;
    }
}

void run_a_gcode(std::string gcode)
{
    ep->send_message(json_run_a_gcode(gcode));
}

/* 订阅内容处理 */
void sub_object_status()
{
    if (sub_mutex.try_lock())
    {
        ep->send_message(json_subscribe_to_printer_object_status(subscribe_objects_status()));
        sub_mutex.unlock();
    }
}

/* 订阅内容 */
void get_object_status()
{
    if (get_obj_mutex.try_lock())
    {
        ep->send_message(json_query_printer_object_status(subscribe_objects_status()));
        get_obj_mutex.unlock();
    }
}

/* 开始打印 */
void start_printing(std::string filepath)
{
    ep->send_message(json_print_a_file(filepath));
}

/* 设置目标温度 */
void set_extruder_target(int target)
{
    run_a_gcode(set_heater_temp("extruder", target));
}
void set_heater_bed_target(int target)
{
    run_a_gcode(set_heater_temp("heater_bed", target));
}
void set_chamber_target(int target)
{
    run_a_gcode(set_chamber_temp(target));
}
/* 设置风扇 */
std::string set_fan_speed(std::string num, int speed)
{
    // std::string speed_temp = std::to_string(int(float(int(speed) % 101) / 100 * 255));
    // return SET_FAN_SPEED + (std::string) " S" + speed_temp;
    std::stringstream ss;
    ss << std::fixed << std::setprecision(2) << (speed / 100.0);
    std::string speed_temp = ss.str();
    std::cout << "speed_temp: " << speed_temp << std::endl;
    return "SET_FAN_SPEED" + (std::string) " FAN=fan" + num + " SPEED=" + speed_temp;
}

/* 设置zoffset */
void set_zoffset(bool positive, float value)
{
    if (positive == true)
    {
        run_a_gcode("SET_GCODE_OFFSET Z_ADJUST=+" + std::to_string(value) + " MOVE=1");
    }
    else
    {
        run_a_gcode("SET_GCODE_OFFSET Z_ADJUST=-" + std::to_string(value) + " MOVE=1");
    }
}

/* 设置打印速度 */
void set_printer_speed(int speed)
{
    std::cout << "Rate = " << std::to_string(speed) << std::endl;
    run_a_gcode(set_speed_rate(std::to_string(speed)));
}
void set_printer_flow(int rate)
{
    run_a_gcode("M221 S" + (std::to_string(rate)));
}

/* 时间转换 */
std::string show_time(int seconds)
{
    if ((int)(seconds / 3600) > 0)
    {
        return std::to_string((int)(seconds / 3600)) + "h" + std::to_string((int)((seconds % 3600) / 60)) + "m";
    }
    else
    {
        return std::to_string((int)((seconds % 3600) / 60)) + "m";
    }
}

/* 移动相关API */
void move_home()
{
    run_a_gcode("G28");
}
void move_home_x()
{
    run_a_gcode("G28 X");
}
void move_home_y()
{
    run_a_gcode("G28 Y");
}
void move_home_z()
{
    run_a_gcode("G28 Z");
}

void move_x_decrease(float value)
{
    ep->send_message(move(AXIS_X, "-" + std::to_string(value), 130));
}
void move_x_increase(float value)
{
    ep->send_message(move(AXIS_X, "+" + std::to_string(value), 130));
}
void move_y_decrease(float value)
{
    ep->send_message(move(AXIS_Y, "-" + std::to_string(value), 130));
}
void move_y_increase(float value)
{
    ep->send_message(move(AXIS_Y, "+" + std::to_string(value), 130));
}
void move_z_decrease(float value)
{
    if (z_position <= 0)
    {
        return;
    }
    ep->send_message(move(AXIS_Z, "-" + std::to_string(value <= z_position ? value : z_position), 130));
}
void move_z_increase(float value)
{
    ep->send_message(move(AXIS_Z, "+" + std::to_string(value), 130));
}
void motors_off()
{
    run_a_gcode("M84");
}

/*获取预估时间*/
int get_estimated_time(std::string estime_key)
{
    int estimated_time = 0;
    if (gcode_metadata_estime_dic.count(estime_key) > 0)
    {
        std::stringstream sstream;
        sstream << gcode_metadata_estime_dic[estime_key];
        sstream >> estimated_time;
        return estimated_time;
    }
    return 0;
}

/*获取预估时间并转换格式*/
std::string get_estimated_show_time(std::string estime_key)
{
    int estimated_time = get_estimated_time(estime_key);
    if (estimated_time > 0)
    {
        return show_time(estimated_time);
    }
    return "--";
}

/*本地获取预估耗材量并转换格式*/
std::string get_estimated_filament(std::string esfile_key)
{
    std::string fila_tmp;
    if (gcode_metadata_esfila_dic.count(esfile_key) > 0)
    {
        fila_tmp = gcode_metadata_esfila_dic[esfile_key];
        return fila_tmp.substr(0, 3) + "m";
    }
    return "--";
}

// chris todo
void _gcode_scanmeta(std::string filepath)
{
    ep->send_message(json_gcode_metascan(filepath));
}

/*从web端获取预估耗材量*/
void _get_gcode_metadata(std::string filepath)
{
    ep->send_message(json_get_gcode_metadata(filepath));
}

/* led1开关 */
void led1_on_off()
{
    if (sled1)
    {
        sled1 = false;
        run_a_gcode("SET_PIN PIN=main_led VALUE=0");  // set_pin_value(0)
    }
    else
    {
        sled1 = true;
        run_a_gcode("SET_PIN PIN=main_led VALUE=1");  // set_pin_value(1)
    }
}

/* 检测是否打印结束 */
void print_finish_check()
{
    switch (current_page_id)
    {
    case TJC_PAGE_PRINTING:
    case TJC_PAGE_OPERATE:
    case TJC_PAGE_IF_STOP:
        // 打印结束
        if ((current_print_state == "complete") && (idle_timeout_state == "Ready"))
        {
            std::cout << "正常结束打印" << std::endl;
            check_filament_flag = false;
            cancel_plr();
            page_to(TJC_PAGE_PRINT_FILISH); 
            send_cmd_txt(tty_fd, "t0", printer_print_stats_filename);
            send_cmd_txt(tty_fd, "t1", show_time((int)(printing_times)));
            run_a_gcode("SDCARD_RESET_FILE");
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
        else if(current_print_state == "cancelled" && idle_timeout_state == "Ready")
        {
            std::string msgStr = "print_finish_check, page id: ";
            msgStr = msgStr + std::to_string(current_page_id);
            mkslog_print(msgStr.c_str());
            page_to(TJC_PAGE_MAIN);
        }

        // 断料检测
        if ((filament_switch_sensor_enabled && !filament_switch_sensor_filament_detected) && check_filament_flag && filament_sensor_print)
        {
            MKSLOG_HIGHLIGHT("打印后检测到无耗材");
            check_filament_flag = false;
            // run_a_gcode("PAUSE");
            force_page_to(TJC_PAGE_FILAMENT_ERROR);
        }
        break;

    default:
        check_filament_flag = false;
        break;
    }
}

/*显示报错信息*/
void show_error_message()
{
    std::string temp = webhooks_state_message;
    replace(temp.begin(), temp.end(), '\n', ' ');
    replace(temp.begin(), temp.end(), '/', ' ');
    replace(temp.begin(), temp.end(), '\"', ' ');
    replace(temp.begin(), temp.end(), '\'', ' ');

    send_cmd_txt(tty_fd, "t0", temp);
    mkslog_print(temp.c_str());
}

/*取消打印*/
void cancel_print()
{
    /* 执行这个避免一直加热 */
    set_extruder_target(0);
    usleep(4000);
    set_heater_bed_target(0);
    usleep(4000);
    run_a_gcode("SDCARD_RESET_FILE");
    run_a_gcode("CANCEL_PRINT");
    // run_a_gcode("PRINT_END");
    cancel_plr();
}

void emergency_stop()
{
    ep->send_message(json_emergency_stop());
}

// 设置是否使能断料检测功能
void set_filament_sensor(bool status)
{
    if (true == status)
    {
        run_a_gcode("SET_FILAMENT_SENSOR SENSOR=filament_sensor ENABLE=1");
    }
    else
    {
        run_a_gcode("SET_FILAMENT_SENSOR SENSOR=filament_sensor ENABLE=0");
    }
}

// 断电续打
void start_plr()
{
    char buf[100] = {0};
    get_reprint_parameter();
    sprintf(buf, "G92 E%.3f", mks_reprint_parameter.e_pos);
    MKSLOG_HIGHLIGHT("buf = %s", buf);
    run_a_gcode(std::string(buf));
    //E轴是否使用相对坐标
    if(mks_reprint_parameter.if_absolute_extrude == 0)
    {
        run_a_gcode("M83");
        MKSLOG_HIGHLIGHT("M83");
    }
    run_a_gcode("RESUME_INTERRUPTED");
}

// 取消断电续打
void cancel_plr()
{
    save_reprint_parameter("IF_REPRINT", 0.0f);
    run_a_gcode("clear_last_file");
    // run_a_gcode("SAVE_VARIABLE VARIABLE=was_interrupted VALUE=False");
    run_a_gcode("RUN_SHELL_COMMAND CMD=clear_plr");
}
