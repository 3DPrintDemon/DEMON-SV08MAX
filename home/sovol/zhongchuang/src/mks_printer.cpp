#include <iostream>

#include "../include/mks_printer.h"

// webhooks
std::string webhooks_state;         // web端状态
std::string webhooks_state_message; // web端信息

// gcode_move
float current_move_speed_factor;
float current_move_speed;
float current_move_extrude_factor;
float printer_gcode_move_homing_origin[4]; // [X, Y, Z, E] - 返回应用于每个轴的“gcode 偏移”。例如，可以检查“Z”轴以确定通过“babystepping”应用了多少偏移量。
float printer_gcode_move_gcode_position[4];

// toolhead
double printer_toolhead_position[4];
double printer_toolhead_axis_minimum[4];
double printer_toolhead_axis_maximum[4];
float printer_toolhead_max_velocity;
float printer_toolhead_max_accel;
float printer_toolhead_max_accel_to_decel;

// x, y, z坐标
double x_position;
double y_position;
double z_position;

float e_position;

// extruder
int current_extruder_temperature;
int current_extruder_target;
int current_extruder_target_back;

// heater bed
int current_heater_bed_temperature;
int current_heater_bed_target;

// chamber bed
int current_chamber_temperature;
int current_chamber_target;

// hot
int printer_hot_temperature;
int printer_hot_target;

// fan
float current_fan_speed;
float generic_fan1_speed;
float generic_fan2_speed;
float generic_fan3_speed;

// heater fan
float printer_heater_fan_speed;

// heater_fan my_nozzle_fan1
float current_model_fan_speed;

// output_pin fan0
float printer_out_pin_fan0_value;

// output_pin fan2
float printer_out_pin_fan2_value;

float printer_out_pin_beep_value;

// print stats
std::string printer_print_stats_filename; // 正在打印的文件名
std::string printer_print_stats_filename_back; // 正在打印的文件名
float printing_total_times;               // 打印结束总时间
float printing_times;                     // 实时打印时间
std::string idle_timeout_state;           // 电机是否移动（"Ready"，"Printing"）
std::string current_print_state;          // 打印机状态（"printing"，"paused"）
int printer_display_status_progress = 0;  // 打印进度百分比
int printer_display_status_progress_back = 0;   // 打印进度百分比备份
double printer_status_progress = 0.0;

// metadata
float metadata_estimated_time = 0;
std::string metadata_filename;
float metadata_filament_total = 0.0;
int metadata_object_height = 0;
std::string metadata_filament_name;
std::string metadata_filament_type;
float metadata_filament_weight_total = 0.0;
std::string thumbnail_relative_path;
std::string thumbnail_path;

float printer_bed_mesh_mesh_min[2] = {0.0, 0.0};
float printer_bed_mesh_mesh_max[2] = {0.0, 0.0};
float printer_bed_mesh_profiles_mks_points[6][6] = {0.0};
float bed_mesh_default_value[11][11] = {0.0}; // 调平默认值

float printer_bed_mesh_profiles_mks_mesh_params_tension = 0.0;
float printer_bed_mesh_profiles_mks_mesh_params_mesh_x_pps = 0;
std::string printer_bed_mesh_profiles_mks_mesh_params_algo = "";
float printer_bed_mesh_profiles_mks_mesh_params_min_x = 0;
float printer_bed_mesh_profiles_mks_mesh_params_min_y = 0;
float printer_bed_mesh_profiles_mks_mesh_params_x_count = 0;
float printer_bed_mesh_profiles_mks_mesh_params_y_count = 0;
float printer_bed_mesh_profiles_mks_mesh_params_mesh_y_pps = 0;
float printer_bed_mesh_profiles_mks_mesh_params_max_x = 0;
float printer_bed_mesh_profiles_mks_mesh_params_max_y = 0;

bool printer_pause_resume_is_paused;

// filament switch sensor fila
bool filament_switch_sensor_filament_detected = true; // true为有料
bool filament_switch_sensor_enabled = true;
bool before_print_filament_error;       //打印前检测到无耗材标志位

float printer_caselight_value = 0;

