#ifndef UI_H
#define UI_H


#include "MakerbaseSerial.h"
#include "mks_wpa_cli.h"
#include <mutex>
#include <stack>
#include "send_msg.h"
#include "mks_log.h"
#include "event.h"
#include "refresh_ui.h"
#include "mks_file.h"
#include "mks_update.h"
#include "file_list.h"
#include "wifi_list.h"
#include "timelapse_list.h"
#include "sovol_http.h"
#include "mks_error.h"

#define LOAD_FILAMENT_TEMP      250
#define UNLOAD_FILAMENT_TEMP    250

extern int filament_change_type;
extern int tty_fd;
extern int current_page_id;  // 当前页面的id号
extern int previous_page_id; // 上一页面的id号
/*打印操作页面*/
extern float printing_babystep;    // babystep值
extern float operate_zoffset_step; // 步进值
extern float operate_percent_step; // 步进值
/*移动页面步进值*/
extern float move_step;
/*调平界面zoffset步进值*/
extern float level_zoffset_step;
extern float level_zoffset_value;
/*wifi界面*/
extern bool wifi_enable;
/*sled控制*/
extern bool sled1;
extern bool open_timelapse_flag;
//是否检测断料
extern bool check_filament_flag;
//是否开启蜂鸣器
extern bool open_beep_flag;

extern float prev_current_fan_speed;
extern float prev_generic_fan1_speed;
extern float prev_generic_fan2_speed;
extern float prev_generic_fan3_speed;

extern int language;

/* 页面ID */
enum
{
    TJC_PAGE_LOGO = 0,
    TJC_PAGE_STARTING,
    TJC_PAGE_MKS_HARDWARE_TEST,
    TJC_PAGE_ERROR_RESTART,
    TJC_PAGE_ERROR_RESET_FILE,
    TJC_PAGE_ERR_TIPS,
    TJC_PAGE_GUIDE_LANGUAGE,
    TJC_PAGE_GUIDE_WIFI,
    TJC_PAGE_IP_POP,
    TJC_PAGE_IF_REPRINT,
    TJC_PAGE_MAIN,
    TJC_PAGE_FILE_LIST,
    TJC_PAGE_FILELIST_INIT,
    TJC_PAGE_NO_UDISK,
    TJC_PAGE_IF_DELETE_FILE,
    TJC_PAGE_TIMELAPSE_LIST,
    TJC_PAGE_IF_EXPORT,
    TJC_PAGE_EXPORT_SUCCEED,
    TJC_PAGE_EXPORT_FAILED,
    TJC_PAGE_IF_DELETE_TIMELAPSE_FILE,
    TJC_PAGE_PREVIEW,
    TJC_PAGE_PRINTING,
    TJC_PAGE_OPERATE,
    TJC_PAGE_IF_CHANGE_FILAMENT,
    TJC_PAGE_FILAMENT_ERROR,
    TJC_PAGE_UNLOADING,
    TJC_PAGE_UNLOAD_FINISH,
    TJC_PAGE_LOAD_BEGIN,
    TJC_PAGE_NO_FILAMENT,
    TJC_PAGE_LOADING,
    TJC_PAGE_LOAD_FINISH,
    TJC_PAGE_LOAD_CLEAR,
    TJC_PAGE_IF_STOP,
    TJC_PAGE_STOPING,
    TJC_PAGE_IF_SAVE,
    TJC_PAGE_SAVING,
    TJC_PAGE_PRINT_FILISH,
    TJC_PAGE_MOVE,
    TJC_PAGE_MOVE_ERROR,
    TJC_PAGE_HOMING,
    TJC_PAGE_FILAMENT,
    TJC_PAGE_KEYBOARD,
    TJC_PAGE_HEAT_TIP,
    TJC_PAGE_UNLOADING2,
    TJC_PAGE_LOADING2,
    TJC_PAGE_FAN,
    TJC_PAGE_HEATING,
    TJC_PAGE_LOAD_CLEAR2,
    TJC_PAGE_SYSTEM,
    TJC_PAGE_LANGUGAE,
    TJC_PAGE_IF_RESET,
    TJC_PAGE_IF_UPDATE,
    TJC_PAGE_UPDATING,
    TJC_PAGE_ABOUT,
    TJC_PAGE_WIFI_SCANING,
    TJC_PAGE_WIFI_LIST,
    TJC_PAGE_WIFI_KB,
    TJC_PAGE_WIFI_CONNECTING,
    TJC_PAGE_WIFI_SUCCEED,
    TJC_PAGE_WIFI_FAILED,
    TJC_PAGE_LEVEL_MODE,
    TJC_PAGE_Z_CALIBRAING,
    TJC_PAGE_HEATED_BED,
    TJC_PAGE_Z_INIT,
    TJC_PAGE_ZOFFSET,
    TJC_PAGE_HEATED_BED_2,
    TJC_PAGE_AUTO_LEVELING,
    TJC_PAGE_FAN_LED,
    TJC_PAGE_CALIBRATION,
    TJC_PAGE_OBICO,
    TJC_PAGE_VIBRATION,
    TJC_PAGE_PID,
    TJC_PAGE_REPRINT_TIP,
    TJC_PAGE_GUIDE_CALIBRATION,
    TJC_PAGE_SELF_CHECK,
    TJC_PAGE_FILE_COPYING,
    TJC_PAGE_FILE_COPYING_ERR,
    TJC_PAGE_FAN_LED_NOCB,
    TJC_PAGE_PRINT_FILAMENT,
    TJC_PAGE_PAUSEING,
    TJC_PAGE_OBICO_UNAVAIABLE,
    TJC_PAGE_PLUG_TIP,
    TJC_PAGE_WINDING_TIP,
    TJC_PAGE_ERROR_NETWORK,
    TJC_PAGE_PAUSE_TIP,
    TJC_PAGE_FIL_FILAMENT,
    TJC_PAGE_UNFIL_FILAMENT,
    TJC_PAGE_IF_FIL_SUCCESS,
    TJC_PAGE_SURE_FILL_IN,
    TJC_PAGE_GUIDE_SELFCHECK,
    TJC_PAGE_CHAMBER_TIP,
};

