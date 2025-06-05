#include "../include/file_list.h"
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <dirent.h>
#include <unordered_set>
#include <stdbool.h>

#include <zip.h>

/*文件列表获取信息锁*/
std::recursive_mutex file_list_mutex;

bool choose_udisk_flag; // 是否选择U盘
bool have_64_jpg[FILE_LIST_NUM];
bool begin_show_64_jpg;
bool begin_show_160_jpg;
bool begin_show_176_jpg;
bool show_176_jpg_complete = true;
bool begin_show_finish_jpg;
bool show_finish_jpg_complete = true;
std::string jpg_160_path;

// 文件列表界面
std::string page_files_list_name[FILE_LIST_NUM];      // 存放当前页面的文件/文件夹名称（包含文件类型）
std::string page_files_list_show_name[FILE_LIST_NUM]; // 存放当前页面的文件/文件夹名称
std::string page_files_list_show_type[FILE_LIST_NUM]; // 存放当前页面的文件类型: [f]或者[d]，或者[n]
std::string current_select_file;
std::string delete_name;
std::string delete_path;

namespace fs = std::filesystem;

// 仅提取 .gcode 文件
bool extractGcodeOnly(const fs::path& file_path) {
    std::string basename = file_path.stem().string();
    fs::path output_path = file_path.parent_path() / (basename + ".gcode");

    if (fs::exists(output_path)) {
        std::cout << "Gcode already exists: " << output_path << std::endl;
        return true;
    }

    int err = 0;
    zip* za = zip_open(file_path.c_str(), ZIP_RDONLY, &err);
    if (!za) {
        std::cerr << "Failed to open 3mf file: " << file_path << std::endl;
        return false;
    }

    zip_int64_t num_entries = zip_get_num_entries(za, 0);
    bool found = false;

    for (zip_uint64_t i = 0; i < num_entries; ++i) {
        const char* name = zip_get_name(za, i, 0);
        if (!name) continue;

        fs::path zip_entry_path(name);
        if (zip_entry_path.extension() == ".gcode") {
            zip_file* zf = zip_fopen_index(za, i, 0);
            if (!zf) {
                std::cerr << "Failed to open gcode in zip: " << name << std::endl;
                break;
            }

            std::ofstream out(output_path, std::ios::binary);
            char buf[4096];
            zip_int64_t n;
            while ((n = zip_fread(zf, buf, sizeof(buf))) > 0) {
                out.write(buf, n);
            }

            out.close();
            zip_fclose(zf);

            auto size = fs::file_size(output_path);
            std::cout << "Extracted gcode: " << output_path << " (" << size << " bytes)" << std::endl;
            found = true;
            break;
        }
    }

    zip_close(za);

    if (!found) {
        std::cerr << "No gcode found in: " << file_path << std::endl;
        return false;
    }

    return true;
}

// 主函数：处理目录中的所有 .3mf 文件
void processDirectory(const fs::path& dir_path) {
    for (const auto& entry : fs::directory_iterator(dir_path)) {
        if (entry.is_regular_file() && entry.path().extension() == ".3mf") {
            extractGcodeOnly(entry.path());
        }
    }
}

// 刷预览图线程
void *check_udisk_status_thread_handle(void *arg)
{
    int count = 0;
    while (1)
    {
        // u盘拔出在文件列表界面拔出与插入时自动刷新界面
        if (choose_udisk_flag == true)
        {
            if ((if_udisk_insert() == false) && (current_page_id == TJC_PAGE_FILE_LIST))
            {
                MKSLOG_BLUE("U盘被拔出");
                page_to(TJC_PAGE_NO_UDISK);
            }
            else if ((if_udisk_insert() == true) && (current_page_id == TJC_PAGE_NO_UDISK))
            {
                count++;
                if (count == 2)
                {
                    MKSLOG_BLUE("U盘插入，刷新");
                    page_to(TJC_PAGE_FILELIST_INIT);
                    fs::path dir_path("/home/sovol/printer_data/gcodes/sda1");
                    processDirectory(dir_path);
                    file_list_init();
                    count = 0;
                }
            }
        }

        usleep(400000);
    }
}