// probe
float printer_probe_x_zoffset = 0.0;
float printer_probe_y_zoffset = 0.0;
float printer_probe_z_zoffset = 0.0;

// printer info software version
std::string printer_info_software_version;

// server history totals
double total_time = 0.0;
double total_print_time = 0.0;

bool has_chamber_fan = false;
bool has_chamber_temp = false;
// 是否已回零
std::string printer_toolhead_homed_axes;

bool plug_status = false;  //dutou
bool winding_status = false;  //canrao
bool filament_sensor_print = false; //

void set_printer_state(std::string printer_state) {
    idle_timeout_state = printer_state;
}


// 获取网页状态
void parse_webhooks(nlohmann::json webhooks)
{
    if (webhooks["state"] != nlohmann::detail::value_t::null)
    {
        webhooks_state = webhooks["state"];
    }
    if (webhooks["state_message"] != nlohmann::detail::value_t::null)
    {
        webhooks_state_message = webhooks["state_message"];
    }
}

// 获取电机移动状态
void parse_idle_timeout(nlohmann::json idle_timeout)
{
    if (idle_timeout["state"] != nlohmann::detail::value_t::null)
    {
        idle_timeout_state = idle_timeout["state"];
    }
}

// 解析获取打印状态
void parse_print_stats(nlohmann::json print_stats)
{
    if (print_stats["state"] != nlohmann::detail::value_t::null)
    {
        current_print_state = print_stats["state"];
    }
    if (print_stats["filename"] != nlohmann::detail::value_t::null)
    {
        printer_print_stats_filename = print_stats["filename"];
    }
    if (print_stats["print_duration"] != nlohmann::detail::value_t::null)
    {
        printing_times = print_stats["print_duration"];
    }
    if (print_stats["total_duration"] != nlohmann::detail::value_t::null)
    {
        printing_total_times = print_stats["total_duration"];
    }
}

// 解析获取各轴zoffset
void parse_printer_probe(nlohmann::json probe)
{
    if (probe["x_offset"] != nlohmann::detail::value_t::null)
    {
        printer_probe_x_zoffset = probe["x_offset"];
    }
    if (probe["y_offset"] != nlohmann::detail::value_t::null)
    {
        printer_probe_y_zoffset = probe["y_offset"];
    }
    if (probe["z_offset"] != nlohmann::detail::value_t::null)
    {
        printer_probe_z_zoffset = probe["z_offset"];
        // MKSLOG_HIGHLIGHT("printer_probe_z_zoffset = ", printer_probe_z_zoffset);
    }
}

// 解析获取蜂鸣器值
void parse_printer_beep(nlohmann::json beep)
{
    if (beep["value"] != nlohmann::detail::value_t::null)
    {
        printer_out_pin_beep_value = beep["value"];
        MKSLOG_BLUE("printer_out_pin_beep_value = %f", printer_out_pin_beep_value);
    }
}

// 获取照明灯数值
void parse_printer_caselight(nlohmann::json caselight)
{
    if (caselight["value"] != nlohmann::detail::value_t::null)
    {
        printer_caselight_value = caselight["value"];
    }
}

// 获取风扇数值
void parse_model_fan_speed(nlohmann::json model_fan_speed)
{
    if (model_fan_speed["speed"] != nlohmann::detail::value_t::null)
    {
        current_model_fan_speed = model_fan_speed["speed"];
    }
}

// 获取风扇0速度
void parse_printer_out_pin_fan0(nlohmann::json out_pin_fan0)
{
    if (out_pin_fan0["value"] != nlohmann::detail::value_t::null)
    {
        printer_out_pin_fan0_value = out_pin_fan0["value"];
    }
};

// 获取风扇2速度
void parse_printer_out_pin_fan2(nlohmann::json out_pin_fan2)
{
    if (out_pin_fan2["value"] != nlohmann::detail::value_t::null)
    {
        printer_out_pin_fan2_value = out_pin_fan2["value"];
    }
};

