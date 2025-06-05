#ifndef __REFRESH_UI_H__
#define __REFRESH_UI_H__

#include <iostream>
#include <stack>
#include <set>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <fstream>
#include <sstream>
#include <map>
#include <mutex>
#include <stack>

#include "ui.h"
#include "event.h"
#include "file_list.h"
#include "mks_wpa_cli.h"
#include "wifi_list.h"
#include "send_msg.h"
#include "mks_log.h"
#include "mks_update.h"
#include "mks_printer.h"
#include "MakerbaseSerial.h"
#include "websocket_client.h"
#include "MakerbaseShell.h"
#include "MakerbasePanel.h"
#include "MakerbaseParseIni.h"
#include "mks_hardware_test.h"
#include "mks_gcode.h"

typedef enum {
    SELF_CHECK = 0,
    EDDY_CALIBRATION,
    BED_LEVEL,
    VIBRATION,
    PID,
    COMPLETE,
    NONE
} CHECK_STATE;

typedef enum {
    FIL_NONE = 0,
    FIL_START,
    FIL_EXTRUDER_HOT,
    FIL_POSITION,
    FIL_NEW_FILAMENT,
    FIL_COMPLETE
} FIL_STATE;

typedef enum {
    UNFIL_NONE = 0,
    UNFIL_START,
    UNFIL_EXTRUDER_HOT,
    UNFIL_RUNNING,
    UNFIL_COMPLETE
} UNFIL_STATE;

/*屏幕刷新锁*/
extern std::recursive_mutex tjc_ui_mutex;

extern std::string babystep;
extern bool get_zoffset_flag;
extern bool auto_level_complete_flag;

extern int save_time;

extern bool hasEddyCb;
extern bool hasBedLevel;
extern bool hasVibration;
extern bool hasPID;

// extern bool hasProbeEddy;
extern CHECK_STATE guide_state;
extern CHECK_STATE state;
extern bool is_errortip_show;

extern FIL_STATE filState;
extern UNFIL_STATE unfilState;
extern bool is_runing;
extern bool is_continue;
extern bool is_retry;

extern std::string current_ip_address;

void *refresh_ui_pth(void *arg);
void guide_self_check();
void check_calibration();
void refresh_page_show();
void tjc_starting_refresh();
void tjc_error_refresh();
void tjc_guide_language_refresh();
void tjc_guide_wifi_refresh();
void tjc_ip_pop_refresh();
void tjc_main_refresh();
void tjc_file_list_refresh();
void tjc_no_udisk_refresh();
void tjc_preview_refresh();
void tjc_printing_refresh();
void tjc_operate_refresh();
void tjc_move_refresh();
void tjc_homing_refresh();
void tjc_stoping_refresh();
void tjc_filament_refresh();
void tjc_loading_refresh();
void tjc_unloading_refresh();
void tjc_heating_refresh();
void tjc_unloading2_refresh();
void tjc_loading2_refresh();
void tjc_fan_refresh();
void tjc_about_refresh();
void tjc_wifi_list_refresh();
void tjc_wifi_connecting_refresh();
void tjc_level_mode_refresh();
void tjc_z_calibrating_refresh();
void tjc_head_bed_refresh();
void tjc_z_init_refresh();
void tjc_zoffset_refresh();
void tjc_head_bed_2_refresh();
void tjc_saving_refresh();
// void tjc_auto_leveling_refresh();
void tjc_fan_led_refresh();
void tjc_obico_refresh();
void tjc_ota_update();
void tjc_change_filament_refresh();
void tjc_change_unfilament_refresh();

int get_cal_printing_time(int print_time, int estimated_time, int progress);
void reset_klipper();
void reset_firmware();

#endif