/*检测U盘是否插入*/
bool if_udisk_insert()
{
    if (access(UDISK_PATH, F_OK) != 0)
    {
        return false;
    }
    else
    {
        return true;
    }
}

bool needMetascan(const std::string& gcodePath, const std::string& thumbsPath) {
    DIR* gcodeDir = opendir(gcodePath.c_str());
    if (!gcodeDir) {
        return false; // gcode 目录无法打开
    }
    
    // 读取 gcode 目录中的文件名（去除 .gcode 后缀）
    std::unordered_set<std::string> gcodeBaseNames;
    struct dirent* entry;
    while ((entry = readdir(gcodeDir)) != nullptr) {
        std::string filename = entry->d_name;
        if (filename.length() > 6 && filename.substr(filename.length() - 6) == ".gcode") {
            gcodeBaseNames.insert(filename.substr(0, filename.length() - 6));
        }
    }
    closedir(gcodeDir);
    
    if (gcodeBaseNames.empty()) {
        return false; // 没有 gcode 文件
    }
    
    DIR* thumbsDir = opendir(thumbsPath.c_str());
    if (!thumbsDir) {
        return true; // thumbs 目录无法打开，需要刷新
    }
    
    // 读取 thumbs 目录中的文件名，查找是否有匹配的前缀
    std::unordered_set<std::string> foundBaseNames;
    while ((entry = readdir(thumbsDir)) != nullptr) {
        std::string filename = entry->d_name;
        if (filename.length() > 4 && filename.substr(filename.length() - 4) == ".png") {
            for (const auto& gcodeBase : gcodeBaseNames) {
                if (filename.find(gcodeBase) == 0) { // PNG 文件名以 G-code 文件名开头
                    foundBaseNames.insert(gcodeBase);
                    break;
                }
            }
        }
    }
    closedir(thumbsDir);
    
    // 如果有任何 gcode 文件没有对应的 png，则返回 true（需要刷新）
    return foundBaseNames.size() != gcodeBaseNames.size();
}

/* 初始化U盘文件列表 */
void file_list_init()
{
    page_files_total_pages = 0;
    page_files_current_pages = 0;
    page_files_folder_layers = 0;
    page_files_path = "";
    if (choose_udisk_flag == false)
    {
        page_files_root_path = LOCAL_PATH;
    }
    else
    {
        page_files_root_path = UDISK_PATH;
    }
    get_page_files(page_files_current_pages); // 获取指定路径的第n页数据
    // has usb and gcode, no png
    if (choose_udisk_flag) {
        bool needScan = needMetascan(page_files_root_path, page_files_root_path + "/.thumbs");
        if (needScan) {
            MKSLOG_YELLOW("Metadata scan ...");
            sleep(2);
        }
    }
    display_page_files();                     // 显示文件列表内容
}

// 切换本地与U盘图标
void update_disk_pic()
{
    if (choose_udisk_flag == false)
    {
        send_cmd_picc_picc2(tty_fd, "file_list.b0", "27", "28");
        send_cmd_picc_picc2(tty_fd, "file_list.b1", "27", "28");
        send_cmd_picc_picc2(tty_fd, "filelist_init.b0", "27", "28");
        send_cmd_picc_picc2(tty_fd, "filelist_init.b1", "27", "28");
        send_cmd_picc_picc2(tty_fd, "no_udisk.b0", "27", "28");
        send_cmd_picc_picc2(tty_fd, "no_udisk.b1", "27", "28");
        send_cmd_picc_picc2(tty_fd, "preview.b0", "27", "28");
        send_cmd_picc_picc2(tty_fd, "preview.b1", "27", "28");
    }
    else
    {
        send_cmd_picc_picc2(tty_fd, "file_list.b0", "25", "26");
        send_cmd_picc_picc2(tty_fd, "file_list.b1", "25", "26");
        send_cmd_picc_picc2(tty_fd, "filelist_init.b0", "25", "26");
        send_cmd_picc_picc2(tty_fd, "filelist_init.b1", "25", "26");
        send_cmd_picc_picc2(tty_fd, "no_udisk.b0", "25", "26");
        send_cmd_picc_picc2(tty_fd, "no_udisk.b1", "25", "26");
        send_cmd_picc_picc2(tty_fd, "preview.b0", "25", "26");
        send_cmd_picc_picc2(tty_fd, "preview.b1", "25", "26");
    }

    // 显示上一页
    if (page_files_current_pages > 0)
    {
        send_cmd_picc_picc2(tty_fd, "b8", "30", "72");
    }
    else
    {
        send_cmd_picc_picc2(tty_fd, "b8", "29", "29");
    }
    // 显示下一页
    if (page_files_current_pages < page_files_total_pages)
    {
        send_cmd_picc_picc2(tty_fd, "b9", "30", "72");
    }
    else
    {
        send_cmd_picc_picc2(tty_fd, "b9", "29", "29");
    }
    // 显示返回按钮
    if (page_files_folder_layers > 0)
    {
        send_cmd_picc_picc2(tty_fd, "b10", "30", "72");
    }
    else
    {
        send_cmd_picc_picc2(tty_fd, "b10", "29", "29");
    }
}