enum
{
    TJC_FAN_LED_CLOSE,
    TJC_FAN_LED_SWITCH
};

/* 左侧导航栏ID */
enum 
{
    TJC_MENU_MAIN = 100,    //主页
    TJC_MENU_TOOLS,         //工具
    TJC_MENU_FILE,          //文件
    TJC_MENU_SETTING,       //设置
};

/* 文件列表菜单栏ID */
enum
{
    TJC_MENU_LOCAL = 110,   //本地文件
    TJC_MENU_UDISK,         //U盘文件
    TJC_MENU_TIMELAPSE,     //延时摄影文件
};

/* 工具菜单ID */
enum
{
    TJC_MENU_MOVE = 120,    //移动
    TJC_MENU_FILAMENT,      //进退料
    TJC_MENU_FAN,           //风扇
};

/* 设置菜单ID */
enum
{
    TJC_MENU_SYSTEM = 130,  //系统
    TJC_MENU_WIFI,          //网络
    TJC_MENU_LEVEL,         //调平校准
};


/* 控件ID */
enum
{
    ERROR_RESTART_KLIPPER,
    ERROR_RESTART_FIRMWARE,
    ERROR_NETWORK_MANAGE,
};

enum
{
    ERROR_RESET_FILE,
};

enum
{
    ERR_TIPS_YES,
};

enum
{
    PIC_TEST_SMALL_JPG,
    PIC_TEST_BIG_JPG,
    PIC_TEST_BACK,
    PIC_TEST_DELETE,
};


enum
{
    MOVE_ERROR_YES,
    MOVE_ERROR_RESTART,
};

enum
{
    GUIDE_LANGUAGE_CN,      //中文
    GUIDE_LANGUAGE_EN,      //英文
    GUIDE_LANGUAGE_JP,      //日语
    GUIDE_LANGUAGE_DU,      //荷兰语
    GUIDE_LANGUAGE_GE,      //德语
    GUIDE_LANGUAGE_SP,      //西班牙语
    GUIDE_LANGUAGE_NEXT_STEP = 8,  //下一步
};

enum
{
    GUIDE_WIFI_SWITCH,      //WIFI开关
    GUIDE_WIFI_PREV_STEP,   //上一步
    GUIDE_WIFI_FINISH,      //结束
    GUIDE_WIFI_PREV_PAGE,   //上一页
    GUIDE_WIFI_NEXT_PAGE,   //下一页
    GUIDE_WIFI_0,
    GUIDE_WIFI_1,
    GUIDE_WIFI_2,
    GUIDE_WIFI_3,
};

enum
{
    IP_POP_OK,              //确定ip
};

enum
{
    IF_PRINT_YES,           //确定断电续打
    IF_PRINT_NO,            //取消断电续打
};

enum
{
    MAIN_EXTRUDE,           //设置喷头温度
    MAIN_HOT_BED,           //设置热床温度
    MAIN_LED,               //LED开关
    MAIN_WIFI,              //wifi信号强度
    MAIN_CHAMBER,
};

