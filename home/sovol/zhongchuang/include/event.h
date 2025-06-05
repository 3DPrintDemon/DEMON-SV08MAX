#ifndef EVENT_H
#define EVENT_H

#include <iostream>
#include <set>
#include <stack>
#include <regex>
#include <mutex>
#include <fstream>
#include <sstream>
#include <map>
#include <termios.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#include "KlippyGcodes.h"
#include "send_msg.h"
#include "ui.h"
#include "wifi_list.h"
#include "mks_printer.h"
#include "mks_file.h"
#include "file_list.h"
#include "wifi_list.h"
#include "refresh_ui.h"
#include "mks_update.h"
#include "MakerbaseSerial.h"
#include "websocket_client.h"
#include "MakerbaseShell.h"
#include "MakerbasePanel.h"
#include "MakerbaseParseIni.h"
#include "mks_wpa_cli.h"
#include "MoonrakerAPI.h"
#include "mks_reprint.h"


extern WebSocketClient *ep;

int time_differ(int duration, int start_time);
int time_differ_ms(long duration, long start_time);
bool time_differ_ms2(double duration_ms, struct timespec &start_time);
void run_a_gcode(std::string gcode);
void printer_state_show();
void auto_clean_big_log();
void auto_get_object();
void show_error_message();
void sub_object_status();
void get_object_status();
void start_printing(std::string filepath);
void set_extruder_target(int target);
void set_heater_bed_target(int target);
void set_chamber_target(int target);
std::string set_fan_speed(std::string num, int speed);
void set_zoffset(bool positive, float value);
void set_printer_speed(int speed);
void set_printer_flow(int rate);
std::string show_time(int seconds);
void led1_on_off();
void move_home();
void move_home_x();
void move_home_y(); 
void move_home_z();
void move_x_decrease(float value);
void move_x_increase(float value);
void move_y_decrease(float value);
void move_y_increase(float value);
void move_z_decrease(float value);
void move_z_increase(float value);
void motors_off();
void emergency_stop();
void set_filament_sensor(bool status);
int get_estimated_time(std::string estime_key);
std::string get_estimated_show_time(std::string estime_key);
std::string get_estimated_filament(std::string esfile_key);
void _gcode_scanmeta(std::string filepath);
void _get_gcode_metadata(std::string filepath);

void print_finish_check();
void cancel_print();
void start_plr();
void cancel_plr();


#endif