/* 获取第n页的文件类型和文件名 */
void one_page_files_scan(int pages)
{
    int i = 0;

    for (i = 0; i < FILE_LIST_NUM; i++)
    {
        page_files_list_name[i] = "";
    }

    // 当前文件夹中的第一个文件
    auto it = page_files_dirname_filename_list.begin();

    // 将指针移到当前页
    for (i = 0; i < pages * FILE_LIST_NUM; i++)
    {
        it++;
        // std::cout << "*it" << *it << std::endl;
    }

    // 获取文件名称（包含文件类型）
    for (i = 0; i < FILE_LIST_NUM; i++)
    {
        if (it != page_files_dirname_filename_list.end())
        {
            page_files_list_name[i] = *it;
            it++;
        }
        else
        {
            break;
        }
    }

    // 获取文件类型和文件名
    for (int j = 0; j < FILE_LIST_NUM; j++)
    {
        if (page_files_list_name[j] != "")
        {
            page_files_list_show_type[j] = page_files_list_name[j].substr(0, 3);
            page_files_list_show_name[j] = page_files_list_name[j].substr(4);
        }
        else
        {
            page_files_list_show_type[j] = "[n]";
            page_files_list_show_name[j] = "";
        }
    }
}

/* 显示文件列表内容 */
void display_page_files()
{
    delete_small_jpg();
    page_to(TJC_PAGE_FILELIST_INIT);

    for (int i = 0; i < FILE_LIST_NUM; i++)
    {
        if (page_files_list_show_type[i] == "[f]")
        {
            png_to_64_jpg(page_files_root_path + page_files_path, page_files_list_show_name[i], i);
        }
    }
    begin_show_64_jpg = true; // 可以开始刷预览图了
}

// 获取文件大小（单位：字节）
std::size_t getFileSize(const std::string& path) {
    struct stat stat_buf;
    if (stat(path.c_str(), &stat_buf) == 0) {
        return stat_buf.st_size;
    }
    return 0; // 文件不存在或无法访问
}

// 返回文件大小字符串，格式 "xx.xxM"
std::string getFileSizeString(const std::string& path) {
    std::size_t size_bytes = getFileSize(path);
    if (size_bytes == 0) {
        return "无法获取文件大小或文件不存在";
    }
    
    double size_mb = size_bytes / (1024.0 * 1024.0);
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2) << size_mb << "M";
    return oss.str();
}

// 获取当前系统剩余空间（单位：MB）
std::string getFreeSpaceString() {
    struct statvfs stat;
    if (statvfs("/", &stat) != 0) {
        return "无法获取剩余空间";
    }
    
    double free_mb = (stat.f_bavail * stat.f_frsize) / (1024.0 * 1024.0);
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2) << free_mb << "M";
    return oss.str();
}