// 获取断料检测模块状态
void parse_filament_switch_sensor_fila(nlohmann::json filament_switch_sensor)
{
    if (filament_switch_sensor["filament_detected"] != nlohmann::detail::value_t::null)
    {
        filament_switch_sensor_filament_detected = filament_switch_sensor["filament_detected"];
        MKSLOG_GREEN("检测到是否有料: %d", filament_switch_sensor_filament_detected);
    }
    if (filament_switch_sensor["enabled"] != nlohmann::detail::value_t::null)
    {
        filament_switch_sensor_enabled = filament_switch_sensor["enabled"];
        MKSLOG_GREEN("断料检测开关为: %d", filament_switch_sensor_enabled);
    }
}

// 获取自动调平数值
void parse_bed_mesh(nlohmann::json bed_mesh)
{
    if (bed_mesh["mesh_min"] != nlohmann::detail::value_t::null)
    {
        if (bed_mesh["mesh_min"][0] != nlohmann::detail::value_t::null)
        {
            printer_bed_mesh_mesh_min[0] = bed_mesh["mesh_min"][0];
        }
        if (bed_mesh["mesh_min"][1] != nlohmann::detail::value_t::null)
        {
            printer_bed_mesh_mesh_min[1] = bed_mesh["mesh_min"][1];
        }
    }
    if (bed_mesh["mesh_max"] != nlohmann::detail::value_t::null)
    {
        if (bed_mesh["mesh_max"][0] != nlohmann::detail::value_t::null)
        {
            printer_bed_mesh_mesh_max[0] = bed_mesh["mesh_max"][0];
        }
        if (bed_mesh["mesh_max"][1] != nlohmann::detail::value_t::null)
        {
            printer_bed_mesh_mesh_max[1] = bed_mesh["mesh_max"][1];
        }
    }
    if (bed_mesh["profiles"] != nlohmann::detail::value_t::null)
    {
        // 获取默认值
        if (bed_mesh["profiles"]["default"] != nlohmann::detail::value_t::null)
        {
            if (bed_mesh["profiles"]["default"]["points"] != nlohmann::detail::value_t::null)
            {

                for (int i = 0; i < 11; i++)
                {
                    for (int j = 0; j < 11; j++)
                    {
                        if (bed_mesh["profiles"]["default"]["points"][i][j] != nlohmann::detail::value_t::null)
                        {
                            bed_mesh_default_value[i][j] = bed_mesh["profiles"]["default"]["points"][i][j];
                        }
                    }
                }
            }
            if (bed_mesh["profiles"]["default"]["mesh_params"] != nlohmann::detail::value_t::null)
            {
                if (bed_mesh["profiles"]["default"]["mesh_params"]["tension"] != nlohmann::detail::value_t::null)
                {
                    printer_bed_mesh_profiles_mks_mesh_params_tension = bed_mesh["profiles"]["default"]["mesh_params"]["tension"];
                }
                if (bed_mesh["profiles"]["default"]["mesh_params"]["mesh_x_pps"] != nlohmann::detail::value_t::null)
                {
                    printer_bed_mesh_profiles_mks_mesh_params_mesh_x_pps = bed_mesh["profiles"]["default"]["mesh_params"]["mesh_x_pps"];
                }
                if (bed_mesh["profiles"]["default"]["mesh_params"]["algo"] != nlohmann::detail::value_t::null)
                {
                    printer_bed_mesh_profiles_mks_mesh_params_algo = bed_mesh["profiles"]["default"]["mesh_params"]["algo"];
                }
                if (bed_mesh["profiles"]["default"]["mesh_params"]["min_x"] != nlohmann::detail::value_t::null)
                {
                    printer_bed_mesh_profiles_mks_mesh_params_min_x = bed_mesh["profiles"]["default"]["mesh_params"]["min_x"];
                    // std::cout << "printer_bed_mesh_profiles_mks_mesh_params_min_x = " << printer_bed_mesh_profiles_mks_mesh_params_min_x << std::endl;
                }
                if (bed_mesh["profiles"]["default"]["mesh_params"]["min_y"] != nlohmann::detail::value_t::null)
                {
                    printer_bed_mesh_profiles_mks_mesh_params_min_y = bed_mesh["profiles"]["default"]["mesh_params"]["min_y"];
                }
                if (bed_mesh["profiles"]["default"]["mesh_params"]["y_count"] != nlohmann::detail::value_t::null)
                {
                    printer_bed_mesh_profiles_mks_mesh_params_y_count = bed_mesh["profiles"]["default"]["mesh_params"]["y_count"];
                }
                if (bed_mesh["profiles"]["default"]["mesh_params"]["mesh_y_pps"] != nlohmann::detail::value_t::null)
                {
                    printer_bed_mesh_profiles_mks_mesh_params_mesh_y_pps = bed_mesh["profiles"]["default"]["mesh_params"]["mesh_y_pps"];
                }
                if (bed_mesh["profiles"]["default"]["mesh_params"]["x_count"] != nlohmann::detail::value_t::null)
                {
                    printer_bed_mesh_profiles_mks_mesh_params_x_count = bed_mesh["profiles"]["default"]["mesh_params"]["x_count"];
                }
                if (bed_mesh["profiles"]["default"]["mesh_params"]["max_x"] != nlohmann::detail::value_t::null)
                {
                    printer_bed_mesh_profiles_mks_mesh_params_max_x = bed_mesh["profiles"]["default"]["mesh_params"]["max_x"];
                }
                if (bed_mesh["profiles"]["default"]["mesh_params"]["max_y"] != nlohmann::detail::value_t::null)
                {
                    printer_bed_mesh_profiles_mks_mesh_params_max_y = bed_mesh["profiles"]["default"]["mesh_params"]["max_y"];
                }
            }
        }
    }
}

