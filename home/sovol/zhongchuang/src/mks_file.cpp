#include <iostream>
#include <stack>
#include <set>
#include <fstream>
#include <stdio.h>
#include <sys/types.h>
#include <dirent.h>
#include <sstream>
#include <string>
// #include <mutex>
#include <unistd.h>
#include <stdlib.h>
#include <sys/inotify.h>
#include <sys/stat.h>
#include <time.h>
#include <malloc.h>
#include <string.h>
#include <sys/time.h>
#include <vector>
#include <list>

#include "../include/send_msg.h"
#include "../include/mks_file.h"
#include "../include/KlippyRest.h"
#include "../include/event.h"
#include "../include/ui.h"


// 解析文件内容相关
std::set<std::string> page_files_dirname_list;          // 存放文件夹
std::list<std::string> page_files_filename_list;         // 存放文件
std::list<std::string> page_files_dirname_filename_list; // 存放文件+文件夹

int page_files_total_pages;   // 文件总页数
int page_files_current_pages; // 当前文件页数
int page_files_folder_layers; // 当前文件层数

std::map<std::string, std::string> gcode_metadata_simage_dic; // 存放小预览图数据
std::map<std::string, std::string> gcode_metadata_gimage_dic;
std::map<std::string, std::string> gcode_metadata_estime_dic;
std::map<std::string, std::string> gcode_metadata_esfila_dic;
std::map<std::string, std::string> gcode_metadata_layerheight_dic;
std::map<std::string, std::string> gcode_metadata_maxz_dic;

std::string page_files_root_path = LOCAL_PATH; // Klippy根目录，默认为本地
std::string page_files_path;                   // 文件所在路径
std::string current_select_file_path;          // 要打印的文件路径
std::stack<std::string> page_files_path_stack; // 路径栈

typedef struct sort_file_by_modifytime
{
    std::string file_name;
    int modify_time;
} sort_node;

/* 获取当前路径下的文件列表信息（文件夹、文件、预览图，时间，耗材量） */
void dir_scan(std::string current_dir)
{
    MKSLOG_GREEN("current_dir = %s", current_dir.c_str());
    page_files_dirname_list.clear();
    page_files_filename_list.clear();
    page_files_dirname_filename_list.clear();

    std::string temp_dirname = "";
    std::string temp_filename = "";

    DIR *dir;
    struct dirent *entry;
    std::vector<sort_node> sort_file_list;

    dir = opendir(current_dir.c_str());
    if (dir == NULL)
    {
        printf("Unable to open directory: %s\n", current_dir.c_str());
        return;
    }

    // Iterate over the directory entries
    while ((entry = readdir(dir)) != NULL)
    {
        std::string entryName = entry->d_name;
        std::string entryPath = current_dir + "/" + entryName;

        // 隐藏隐藏目录
        if (entryName.substr(0, 1) == ".")
        {
            continue;
        }
        // 在本地文件列表隐藏U盘节点
        if (!choose_udisk_flag && entryName == "sda1")
        {
            continue;
        }
        // 隐藏U盘自身系统目录
        if (entryName == "System Volume Information")
        {
            continue;
        }
        // 隐藏更新目录
        if (entryName == "UPDATE_DIR")
        {
            continue;
        }

        if (entry->d_type == DT_DIR)
        {
            // Recursively scan the subdirectory
            page_files_dirname_list.insert(entryName);
        }
        else if (entry->d_type == DT_REG)
        {
            if (entryName.size() >= 6 && entryName.substr(entryName.size() - 6) == ".gcode")
            {
                struct stat file_stat;
                std::string name_tmp = current_dir + "/" + entryName;
                int ret = stat(name_tmp.c_str(), &file_stat); // 获取对应文件的最后修改时间
                if (ret == 0)
                {
                    sort_node node_tmp;
                    node_tmp.file_name = entryName;
                    node_tmp.modify_time = file_stat.st_mtime;
                    sort_file_list.push_back(node_tmp);
                }

                std::ifstream file(entryPath, std::ios::binary);
                std::string gcodeData = "";
                if (file)
                {
                    file.seekg(0, std::ios::end);
                    std::streampos fileSize = file.tellg();
                    if (fileSize > GCODE_READ_SIZE)
                    {
                        fileSize = GCODE_READ_SIZE;
                    }
                    file.seekg(0, std::ios::beg);
                    gcodeData.resize(fileSize);
                    file.read(&gcodeData[0], fileSize);
                    file.close();
                }

                std::string g_tmp = "";
                std::string s_tmp = "";
                std::string time_tmp = "";
                std::string fila_tmp = "";
                std::string layer_height_tmp = "";
                std::string max_z_tmp = "";
                std::istringstream iss(gcodeData);
                std::string line;
                while (std::getline(iss, line))
                {
                    if (line.substr(0, 6) == ESTIME_TAG)
                    {
                        std::string tmp = ESTIME_TAG;
                        line.replace(0, tmp.length(), "");
                        time_tmp = line;
                    }
                    if (line.substr(0, 16) == ESFILA_TAG)
                    {
                        std::string tmp = ESFILA_TAG;
                        line.replace(0, tmp.length(), "");
                        fila_tmp = line;
                    }
                }
                // 字典虽方便，后面需要关注下同名文件
                gcode_metadata_estime_dic[entryName] = time_tmp;
                gcode_metadata_esfila_dic[entryName] = fila_tmp;
            }
        }
    }
    closedir(dir);

    // 按修改时间排序
    std::sort(sort_file_list.begin(), sort_file_list.end(), [](sort_node a, sort_node b)
              { return a.modify_time > b.modify_time; });

    for (int i = 0; i < sort_file_list.size(); i++)
    {
        MKSLOG_BLUE("文件 %s 最后修改日期为 %d ", sort_file_list[i].file_name.c_str(), sort_file_list[i].modify_time);
        page_files_filename_list.push_back(sort_file_list[i].file_name);
        if (choose_udisk_flag) {
            std::string sdaPath = (std::string) "sda1/" + sort_file_list[i].file_name;
            MKSLOG_BLUE("sdaPath: %s", sdaPath.c_str());
            _gcode_scanmeta(sdaPath);
        }
    }

    // 再存放文件夹
    for (auto it = page_files_dirname_list.begin(); it != page_files_dirname_list.end(); it++)
    {
        page_files_dirname_filename_list.push_back("[d] " + *it);
    }

    // 再存放文件
    for (auto it = page_files_filename_list.begin(); it != page_files_filename_list.end(); it++)
    {
        page_files_dirname_filename_list.push_back("[f] " + *it);
    }

    // 最后计算总页数
    if (0 == page_files_dirname_filename_list.size() % FILE_LIST_NUM)
    {
        page_files_total_pages = page_files_dirname_filename_list.size() / FILE_LIST_NUM - 1;
    }
    else
    {
        page_files_total_pages = page_files_dirname_filename_list.size() / FILE_LIST_NUM;
    }
}