/* 文件列表点击出处理 */
void click_dir_or_file(int button)
{
    // 点击为文件夹
    if ("[d]" == page_files_list_show_type[button])
    {
        page_files_path_stack.push(page_files_path);                                 // 旧路径入栈
        page_files_path = page_files_path + "/" + page_files_list_show_name[button]; // 新的路径
        page_files_folder_layers++;                                                  // 层数加1
        page_files_current_pages = 0;                                                // 页数清0
        get_page_files(page_files_current_pages);                                    // 获取指定路径的第n页数据
        display_page_files();                                                        // 显示内容
    }
    // 点击为文件
    else if ("[f]" == page_files_list_show_type[button])
    {
        // U盘路径需要加上sda1
        if (choose_udisk_flag)
        {
            current_select_file_path = "sda1" + page_files_path + "/" + page_files_list_show_name[button];
        }
        else
        {
            if ("" == page_files_path)
            {
                current_select_file_path = page_files_list_show_name[button];
            }
            else
            {
                current_select_file_path = page_files_path + "/" + page_files_list_show_name[button];
            }
        }
        _get_gcode_metadata(current_select_file_path);
        current_select_file = page_files_list_show_name[button];

        page_to(TJC_PAGE_PREVIEW); // 跳转到显示页面
        open_timelapse_flag = false;
        send_cmd_txt(tty_fd, "t2", current_select_file);

        std::string absolutePath = (std::string) LOCAL_PATH + "/" + current_select_file_path;
        std::string sizeString = getFileSizeString(absolutePath);
        std::string freeSpace = getFreeSpaceString();
        MKSLOG_YELLOW("FilePath:%s, size:%s, space:%s", absolutePath.c_str(), sizeString.c_str(), freeSpace.c_str());
        send_cmd_txt(tty_fd, "t6", sizeString);
        send_cmd_txt(tty_fd, "t7", freeSpace);

        png_to_176_jpg(page_files_root_path + page_files_path, current_select_file);
        png_to_160_jpg(page_files_root_path + page_files_path, current_select_file);
    }
}

//点击删除
void delete_dir_or_file(int button)
{  
    delete_name = page_files_list_show_name[button];
    delete_path = page_files_root_path + page_files_path + "/" + delete_name;
    MKSLOG_YELLOW("delete_path: %s", delete_path.c_str());
    if (delete_name != "")
    {
        page_to(TJC_PAGE_IF_DELETE_FILE);
        send_cmd_txt(tty_fd, "t1", delete_name);
    }
}

// 返回上一级文件夹
void back_to_parenet_dir()
{
    page_files_path = page_files_path_stack.top(); // 获取旧路径
    page_files_path_stack.pop();                   // 出栈
    page_files_folder_layers--;                    // 层数减1
    page_files_current_pages = 0;                  // 页数清0
    get_page_files(page_files_current_pages);      // 获取指定路径的第n页数据
    display_page_files();                          // 显示内容
}

/* 获取指定路径的第n页数据 */
void get_page_files(int pages)
{
    if (file_list_mutex.try_lock())
    {
        dir_scan(page_files_root_path + page_files_path); // 获取当前路径下的文件列表信息（文件夹、文件、预览图，时间，耗材量）
        one_page_files_scan(pages);                       // 获取第n页的文件类型和文件名
        file_list_mutex.unlock();
    }
}

