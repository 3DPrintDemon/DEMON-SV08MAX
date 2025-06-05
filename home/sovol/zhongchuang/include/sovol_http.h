#ifndef SOVOL_HTTP_H
#define SOVOL_HTTP_H

#include <stdio.h>
#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <algorithm>
#include "nlohmann/json.hpp"

#define GUIDE_PATH "/home/sovol/printer_data/build/finishedGuide"
#define VERSION_PATH "/home/sovol/printer_data/build/.version.cfg"
#define OTA_URL "https://www.comgrow.com/files/printer/08max/ver.json"

extern int ota_progress;
extern std::string cur_remote_version;
extern std::string newVersion;
extern std::string curVersion;
void create_download_thread();
const char *get_ota_info();
std::string get_json_data(const std::string& url);
std::map<std::string, std::string> nlohmann_parse_json(const std::string& json_data);
void firmware_version_check();
void create_guide_file(const std::string& file_path);
bool file_exists(const std::string& file_path);
void deleteOneFile(const std::string& file_path);

#endif