// 获取文件的当前文件夹路径
std::string getParentDirectory(const std::string &path)
{
    size_t found = path.find_last_of("/\\");
    if (found != std::string::npos)
    {
        return path.substr(0, found);
    }
    else
    {
        return ""; // 如果没有找到分隔符，说明是根目录，返回空字符串或根据需求返回当前目录 "."
    }
}

// 检测文件是否包含对应字符串
bool if_need_reprint(const std::string &filename)
{
    std::ifstream file(filename);
    if (!file.is_open())
    {
        std::cerr << "无法打开文件" << std::endl;
        return false;
    }

    std::string line;
    while (std::getline(file, line))
    {
        if (line.find("#IF_REPRINT	1.000") != std::string::npos)
        {
            // MKSLOG_HIGHLIGHT("需要续打！！！");
            return true;
        }
        // MKSLOG_HIGHLIGHT("不需要续打");
    }

    return false;
}

/* 删除文件夹 */
// chris todo delete gcode, 3mf, pics
// void delete_file(const char *file_path)
// {
//     char command[256];
//     sprintf(command, "rm \"%s\" -rf", file_path);
//     MKSLOG_BLUE("%s", command);
//     system(command);
// }

namespace fs = std::filesystem;

void delete_file(const char *file_path_cstr)
{
    fs::path gcode_path(file_path_cstr);

    if (!fs::exists(gcode_path)) {
        std::cerr << "File does not exist: " << gcode_path << std::endl;
        return;
    }

    // 删除 gcode 文件
    std::cout << "[DEL] " << gcode_path << std::endl;
    fs::remove(gcode_path);

    // 获取目录和不带扩展名的基本名
    fs::path base_dir = gcode_path.parent_path();
    std::string stem = gcode_path.stem().string();

    // 删除同名 .3mf 文件
    fs::path three_mf = base_dir / (stem + ".3mf");
    if (fs::exists(three_mf)) {
        std::cout << "[DEL] " << three_mf << std::endl;
        fs::remove(three_mf);
    }

    // 删除 .thumbs 目录下以 stem- 开头的 png/jpg/jpeg 文件
    fs::path thumbs_dir = base_dir / ".thumbs";
    if (fs::exists(thumbs_dir) && fs::is_directory(thumbs_dir)) {
        for (const auto& file : fs::directory_iterator(thumbs_dir)) {
            std::string fname = file.path().filename().string();
            if (fname.rfind(stem + "-", 0) == 0) { // 以 stem- 开头
                std::string ext = file.path().extension().string();
                if (ext == ".png" || ext == ".jpg" || ext == ".jpeg") {
                    std::cout << "[DEL] " << file.path() << std::endl;
                    fs::remove(file.path());
                }
            }
        }
    }
}