// 刷预览图线程
void *sent_jpg_thread_handle(void *arg)
{
    std::string png_path = "";
    std::string ram_path = "";
    std::string jpg_path = "";

    while (1)
    {
        // 刷新小预览图
        if (begin_show_64_jpg)
        {
            begin_show_64_jpg = false;
            for (uint i = 0; i < FILE_LIST_NUM; i++)
            {
                if (have_64_jpg[i] == true)
                {
                    send_cmd_tsw(tty_fd, "255", "0"); // 禁止屏幕触摸
                    ram_path = std::string("ram/") + std::string("file") + std::to_string(i) + std::string(".jpg");
                    jpg_path = page_files_root_path + std::string("/") + page_files_path + std::string("/.thumbs/") + page_files_list_show_name[i].substr(0, page_files_list_show_name[i].find(".gcode")) + std::string("-64x64.jpg");
                    if (sent_jpg_to_tjc(ram_path, jpg_path) != true)
                    {
                        sent_jpg_to_tjc(ram_path, jpg_path);
                    }
                    have_64_jpg[i] = false;
                    send_cmd_tsw(tty_fd, "255", "1"); // 使能屏幕触摸
                }
            }
            if (current_page_id == TJC_PAGE_FILELIST_INIT)
            {
                page_to(TJC_PAGE_FILE_LIST);
                update_disk_pic();
                for (int i = 0; i < FILE_LIST_NUM; i++)
                {
                    // 显示文件夹/文件名
                    std::string disp_file_name = "";
                    disp_file_name = page_files_list_show_name[i].substr(0, page_files_list_show_name[i].find(".gcode"));
                    send_cmd_txt(tty_fd, "file_" + std::to_string(i), disp_file_name);

                    // 显示删除按钮
                    if (page_files_list_show_name[i] != "")
                    {
                        send_cmd_picc_picc2(tty_fd, "b" + std::to_string(i + 11), "16", "16");
                    }
                    else
                    {
                        send_cmd_picc_picc2(tty_fd, "b" + std::to_string(i + 11), "25", "25");
                    }

                    // 为文件夹
                    if (page_files_list_show_type[i] == "[d]")
                    {
                        send_cmd_picc_picc2(tty_fd, "b" + std::to_string(i + 2), "27", "27"); // 显示文件夹
                        send_cmd_txt(tty_fd, "t" + std::to_string(i), "");                    // 清空用时
                        send_cmd_txt(tty_fd, "t" + std::to_string(i + 6), "");                // 清空耗材长度
                    }
                    // 为文件
                    else if (page_files_list_show_type[i] == "[f]")
                    {
                        send_cmd_txt(tty_fd, "t" + std::to_string(i), get_estimated_show_time(page_files_list_show_name[i]));
                        send_cmd_txt(tty_fd, "t" + std::to_string(i + 6), get_estimated_filament(page_files_list_show_name[i]));
                        // 有预览图
                        if (have_64_jpg[i] == true)
                        {
                            send_cmd_picc_picc2(tty_fd, "b" + std::to_string(i + 2), "26", "26"); // 隐藏文件图标
                        }
                        // 无预览图，显示文件图标
                        else
                        {
                            send_cmd_picc_picc2(tty_fd, "b" + std::to_string(i + 2), "28", "28"); // 显示文件图标
                        }
                    }
                    // 无文件
                    else
                    {
                        send_cmd_txt(tty_fd, "t" + std::to_string(i), "");                    // 清空用时
                        send_cmd_txt(tty_fd, "t" + std::to_string(i + 6), "");                // 清空耗材长度
                        send_cmd_txt(tty_fd, "file_" + std::to_string(i), "");                // 清空文件名
                        send_cmd_picc_picc2(tty_fd, "b" + std::to_string(i + 2), "25", "25"); // 显示无文件
                    }
                }
            }
        }

        // 刷新预览界面预览图
        if (begin_show_176_jpg)
        {
            jpg_path = page_files_root_path + std::string("/") + page_files_path + std::string("/.thumbs/") + current_select_file.substr(0, current_select_file.find(".gcode")) + std::string("-176x176.jpg");
            while (1)
            {
                if (sent_jpg_to_tjc("ram/176.jpg", jpg_path) == true)
                {
                    break;
                }
                delet_pic("ram/176.jpg");
                usleep(500000);
            }
            begin_show_176_jpg = false;
            show_176_jpg_complete = true;
        }

        // 刷新打印界面预览图
        if (begin_show_160_jpg)
        {
            while (1)
            {
                if (sent_jpg_to_tjc("ram/160.jpg", jpg_160_path) == true)
                {
                    break;
                }
                delet_pic("ram/160.jpg");
                usleep(500000);
            }
            begin_show_160_jpg = false;
        }

        // 提前删除预览界面的预览图，防止进入预览页面时闪一下
        if (current_page_id != TJC_PAGE_PREVIEW)
        {
            if (show_176_jpg_complete)
            {
                delet_pic("ram/176.jpg");
                show_176_jpg_complete = false;
            }
        }

        if (begin_show_finish_jpg)
        {
            while (1)
            {
                jpg_path = jpg_160_path.substr(0, jpg_160_path.find("-160x160.jpg")) + std::string("-64x64.jpg");
                std::string log_msg = "Finish_jpg: " + jpg_path;

                if (sent_jpg_to_tjc("ram/finish.jpg", jpg_path) == true)
                {
                    break;
                }
                delet_pic("ram/finish.jpg");
                usleep(500000);
            }
            begin_show_finish_jpg = false;
        }

        if (current_page_id != TJC_PAGE_PRINT_FILISH)
        {
            if (show_finish_jpg_complete)
            {
                delet_pic("ram/finish.jpg");
                show_finish_jpg_complete = false;
            }
        }

        usleep(50000);
    }
}