enum
{
    FILE_LIST_FILE0,
    FILE_LIST_FILE1,
    FILE_LIST_FILE2,
    FILE_LIST_FILE3,
    FILE_LIST_FILE4,
    FILE_LIST_FILE5,
    FILE_LIST_PREV,         //上一页
    FILE_LIST_NEXT,         //下一页
    FILE_LIST_BACK,         //返回
    FILE_LIST_DELETE0,
    FILE_LIST_DELETE1,
    FILE_LIST_DELETE2,
    FILE_LIST_DELETE3,
    FILE_LIST_DELETE4,
    FILE_LIST_DELETE5,
};

enum
{
    TIMELAPSE_LIST_FILE0,
    TIMELAPSE_LIST_FILE1,
    TIMELAPSE_LIST_FILE2,
    TIMELAPSE_LIST_FILE3,
    TIMELAPSE_LIST_FILE4,
    TIMELAPSE_LIST_PREV,        //上一页
    TIMELAPSE_LIST_NEXT,        //下一页
    TIMELAPSE_LIST_DELETE0,
    TIMELAPSE_LIST_DELETE1,
    TIMELAPSE_LIST_DELETE2,
    TIMELAPSE_LIST_DELETE3,
    TIMELAPSE_LIST_DELETE4,
};

enum
{
    IF_EXPORT_YES,          //确定导出文件
    IF_EXPORT_NO,           //取消导出文件
};

enum
{
    EXPORT_SUCCEED_OK,       
};

enum
{
    EXPORT_FAILED_OK,      
};

enum
{
    IF_DELETE_YES,          //确定删除文件
    IF_DELETE_NO,           //取消删除文件
};


enum
{
    PREVIEW_BACK,           //返回
    PREVIEW_START_PRINT,    //开始打印
    PREVIEW_OPEN_TIMELAPSE, //开启延时摄影
};

enum
{
    PRINTING_EXTRUDE,       //设置喷头温度
    PRINTING_HOT_BED,       //设置热床温度
    PRINTING_LED_SWITCH,    //LED开关
    PRINTING_PAUSE_RESUME,  //暂停恢复
    PRINTING_STOP,          //结束打印
    PRINTING_OPERATE,       //调整页面
    PRINTING_WIFI,          //打印点击wifi图标
    PRINTING_FILAMENT,      //change filament
    PRINTING_CHAMBER,
};

enum
{
    OPERATE_BACK,           //返回
    OPERATE_0_0_1MM,        //步进值0.01mm
    OPERATE_0_1MM,          //步进值0.1mm
    OPERATE_0_5MM,          //步进值0.5mm
    OPERATE_ZOFFSET_ADD,    //zoffset+
    OPERATE_ZOFFSET_SUB,    //zoffset-
    OPERATE_1_PERCENT,      //1%
    OPERATE_5_PERCENT,      //5%
    OPERATE_10_PERCENT,     //10%
    OPERATE_25_PERCENT,     //25%
    OPERATE_FLOW_SUB,       //流量-
    OPERATE_FLOW_ADD,       //流量+
    OPERATE_SPEED_SUB,      //速度-
    OPERATE_SPEED_ADD,      //速度+
    OPERATE_MODEL_FAN,      //模型风扇进度条
    OPERATE_MODEL_FILAMENT, //换料
};

enum
{
    PRINT_FILAMENT_SET_EXTRUDE,
    PRINT_FILAMENT_BACK,
    PRINT_FILAMENT_IN,            //进料
    PRINT_FILAMENT_OUT,           //退料
};

enum
{
    IF_STOP_YES,            //确定停止打印
    IF_STOP_NO,             //取消停止打印
};

enum
{
    IF_SAVE_YES,            //确定停止打印
    IF_SAVE_NO,             //取消停止打印   
};

enum
{
    CANNOT_CHANGE_BACK,     //确定无法跳转
};

enum
{
    PRINT_FINISH_SAVE_ZOFFSET,  //保存zoffset
    PRINT_FINISH_PRINT_ANGIN,   //再次打印
    PRINT_FINISH_DONE,          //确定打印结束
};

enum
{
    FILAMENT_ERROR_STOP_PRINT,          //结束打印
    FILAMENT_ERROR_CHANGE_FILAMENT,     //确定换料
};


