#ifndef __FILE_LIST_H__
#define __FILE_LIST_H__

#include <iostream>
#include <set>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <fstream>
#include <sstream>
#include <string>
#include <map>
#include <mutex>
#include <stack>

#include "ui.h"
#include "event.h"
#include "mks_file.h"
#include "refresh_ui.h"
#include "send_msg.h"
#include "mks_log.h"
#include "mks_printer.h"


#define FILE_LIST_NUM                           6       // UI需要6个文件
#define PYTHON_PATH "/home/sovol/zhongchuang/gene5.py"

//文件列表页面
extern std::string page_files_list_name[FILE_LIST_NUM];      // 文件列表显示文件名称
extern std::string page_files_list_show_name[FILE_LIST_NUM]; // 文件列表名称
extern std::string page_files_list_show_type[FILE_LIST_NUM]; // 文件类型: [f]或者[d]，或者[n]

/*文件列表获取信息锁*/
extern std::recursive_mutex file_list_mutex;
extern bool choose_udisk_flag;                    // 是否选择U盘

extern std::string delete_name;
extern std::string delete_path;

extern bool have_64_jpg[FILE_LIST_NUM];
extern bool begin_show_160_jpg;
extern bool begin_show_176_jpg;
extern bool begin_show_finish_jpg;

extern bool get_0xfe;
extern bool get_0x06;
extern bool get_0x05;
extern bool get_0xfd;
extern bool get_0x04;

void *check_udisk_status_thread_handle(void *arg);
bool if_udisk_insert();
void file_list_init();
void update_disk_pic();
void one_page_files_scan(int pages);
void display_page_files();
void get_page_files(int pages);
void click_dir_or_file(int button);
void delete_dir_or_file(int button);
void back_to_parenet_dir();


void *sent_jpg_thread_handle(void *arg);
void png_to_64_jpg(std::string dir_path, std::string file_name, uint8_t i);
void png_to_160_jpg(std::string dir_path, std::string file_name);
void png_to_176_jpg(std::string dir_path, std::string file_name);
void delet_pic(std::string ram_path);
void delete_small_jpg();
bool sent_jpg_to_tjc(std::string ram_path, std::string pic_path);

#endif