// 判断文件存在且非空
bool is_file_non_empty(const char *filepath) {
    struct stat st;
    // stat 失败说明文件不存在
    MKSLOG_BLUE("is_file_non_empty:%s, %d, %d", filepath, stat(filepath, &st), st.st_size);
    if (stat(filepath, &st) != 0) {
        return false;
    }
    // 文件大小为 0 也返回 false
    return st.st_size > 0;
}

// 生成64*64的jpg图片
void png_to_64_jpg(std::string dir_path, std::string file_name, uint8_t i)
{
    char command[256];
    std::string png_path;
    std::string jpg_path;

    png_path = dir_path + std::string("/.thumbs/") + file_name.substr(0, file_name.find(".gcode")) + std::string("-256x256.png");
    jpg_path = png_path.substr(0, png_path.find("-256x256.png")) + std::string("-64x64.jpg");

    MKSLOG_YELLOW("png_to_64_jpg png_path: %s", png_path.c_str());

    if ((access(png_path.c_str(), F_OK) == 0) && !is_file_non_empty(jpg_path.c_str()))
    {
        sprintf(command, "python3 %s %s %s 64", PYTHON_PATH, ("\"" + png_path + "\"").c_str(), ("\"" + jpg_path + "\"").c_str());
        MKSLOG_BLUE("%s", command);
        system(command);
    }

    // 判断是否有jpg图
    if (access(jpg_path.c_str(), F_OK) == 0)
    {
        have_64_jpg[i] = true;
    }
    else
    {
        have_64_jpg[i] = false;
    }
}

// 生成160*160的jpg图片
void png_to_160_jpg(std::string dir_path, std::string file_name)
{
    char command[256];
    std::string png_path;
    std::string jpg_path;

    png_path = dir_path + std::string("/.thumbs/") + file_name.substr(0, file_name.find(".gcode")) + std::string("-256x256.png");
    jpg_path = png_path.substr(0, png_path.find("-256x256.png")) + std::string("-160x160.jpg");
    MKSLOG_BLUE("jpg_path = %s", jpg_path.c_str());

    if ((access(png_path.c_str(), F_OK) == 0) && !is_file_non_empty(jpg_path.c_str()))
    {
        sprintf(command, "python3 %s %s %s 160", PYTHON_PATH, ("\"" + png_path + "\"").c_str(), ("\"" + jpg_path + "\"").c_str());
        MKSLOG_BLUE("%s", command);
        system(command);
    }

    if (access(jpg_path.c_str(), F_OK) == 0)
    {
        jpg_160_path = jpg_path;
    }
}

// 生成176*176的jpg图片
void png_to_176_jpg(std::string dir_path, std::string file_name)
{
    char command[256];
    std::string png_path;
    std::string jpg_path;

    png_path = dir_path + std::string("/.thumbs/") + file_name.substr(0, file_name.find(".gcode")) + std::string("-256x256.png");
    jpg_path = png_path.substr(0, png_path.find("-256x256.png")) + std::string("-176x176.jpg");

    if ((access(png_path.c_str(), F_OK) == 0) && !is_file_non_empty(jpg_path.c_str()))
    {
        sprintf(command, "python3 %s %s %s 176", PYTHON_PATH, ("\"" + png_path + "\"").c_str(), ("\"" + jpg_path + "\"").c_str());
        MKSLOG_BLUE("%s", command);
        system(command);
    }

    if (access(jpg_path.c_str(), F_OK) == 0)
    {
        begin_show_176_jpg = true;
    }
}

// 删除图片
void delet_pic(std::string ram_path)
{
    send_cmd_delfile(tty_fd, ram_path);
}