// 获取移动速度
void parse_gcode_move(nlohmann::json gcode_move)
{
    if (gcode_move["speed_factor"] != nlohmann::detail::value_t::null)
    {
        current_move_speed_factor = gcode_move["speed_factor"];
    }
    if (gcode_move["speed"] != nlohmann::detail::value_t::null)
    {
        current_move_speed = gcode_move["speed"];
    }
    if (gcode_move["extrude_factor"] != nlohmann::detail::value_t::null)
    {
        current_move_extrude_factor = gcode_move["extrude_factor"];
    }
    if (gcode_move["homing_origin"] != nlohmann::detail::value_t::null)
    {
        for (int i = 0; i < 4; i++)
        {
            if (gcode_move["homing_origin"][i] != nlohmann::detail::value_t::null)
            {
                printer_gcode_move_homing_origin[i] = gcode_move["homing_origin"][i];
            }
        }
    }
    if (gcode_move["gcode_position"] != nlohmann::detail::value_t::null)
    {
        //断电续打
        if (gcode_move["gcode_position"][2] != nlohmann::detail::value_t::null)
        {
            printer_gcode_move_gcode_position[2] = gcode_move["gcode_position"][2];
            printer_gcode_move_gcode_position[3] = gcode_move["gcode_position"][3];
            if (current_print_state == "printing" && current_extruder_target > 0 && check_filament_flag)
            {
                save_z_e_value(printer_gcode_move_gcode_position[2], printer_gcode_move_gcode_position[3]);
            }
        }
    }
    if (gcode_move["position"] != nlohmann::detail::value_t::null)
    {
        if (gcode_move["position"][2] != nlohmann::detail::value_t::null)
        {
        }
    }

    if (gcode_move["absolute_extrude"] != nlohmann::detail::value_t::null)
    {
        mks_reprint_parameter.if_absolute_extrude = gcode_move["absolute_extrude"];
    }
}