enum
{
    MOVE_1MM,               //1mm
    MOVE_5MM,               //5mm
    MOVE_10MM,              //10mm
    MOVE_50MM,              //50mm
    MOVE_X_ADD,             //X+
    MOVE_X_SUB,             //X-
    MOVE_Y_ADD,             //Y+
    MOVE_Y_SUB,             //Y-
    MOVE_Z_ADD,             //Z+
    MOVE_Z_SUB,             //Z-
    MOVE_XY_HOME,           //XY回零
    MOVE_Z_HOME,            //Z回零
    MOVE_HOME_ALL,          //全部回零
    MOVE_UNLOCK,            //电机解锁
};

enum
{
    FILAMENT_SET_EXTRUDE,   //温度-
    FILAMENT_SET_HOT_BED,   //温度+
    FILAMENT_PLA,           //PLA预热
    FILAMENT_ABS,           //ABS预热
    FILAMENT_COOL,          //一键冷却
    FILAMENT_IN,            //进料
    FILAMENT_OUT,           //退料
    FILAMENT_LONG_OUT,
    FILAMENT_LONG_CLOSE,
};

enum
{
    KEYBOARD_BACK,         //数字键盘返回
};

enum
{
    FAN_LED,                //LED
    FAN_MODEL_FAN,          //模型风扇
    FAN_BEEP,               //蜂蜜器
    FAN_CHANGE_FILAMENT,    //一键换料
};

enum
{
    FAN_0,                //
    FAN_1,          //模型风扇
    FAN_2,               //蜂蜜器
    FAN_3,    //一键换料
};

enum
{
    SYSTEM_LANGUAGE,        //语言选择
    SYSTEM_RESET,           //恢复出厂
    SYSTEM_ABOUT,           //关于机器
    SYSTEM_CALIBRATION,     //校准
    SYSTEM_OBICO,           //
};

enum
{
    CALIBRATION_BACK,
    CALIBRATION_OK,
    CALIBRATION_GANTRY,
    CALIBRATION_Z,
    CALIBRATION_VIBRATION,
    CALIBRATION_PID,
};

enum
{
    OBICO_OK,
    OBICO_REFREASH,
};

enum
{
    LANGUAGE_CN,            //中文
    LANGUAGE_EN,            //英文
    LANGUAGE_JP,            //日语
    LANGUAGE_DU,            //荷兰语
    LANGUAGE_GE,            //德语
    LANGUAGE_SP,            //西班牙语
    LANGUAGE_BACK,          //返回
};

enum
{
    IF_RESET_YES,           //确实重置
    IF_RESET_NO,            //取消重置
};

enum
{
    IF_UPDATE_YES,           //确实更新
    IF_UPDATE_NO,            //取消更新
};

enum
{
    ABOUT_YES,              //版本号确定
    ABOUT_UPDATE,
};


enum
{
    WIFI_LIST_SWITCH,       //WiFi开关
    WIFI_LIST_PREV_PAGE,    //上一页
    WIFI_LIST_NEXT_PAGE,    //下一页
    WIFI_LIST_0,            //第一个wifi
    WIFI_LIST_1,            //第二个wifi
    WIFI_LIST_2,            //第三个wifi
    WIFI_LIST_3,            //第四个wifi
    WIFI_LIST_SCANE,        //扫描wifi
    WIFI_LIST_PREV,
    WIFI_LIST_NEXT,
    WIFI_SKIP,
    WIFI_BACK,
};

enum
{
    GUIDE_CALIBRATION_SKIP,
    GUIDE_CALIBRATION_BACK,
    GUIDE_CALIBRATION_CONFIRM,
};

enum
{
    WIFI_KB_CONNECT,        //连接
    WIFI_KB_CANCEL,         //取消
};

enum
{
    WIFI_SUCCEED_YES,       //WIFI连接成功确定按钮
};

enum
{
    WIFI_FAILED_YES,       //WIFI连接失败确定按钮
};

enum
{
    LEVEL_MODE_Z_TITL,           //Z轴倾斜校准
    LEVEL_MODE_ZOFFSET,            //Z轴校准
    LEVEL_MODE_AUTO_LEVELING,    //自动调平
};

enum
{
    ZOFFSET_0_01MM,           //0.01mm
    ZOFFSET_0_1MM,            //0.1mm
    ZOFFSET_1MM,              //1mm
    ZOFFSET_ADD,      //Z+
    ZOFFSET_SUB,      //Z-
    ZOFFSET_ABORT,    //取消
    ZOFFSET_SAVE,     //保存
};

enum
{
    MANUAL_LEVEL_BACK,
    MANUAL_LEVEL_POINT1,
    MANUAL_LEVEL_POINT2,
    MANUAL_LEVEL_POINT3,
    MANUAL_LEVEL_POINT4,
    MANUAL_LEVEL_UNLOCK,
    MANUAL_LEVEL_AUTO_LEVEL,
};