// 获取文件大小
long getFileSize(FILE *file)
{
    long fileSize = -1;
    if (file != NULL)
    {
        if (fseek(file, 0L, SEEK_END) == 0)
        {
            fileSize = ftell(file);
        }
        rewind(file);
    }
    return fileSize;
}

// 计算单个数据的校验码
unsigned int calccrc(unsigned char crcbuf, unsigned int crc)
{
    unsigned char i;

    crc = crc ^ crcbuf;

    for (i = 0; i < 8; i++)
    {
        unsigned char chk;

        chk = crc & 1;

        crc = crc >> 1;

        crc = crc & 0x7fff;

        if (chk == 1)

            crc = crc ^ 0xa001;

        crc = crc & 0xffff;
    }

    return crc;
}

// 计算一串数据的校验码
unsigned int check_crc(unsigned char *buf, unsigned int len)
{
    unsigned char hi, lo;

    unsigned int i;

    unsigned int crc;

    crc = 0xFFFF;

    for (i = 0; i < len; i++)
    {
        crc = calccrc(*buf, crc);

        buf++;
    }

    hi = crc % 256;

    lo = crc / 256;

    crc = (hi << 8) | lo;

    return crc;
}

// 删除所有小预览图
void delete_small_jpg()
{
    delet_pic("ram/file0.jpg");
    delet_pic("ram/file1.jpg");
    delet_pic("ram/file2.jpg");
    delet_pic("ram/file3.jpg");
    delet_pic("ram/file4.jpg");
    delet_pic("ram/file5.jpg");
}