void parse_toolhead(nlohmann::json toolhead)
{
    // 解析获取各轴回零状态
    if (toolhead["homed_axes"] != nlohmann::detail::value_t::null)
    {
        printer_toolhead_homed_axes = toolhead["homed_axes"];
        // MKSLOG_HIGHLIGHT("printer_toolhead_homed_axes: %s", printer_toolhead_homed_axes.c_str());
    }
    //
    if (toolhead["position"] != nlohmann::detail::value_t::null)
    {
        if (toolhead["position"][0] != nlohmann::detail::value_t::null)
        {
            printer_toolhead_position[0] = toolhead["position"][0];
            x_position = round(printer_toolhead_position[0] * 10) / 10;
        }
        if (toolhead["position"][1] != nlohmann::detail::value_t::null)
        {
            printer_toolhead_position[1] = toolhead["position"][1];
            y_position = round(printer_toolhead_position[1] * 10) / 10;
        }
        if (toolhead["position"][2] != nlohmann::detail::value_t::null)
        {
            printer_toolhead_position[2] = toolhead["position"][2];
            // std::cout << "printer_toolhead_position[2] = " << std::to_string(printer_toolhead_position[2]) << std::endl;
            z_position = round(printer_toolhead_position[2] * 10) / 10;
            // std::cout << "z_position = " << std::to_string(z_position) << std::endl;
        }
        if (toolhead["position"][3] != nlohmann::detail::value_t::null)
        {
            // std::cout << "z_position: " << z_position << std::endl;
            printer_toolhead_position[3] = toolhead["position"][3];
            // std::cout << "printer_toolhead_position z == " << printer_toolhead_position << std::endl;
        }
    }

    if (toolhead["max_velocity"] != nlohmann::detail::value_t::null)
    {
        printer_toolhead_max_velocity = toolhead["max_velocity"];
        // std::cout << "printer_toolhead_max_velocity: " << printer_toolhead_max_velocity << std::endl;
    }
    if (toolhead["max_accel"] != nlohmann::detail::value_t::null)
    {
        printer_toolhead_max_accel = toolhead["max_accel"];
        // std::cout << "printer_toolhead_max_accel: " << printer_toolhead_max_accel << std::endl;
    }
    if (toolhead["max_accel_to_decel"] != nlohmann::detail::value_t::null)
    {
        printer_toolhead_max_accel_to_decel = toolhead["max_accel_to_decel"];
        // std::cout << "printer_toolhead_max_accel_to_decel: " << printer_toolhead_max_accel_to_decel << std::endl;
    }

    if (toolhead["axis_minimum"] != nlohmann::detail::value_t::null)
    {
        for (int i = 0; i < 4; i++)
        {
            if (toolhead["axis_minimum"][i] != nlohmann::detail::value_t::null)
            {
                printer_toolhead_axis_minimum[i] = toolhead["axis_minimum"][i];
            }
        }
    }

    if (toolhead["axis_maximum"] != nlohmann::detail::value_t::null)
    {
        // MKSLOG_HIGHLIGHT("toolhead != nlohmann::detail::value_t::nul %s", toolhead["axis_maximum"]);
        // std::cout << toolhead["axis_maximum"] << std::endl;
        for (int j = 0; j < 4; j++)
        {
            if (toolhead["axis_maximum"][j] != nlohmann::detail::value_t::null)
            {
                printer_toolhead_axis_maximum[j] = toolhead["axis_maximum"][j];
                // MKSLOG_HIGHLIGHT("printer_toolhead_axis_maximum = %.2f", printer_toolhead_axis_maximum[j]);
            }
        }
    }
}

// 解析获取喷头参数
void parse_extruder(nlohmann::json extruder)
{
    float temp;
    if (extruder["temperature"] != nlohmann::detail::value_t::null)
    {
        temp = extruder["temperature"];
        current_extruder_temperature = (int)(temp + 0.5);
        // MKSLOG_GREEN("获取到挤出头当前温度为%d", current_extruder_temperature);
    }
    if (extruder["target"] != nlohmann::detail::value_t::null)
    {
        temp = extruder["target"];
        current_extruder_target = (int)(temp + 0.5);
        // MKSLOG_GREEN("获取到挤出头目标温度为%d", current_extruder_target);
    }
}

void parse_chamber(nlohmann::json chamber)
{
    float temp;
    if (chamber["temperature"] != nlohmann::detail::value_t::null)
    {
        has_chamber_temp = true;
        temp = chamber["temperature"];
        current_chamber_temperature = (int)(temp + 0.5);
        MKSLOG_GREEN("获取到chamber当前温度为%d", current_chamber_temperature);
    }
    if (chamber["target"] != nlohmann::detail::value_t::null)
    {
        temp = chamber["target"];
        current_chamber_target = (int)(temp + 0.5);
        MKSLOG_GREEN("获取到chamber目标温度为%d", current_chamber_target);
    }
}

