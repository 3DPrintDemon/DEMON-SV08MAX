#ifndef __TIMELAPSE_LIST_H__
#define __TIMELAPSE_LIST_H__

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



#define TIMELAPSE_LIST_NUM          5
#define TIMELAPSE_LIST_PATH         "/home/sovol/timelapse/"

extern std::recursive_mutex timelapse_list_mutex;
extern std::set<std::string> timelapse_list;                     // 存放延时摄影文件列表
extern std::string page_timelapse_list_name[TIMELAPSE_LIST_NUM]; // 存放当前页面延时摄影文件列表
extern std::string current_select_timelapse_file;
extern int timelapse_total_pages;   // 文件总页数
extern int timelapse_current_pages; // 当前文件页数


void timelapse_list_init();
void timelapse_list_scan(std::string current_dir);
void get_one_page_timelapse(int pages);
void display_timelapse_files();
void click_timelapse_file(int button);
void export_timelapse_file();
void click_delete_timelapse_file(int button);

#endif