enum
{
    SAVE_OK_OK,             //确定保存成功
};  

enum
{
    AUTO_FINISH_SAVE,       //自动调平完成保存数据
};  

enum
{
    SHAPER_AUTO_START,      //开始自动校准
    SHAPER_0_1MM,           //0.1mm
    SHAPER_0_5MM,           //0.5mm
    SHAPER_1MM,             //1mm
    SHAPER_5MM,             //5mm
    SHAPER_X_SUB,           //X-
    SHAPER_X_ADD,           //X+
    SHAPER_Y_SUB,           //Y-
    SHAPER_Y_ADD,           //Y+
    SHAPER_X_SAVE,          //X校准
    SHAPER_Y_SAVE,          //Y校准
    SHAPER_MESH,            //校准
};


enum
{
    TEST_LED_SWITCH,
    TEST_FAN0_SWITCH,
    TEST_PREHEAT,
    TEST_COOL,
    TEST_EXIT,
};

enum
{
    YES,
    NO,
};

enum
{
    LOAD_FINISH_AGAIN,
    LOAD_FINISH_DONE,
};

enum
{
    LOAD_CLEAR_RESUME_PRINT,
};

enum
{
    PRINTING_FILAMENT_BACK,
    PRINTING_FILAMENT_IN,            //进料
    PRINTING_FILAMENT_OUT,           //退料
    PRINTING_FILAMENT_LONG_OUT,
};

void page_to_wifi_list();

void parse_cmd_msg_from_tjc_screen(char *cmd);
void page_to(int page_id);
void force_page_to(int page_id);
void tjc_event_clicked_handler(int page_id, int widget_id, int type_id);
void tjc_event_setted_handler(int page_id, int widget_id, unsigned char first, unsigned char second);
void tjc_event_wifi_keyboard(char *cmd);
void tjc_menu_handler(int widget_id);
void tjc_error_restart_handler(int widget_id);
void tjc_error_reset_file_handler(int widget_id);
void tjc_error_tips_handler(int widget_id);
void tjc_guide_language_handler(int widget_id, int type_id);
void tjc_guide_wifi_handler(int widget_id);
void tjc_main_handler(int widget_id);
void tjc_ip_pop_handler(int widget_id);
void tjc_if_reprint(int widget_id);
void tjc_file_list_handler(int widget_id);
void tjc_if_delete_file_handler(int widget_id);
void tjc_timelapse_list_handler(int widget_id);
void tjc_if_export_handler(int widget_id);
void tjc_export_succeed_handler(int widget_id);
void tjc_export_failed_handler(int widget_id);
void tjc_if_delete_timelapse_file_handler(int widget_id);
void tjc_if_export_handler(int widget_id);
void tjc_preview_handler(int widget_id);
void tjc_printing_handler(int widget_id);
void tjc_operate_handler(int widget_id);
void tjc_if_change_filament_handler(int widget_id);
void tjc_filament_error_handler(int widget_id);
void tjc_unload_finish_handler(int widget_id);
void tjc_load_begin_handler(int widget_id);
void tjc_no_filament_handler(int widget_id);
void tjc_load_finish_handler(int widget_id);
void tjc_load_clear_handler(int widget_id);
void tjc_if_stop_handler(int widget_id);
void tjc_print_finish_handler(int widget_id);
void tjc_move_handler(int widget_id);
void tjc_move_error_handler(int widget_id);
void tjc_filament_handler(int widget_id);
void tjc_heat_tip_handler(int widget_id);
void tjc_fan_hangler(int widget_id);
void tjc_load_clear2_handler(int widget_id);
void tjc_keyboard_handler(int widget_id);
void tjc_system_handler(int widget_id);
void tjc_language_handler(int widget_id);
void tjc_if_factory_handler(int widget_id);
void tjc_if_update_handler(int widget_id);
void tjc_about_hander(int widget_id);
void tjc_wifi_list_handler(int widget_id);
void tjc_wifi_kb_handler(int widget_id);
void tjc_wifi_connect_succeed(int widget_id);
void tjc_wifi_connect_failed(int widget_id);
void tjc_level_mode_handler(int widget_id);
void tjc_zoffset_handler(int widget_id);
void tjc_fan_led_handler(int widget_id);
void tjc_calibration_handler(int widget_id);
void tjc_obico_handler(int widget_id);
void tjc_reprint_tip_handler(int widget_id);
void tjc_guide_calibration_handler(int widget_id);

#endif