// 发送图片
bool get_0xfe = false;
bool get_0x06 = false;
bool get_0x05 = false;
bool get_0xfd = false;
bool get_0x04 = false;
bool sent_jpg_to_tjc(std::string ram_path, std::string jpg_path)
{
    FILE *file;
    long filesize;
    int file_res = 0;
    static int start_time;
    struct timeval tv1;
    long resent_time;

    uint16_t head_id = 0;
    uint8_t head_buf[12] = {0x3A, 0xA1, 0xBB, 0x44, 0x7F, 0xFF, 0xFE, 0x01, 0x00, 0x00, 0xDC, 0x07};
    uint8_t exit_buf[12] = {0x3A, 0xA1, 0xBB, 0x44, 0x7F, 0xFF, 0xFE, 0x00, 0xFF, 0xFF, 0x00, 0x00};
    uint8_t read_buf[4096 - sizeof(head_buf)] = {0}; // 最大4096 - 帧头

    MKSLOG_BLUE("图片路径为：%s\n", jpg_path.c_str());

    file = fopen(jpg_path.c_str(), "r");
    if (file == NULL)
    {
        MKSLOG_BLUE("文件打开失败\n");
        return true;
    }

    filesize = getFileSize(file);
    // MKSLOG_BLUE("文件大小为: %ld字节\n", filesize);

    // 发送透传指令
    send_cmd_twfile(tty_fd, ram_path, std::to_string(filesize));
    // 延时，等待收到0xfe+结束符
    usleep(100000);

    // get_0xfe = false;
    // get_0x06 = false;
    // start_time = time(NULL);
    // resent_time = time(NULL);
    // while (!get_0xfe)
    // {
    //     usleep(50000);
    //     // 超时重发机制
    //     if (time_differ(1, resent_time))
    //     {
    //         resent_time = time(NULL);
    //         //重发创建文件
    //         send_cmd_delfile(tty_fd, ram_path);
    //         usleep(500000);
    //         send_cmd_twfile(tty_fd, ram_path, std::to_string(filesize));
    //         MKSLOG_GREEN("返回超时，重新删除创建文件");
    //     }
    //     // 超时机制
    //     if (time_differ(3, start_time))
    //     {
    //         MKSLOG_GREEN("创建文件超时，失败");
    //         fclose(file);
    //         write(tty_fd, exit_buf, sizeof(exit_buf));
    //         for (int i = 0; i < sizeof(exit_buf); i++)
    //         {
    //             printf("%02X", exit_buf[i]);
    //         }
    //         printf("\n");
    //         write(tty_fd, exit_buf, sizeof(exit_buf));
    //         write(tty_fd, exit_buf, sizeof(exit_buf));
    //         return;
    //     }
    // }
    // usleep(500000);
    // 循环发送包头+4096个数据，直到文件结尾
    start_time = time(NULL);
    while (1)
    {
        file_res = fread(read_buf, 1, (sizeof(read_buf) - 2), file); // 留两字节存放校验码
        // MKSLOG_BLUE("读取大小为: %ld字节\n", file_res);
        if (file_res <= 0)
        {
            break;
        }

        // 发送包头
        head_buf[8] = head_id & 0xff;
        head_buf[9] = head_id >> 8;
        head_buf[10] = (file_res + 2) & 0xff;
        head_buf[11] = (file_res + 2) >> 8;
        write(tty_fd, head_buf, sizeof(head_buf));
        // 打印头数据
        // for (int i = 0; i < sizeof(head_buf); i++)
        // {
        //     printf("%02X", head_buf[i]);
        // }
        // printf("\n");

        // 获取校验码，存放在最后两字节
        uint16_t crc_val = check_crc(read_buf, file_res);
        read_buf[file_res] = crc_val >> 8;
        read_buf[file_res + 1] = crc_val & 0xff;
        // MKSLOG_BLUE("校验码为: %02X, %02X", crc_val >> 8, crc_val & 0xff);

        // 发送数据
        write(tty_fd, read_buf, file_res + 2);

        // 打印数据
        // for (int i = 0; i < file_res+2; i++)
        // {
        //     printf("%02X", read_buf[i]);
        // }
        // printf("\n");

        // 返回0x05表示这一帧写入成功
        start_time = time(NULL);
        gettimeofday(&tv1, NULL);
        resent_time = tv1.tv_sec * 1000 + tv1.tv_usec / 1000;
        get_0x05 = false;
        get_0xfd = false;
        get_0x04 = false;
        while (!get_0x05 && !get_0xfd)
        {
            usleep(100000);
            // 返回0x04表示这一帧写入失败
            if (get_0x04)
            {
                // 打印退出透传模式
                write(tty_fd, exit_buf, sizeof(exit_buf));
                for (int i = 0; i < sizeof(exit_buf); i++)
                {
                    printf("%02X", exit_buf[i]);
                }
                printf("\n");
                write(tty_fd, exit_buf, sizeof(exit_buf));
                MKSLOG_GREEN("返回0x04，失败");
                fclose(file);
                return false;
            }
            // 超时重发机制
            gettimeofday(&tv1, NULL);
            if (time_differ_ms(600, resent_time))
            {
                gettimeofday(&tv1, NULL);
                resent_time = tv1.tv_sec * 1000 + tv1.tv_usec / 1000;
                // 重发数据帧
                write(tty_fd, head_buf, sizeof(head_buf));
                write(tty_fd, read_buf, file_res + 2);
                MKSLOG_GREEN("返回超时，重发数据包");
            }
            // 超时机制
            if (time_differ(3, start_time))
            {
                write(tty_fd, exit_buf, sizeof(exit_buf));
                for (int i = 0; i < sizeof(exit_buf); i++)
                {
                    printf("%02X", exit_buf[i]);
                }
                printf("\n");
                write(tty_fd, exit_buf, sizeof(exit_buf));
                write(tty_fd, exit_buf, sizeof(exit_buf));
                MKSLOG_GREEN("写数据帧超时，失败");
                fclose(file);
                return false;
            }
        }

        // 修改包头ID
        head_id++;
        // 超时机制
        if (time_differ(10, start_time))
        {
            write(tty_fd, exit_buf, sizeof(exit_buf));
            for (int i = 0; i < sizeof(exit_buf); i++)
            {
                printf("%02X", exit_buf[i]);
            }
            printf("\n");
            write(tty_fd, exit_buf, sizeof(exit_buf));
            write(tty_fd, exit_buf, sizeof(exit_buf));
            MKSLOG_GREEN("写图片超时，失败");
            fclose(file);
            break;
        }
    }

    MKSLOG_GREEN("成功写入图片到内存");
    fclose(file);
    return true;
}
