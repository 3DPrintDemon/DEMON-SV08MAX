#ifndef MKS_PRINTER_H
#define MKS_PRINTER_H

#include "nlohmann/json.hpp"

#include "mks_log.h"
#include "mks_file.h"
#include "file_list.h"
#include "mks_reprint.h"

// 当前坐标
extern double x_position;
extern double y_position;
extern double z_position;
extern float e_position;

/*状态判断*/
extern std::string current_print_state;    // 打印机状态
extern std::string idle_timeout_state;     // 电机是否移动
extern std::string webhooks_state;         // web端状态
extern std::string webhooks_state_message; // web端信息

/*解析得到的参数值*/
extern int current_extruder_temperature;         // 喷头温度
extern int current_heater_bed_temperature;       // 热床温度
extern int current_extruder_target;              // 喷头目标温度
extern int current_extruder_target_back;              // 喷头目标温度
extern int current_heater_bed_target;            // 热床目标温度

extern int current_chamber_temperature;
extern int current_chamber_target;

extern std::string printer_print_stats_filename; // 正在打印的文件名
extern std::string printer_print_stats_filename_back; // 正在打印的文件名
// metadata
extern float metadata_estimated_time;            // 预估时间
extern float metadata_filament_total;            // 预计用料
extern std::string metadata_filename;
extern int metadata_object_height;
extern std::string metadata_filament_name;
extern std::string metadata_filament_type;
extern float metadata_filament_weight_total;
extern std::string thumbnail_relative_path;
extern std::string thumbnail_path;

extern float printing_times;                     // 打印时间
extern float printing_total_times;               // 打印结束总时间
extern float current_move_speed;                 // 打印速度
extern int printer_display_status_progress;      // 打印进度
extern int printer_display_status_progress_back;
extern double printer_status_progress;
extern float current_move_extrude_factor;        // 流量速度百分比
extern float current_move_speed_factor;          // 移动速度百分比
extern float current_fan_speed;                  // 风扇速度
extern float generic_fan1_speed;
extern float generic_fan2_speed;
extern float generic_fan3_speed;
extern bool has_chamber_fan;
extern bool has_chamber_temp;
// [X, Y, Z, E] - 返回应用于每个轴的“gcode 偏移”。例如，可以检查“Z”轴以确定通过“babystepping”应用了多少偏移量。
extern float printer_gcode_move_homing_origin[4];
// 最近的gcode指令坐标
extern float printer_gcode_move_gcode_position[4];

// probe
extern float printer_probe_x_zoffset;
extern float printer_probe_y_zoffset;
extern float printer_probe_z_zoffset;
// 断料检测
extern bool filament_switch_sensor_filament_detected;
extern bool filament_switch_sensor_enabled;
extern bool before_print_filament_error;
// 是否已回零
extern std::string printer_toolhead_homed_axes;
extern double printer_toolhead_position[4];
extern double printer_toolhead_axis_minimum[4];
extern double printer_toolhead_axis_maximum[4];

extern bool plug_status;  //dutou
extern bool winding_status;
extern bool filament_sensor_print;

void parse_server_history_totals(nlohmann::json totals);
void parse_printer_probe(nlohmann::json probe);
void parse_printer_beep(nlohmann::json beep);
void parse_printer_caselight(nlohmann::json caselight);
void parse_model_fan_speed(nlohmann::json model_fan_speed);
void parese_printer_out_pin_fan0(nlohmann::json out_pin_fan0);
void parese_printer_out_pin_fan2(nlohmann::json out_pin_fan2);
void parse_filament_switch_sensor_fila(nlohmann::json filament_switch_sensor);
void parse_idle_timeout(nlohmann::json idle_timeout);
void parse_bed_mesh(nlohmann::json bed_mesh);
void parse_webhooks(nlohmann::json webhooks);
void parse_gcode_move(nlohmann::json gcode_move);
void parse_toolhead(nlohmann::json toolhead);
void parse_extruder(nlohmann::json extruder);
void parse_heater_bed(nlohmann::json heater_bed);
void parse_heater_generic_hot(nlohmann::json heater_generic_hot);
void parse_fan(nlohmann::json fan, int num);
void parse_heater_fan(nlohmann::json heater_fan);
void parse_print_stats(nlohmann::json print_stats);
void parse_gcode_metadata(nlohmann::json response);
void parse_display_status(nlohmann::json display_status);
void parse_pause_resume(nlohmann::json pause_resume);
void parse_printer_info(nlohmann::json result);

nlohmann::json subscribe_objects_status();
void parse_subscribe_objects_status(nlohmann::json status);

void set_printer_state(std::string printer_state);




#endif
