#ifndef MKS_FILE_H
#define MKS_FILE_H

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
#include <list>

#include "mks_log.h"
#include "nlohmann/json.hpp"
#include "file_list.h"

// List available files
#define GCODE_READ_SIZE                         524288
#define GIMAGE_LINE_TAG                         ";gimage:"
#define GIMAGE_END_TAG                          ";;gimage:"
#define SIMAGE_LINE_TAG                         ";simage:"
#define SIMAGE_END_TAG                          ";;simage:"
#define ESTIME_TAG                              ";TIME:"
#define ESFILA_TAG                              ";Filament used: "
#define LAYER_HEIGHT                            ";Layer height: "
#define MAX_Z                                   ";MAXZ:"
#define LOCAL_PATH                               "/home/sovol/printer_data/gcodes"
#define UDISK_PATH                               "/home/sovol/printer_data/gcodes/sda1"


extern std::set<std::string> page_files_dirname_list;          // 存放文件夹
extern std::list<std::string> page_files_filename_list;         // 存放文件
extern std::list<std::string> page_files_dirname_filename_list; // 存放文件+文件夹

extern std::map<std::string, std::string> gcode_metadata_simage_dic;
extern std::map<std::string, std::string> gcode_metadata_gimage_dic;
extern std::map<std::string, std::string> gcode_metadata_estime_dic;
extern std::map<std::string, std::string> gcode_metadata_esfila_dic;
extern std::map<std::string, std::string> gcode_metadata_layerheight_dic;
extern std::map<std::string, std::string> gcode_metadata_maxz_dic;

extern int page_files_total_pages;                // 文件总页数
extern int page_files_current_pages;              // 当前文件页数
extern int page_files_folder_layers;              // 当前文件层数


/*文件系统*/
extern std::string page_files_root_path;              // 当前根目录
extern std::string page_files_path;                   // 当前相对根目录的路径
extern std::string current_select_file;               // 当前选中的文件
extern std::string current_select_file_path;          // 当前选中的文件路径
extern std::stack<std::string> page_files_path_stack; // 路径栈


void dir_scan(std::string current_dir);
std::string getParentDirectory(const std::string& path);
bool if_need_reprint(const std::string &filename);
void delete_file(const char *file_path);
void delete_file(const char *file_path);

#endif