// 解析获取热床参数
void parse_heater_bed(nlohmann::json heater_bed)
{
    float temp;
    if (heater_bed["temperature"] != nlohmann::detail::value_t::null)
    {
        temp = heater_bed["temperature"];
        current_heater_bed_temperature = (int)(temp + 0.5);
        // MKSLOG_GREEN("获取到热床当前温度为%d", current_heater_bed_temperature);
    }
    if (heater_bed["target"] != nlohmann::detail::value_t::null)
    {
        temp = heater_bed["target"];
        current_heater_bed_target = (int)(temp + 0.5);
        // MKSLOG_GREEN("获取到热床目标温度为%d", current_heater_bed_target);
    }
}

void parse_heater_generic_hot(nlohmann::json heater_generic_hot)
{
    float temp;
    if (heater_generic_hot["temperature"] != nlohmann::detail::value_t::null)
    {
        temp = heater_generic_hot["temperature"];
        printer_hot_temperature = (int)(temp + 0.5);
    }
    if (heater_generic_hot["target"] != nlohmann::detail::value_t::null)
    {
        temp = heater_generic_hot["target"];
        printer_hot_target = (int)(temp + 0.5);
    }
}

// 解析获取风扇速度
void parse_fan(nlohmann::json fan, int num)
{
    if (fan["speed"] != nlohmann::detail::value_t::null)
    {
        if (num == 0) {
            current_fan_speed = fan["speed"];
            MKSLOG_YELLOW("current_fan_speed = %.2f", current_fan_speed);
        }else if (num == 1) {
            generic_fan1_speed = fan["speed"];
            // MKSLOG_YELLOW("generic_fan1_speed = %.2f", generic_fan1_speed);
        }else if (num == 2) {
            generic_fan2_speed = fan["speed"];
            has_chamber_fan = true;
            // MKSLOG_YELLOW("generic_fan2_speed = %.2f", generic_fan2_speed);
        }else if (num == 3) {
            generic_fan3_speed = fan["speed"];
            has_chamber_fan = true;
            // MKSLOG_YELLOW("generic_fan3_speed = %.2f", generic_fan3_speed);
        }
    }
    // MKSLOG_YELLOW("has_chamber_fan: %d", has_chamber_fan);
}

// 解析获取加热风扇速度
void parse_heater_fan(nlohmann::json heater_fan)
{
    if (heater_fan["speed"] != nlohmann::detail::value_t::null)
    {
        printer_heater_fan_speed = heater_fan["speed"];
    }
}

void parse_plus_winding(nlohmann::json plug_winding)
{
    if (plug_winding["plug_status"] != nlohmann::detail::value_t::null)
    {
        plug_status = plug_winding["plug_status"];
        MKSLOG_YELLOW("plug_status: %d", plug_status);
    }
    if (plug_winding["winding_status"] != nlohmann::detail::value_t::null)
    {
        winding_status = plug_winding["winding_status"];
        MKSLOG_YELLOW("winding_status: %d", winding_status);
    }
}

void parse_filament_print(nlohmann::json filament_sensor)
{
    if (filament_sensor["filament_sensor_print"] != nlohmann::detail::value_t::null)
    {
        filament_sensor_print = filament_sensor["filament_sensor_print"];
        MKSLOG_YELLOW("filament_sensor_print: %d", filament_sensor_print);
    }
}

// 解析获取进度条状态
void parse_display_status(nlohmann::json display_status)
{
    if (display_status["progress"] != nlohmann::detail::value_t::null)
    {
        printer_status_progress = display_status["progress"];   
        printer_display_status_progress = (int)(printer_status_progress * 100);
        // MKSLOG_HIGHLIGHT("当前进度为%d", printer_status_progress);
    }
}

// 解析获取暂停状态
void parse_pause_resume(nlohmann::json pause_resume)
{
    // std::cout << pause_resume << std::endl;
    if (pause_resume["is_paused"] != nlohmann::detail::value_t::null)
    {
        printer_pause_resume_is_paused = pause_resume["is_paused"];
    }
}

// 解析获取总打印时间
void parse_server_history_totals(nlohmann::json totals)
{
    if (totals["total_print_time"] != nlohmann::detail::value_t::null)
    {
        total_print_time = totals["total_print_time"];
    }
}

// 解析获取版本信息
void parse_printer_info(nlohmann::json result)
{
    if (result["software_version"] != nlohmann::detail::value_t::null)
    {
        printer_info_software_version = result["software_version"];
        MKSLOG_RED("Version: %s", printer_info_software_version.c_str());
    }
}

// 解析订阅参数汇总
void parse_subscribe_objects_status(nlohmann::json status)
{
    if (status["idle_timeout"] != nlohmann::detail::value_t::null)
    {
        parse_idle_timeout(status["idle_timeout"]);
    }
    if (status["webhooks"] != nlohmann::detail::value_t::null)
    {
        parse_webhooks(status["webhooks"]);
    }
    if (status["bed_mesh"] != nlohmann::detail::value_t::null)
    {
        parse_bed_mesh(status["bed_mesh"]);
    }

    if (status["gcode_move"] != nlohmann::detail::value_t::null)
    {
        parse_gcode_move(status["gcode_move"]);
    }
    if (status["toolhead"] != nlohmann::detail::value_t::null)
    {
        parse_toolhead(status["toolhead"]);
    }
    if (status["extruder"] != nlohmann::detail::value_t::null)
    {
        parse_extruder(status["extruder"]);
    }
    if (status["heater_bed"] != nlohmann::detail::value_t::null)
    {
        parse_heater_bed(status["heater_bed"]);
    }
    if (status["heater_generic chamber_temp"] != nlohmann::detail::value_t::null)
    {
        parse_chamber(status["heater_generic chamber_temp"]);
    }
    if (status["heater_generic hot"] != nlohmann::detail::value_t::null)
    {
        parse_heater_generic_hot(status["heater_generic hot"]);
    }
    if (status["fan_generic fan0"] != nlohmann::detail::value_t::null)  //front model cool fan
    {
        parse_fan(status["fan_generic fan0"], 0);
    }
    if (status["fan_generic fan1"] != nlohmann::detail::value_t::null)  //back model cool fan
    {
        parse_fan(status["fan_generic fan1"], 1);
    }
    if (status["fan_generic fan2"] != nlohmann::detail::value_t::null)  //auxiliary fan
    {
        parse_fan(status["fan_generic fan2"], 2);
    }
    if (status["fan_generic fan3"] != nlohmann::detail::value_t::null)  //exhaust fan
    {
        parse_fan(status["fan_generic fan3"], 3);
    }
    if (status["heater_fan fan1"] != nlohmann::detail::value_t::null)
    {
        parse_heater_fan(status["heater_fan fan1"]);
    }
    if (status["gcode_macro variables"] != nlohmann::detail::value_t::null)
    {
        parse_plus_winding(status["gcode_macro variables"]);
    }
    if (status["gcode_macro _global_var"] != nlohmann::detail::value_t::null)
    {
        parse_filament_print(status["gcode_macro _global_var"]);
    }
    if (status["pause_resume"] != nlohmann::detail::value_t::null)
    {
        parse_pause_resume(status["pause_resume"]);
    }
    if (status["print_stats"] != nlohmann::detail::value_t::null)
    {
        parse_print_stats(status["print_stats"]);
    }
    if (status["display_status"] != nlohmann::detail::value_t::null)
    {
        parse_display_status(status["display_status"]);
    }
    if (status["heater_fan model_fan"] != nlohmann::detail::value_t::null)
    {
        parse_model_fan_speed(status["heater_fan model_fan"]);
    }
    if (status["output_pin fan0"] != nlohmann::detail::value_t::null)
    {
        parse_printer_out_pin_fan0(status["output_pin fan0"]);
    }
    if (status["output_pin fan2"] != nlohmann::detail::value_t::null)
    {
        parse_printer_out_pin_fan2(status["output_pin fan2"]);
    }
    if (status["filament_switch_sensor filament_sensor"] != nlohmann::detail::value_t::null)
    {
        parse_filament_switch_sensor_fila(status["filament_switch_sensor filament_sensor"]);
    }
    if (status["output_pin caselight"] != nlohmann::detail::value_t::null)
    {
        parse_printer_caselight(status["output_pin caselight"]);
    }
    if (status["output_pin sound"] != nlohmann::detail::value_t::null)
    {
        parse_printer_beep(status["output_pin sound"]);
    }
    if (status["probe"] != nlohmann::detail::value_t::null)
    {
        parse_printer_probe(status["probe"]);
    }
}

// 订阅参数
nlohmann::json subscribe_objects_status()
{
    nlohmann::json objects;
    objects["extruder"];
    objects["heater_bed"];
    objects["heater_generic chamber_temp"];
    objects["gcode_move"];
    objects["fan_generic fan0"] = {"speed"};
    objects["fan_generic fan1"] = {"speed"};
    objects["fan_generic fan2"] = {"speed"};
    objects["fan_generic fan3"] = {"speed"};
    objects["heater_fan fan1"] = {"speed"};
    objects["gcode_macro variables"] = {"plug_status", "winding_status"};
    objects["gcode_macro _global_var"] = {"filament_sensor_print"};
    objects["toolhead"];
    objects["print_stats"] = {"print_duration", "total_duration", "filament_used", "filename", "state", "message"};
    objects["display_status"] = {"progress", "message"};
    objects["idle_timeout"] = {"state"};
    objects["pause_resume"] = {"is_paused"};
    objects["webhooks"] = {"state", "state_message"};
    objects["firmware_retraction"] = {"retract_length", "retract_speed", "unretract_extra_length", "unretract_speed"};
    objects["bed_mesh"];
    objects["heater_fan my_nozzle_fan1"];
    objects["filament_switch_sensor filament_sensor"] = {"filament_detected", "enabled"};
    objects["output_pin fan0"] = {"value"};
    objects["output_pin fan2"] = {"value"};
    objects["output_pin caselight"];
    objects["output_pin sound"];
    objects["probe"];
    objects["configfile"];
    return objects;
}

void parse_gcode_metadata(nlohmann::json response)
{
    if (response["result"] == nlohmann::detail::value_t::null)
    {
        return;
    }
    if (response["result"]["estimated_time"] != nlohmann::detail::value_t::null)
    {
        metadata_estimated_time = (float)response["result"]["estimated_time"];
        std::cout << "预估时间: " << metadata_estimated_time << std::endl;
    }
    if (response["result"]["filename"] != nlohmann::detail::value_t::null)
    {
        metadata_filename = response["result"]["filename"];
        // MKSLOG_HIGHLIGHT("metadata_filename = %s", metadata_filename.c_str());
    }
    if (response["result"]["filament_total"] != nlohmann::detail::value_t::null)
    {
        metadata_filament_total = (int)response["result"]["filament_total"];
        MKSLOG("用料长度%f", metadata_filament_total);
    }
    if (response["result"]["object_height"] != nlohmann::detail::value_t::null)
    {
        metadata_object_height = (int)response["result"]["object_height"];
    }
    if (response["result"]["filament_name"] != nlohmann::detail::value_t::null)
    {
        metadata_filament_name = response["result"]["filament_name"];
    }
    if (response["result"]["filament_type"] != nlohmann::detail::value_t::null)
    {
        metadata_filament_type = response["result"]["filament_type"];
    }
    if (response["result"]["filament_weight_total"] != nlohmann::detail::value_t::null)
    {
        metadata_filament_weight_total = response["result"]["filament_weight_total"];
    }
    if (response["result"]["thumbnails"] != nlohmann::detail::value_t::null)
    {
        if (response["result"]["thumbnails"].back() != nlohmann::detail::value_t::null)
        {
            thumbnail_relative_path = response["result"]["thumbnails"].back()["relative_path"];
            if (getParentDirectory(metadata_filename) == "")
            {
                thumbnail_path = thumbnail_relative_path;
            }
            else
            {
                thumbnail_path = getParentDirectory(metadata_filename) + "/" + thumbnail_relative_path;
            }
        }
    }
    else
    {
        thumbnail_relative_path.clear();
        thumbnail_path.clear();
    }
}
