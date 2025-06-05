#include "../include/timelapse_list.h"

std::recursive_mutex timelapse_list_mutex;

std::set<std::string> timelapse_list;                     // 存放延时摄影文件列表
std::string page_timelapse_list_name[TIMELAPSE_LIST_NUM]; // 存放当前页面延时摄影文件列表
std::string current_select_timelapse_file;

int timelapse_total_pages;   // 文件总页数
int timelapse_current_pages; // 当前文件页数

// 初始化列表
void timelapse_list_init()
{
    timelapse_total_pages = 0;
    timelapse_current_pages = 0; 
    get_one_page_timelapse(timelapse_current_pages); // 获取第n页的文件类型和文件名
    display_timelapse_files();                       // 显示文件列表内容
}

// 扫描列表
void timelapse_list_scan(std::string current_dir)
{
    DIR *dir;
    struct dirent *entry;

    dir = opendir(current_dir.c_str());
    if (dir == NULL)
    {
        printf("Unable to open directory: %s\n", current_dir.c_str());
        return;
    }

    timelapse_list.clear();
    while ((entry = readdir(dir)) != NULL)
    {
        std::string entryName = entry->d_name;

        // 隐藏隐藏目录
        if (entryName.substr(0, 1) == ".")
        {
            continue;
        }

        if (entry->d_type == DT_REG)
        {
            timelapse_list.insert(entryName);
        }
    }
    closedir(dir);

    // 最后计算总页数
    if ((timelapse_list.size() % TIMELAPSE_LIST_NUM) == 0)
    {
        timelapse_total_pages = timelapse_list.size() / TIMELAPSE_LIST_NUM - 1;
    }
    else
    {
        timelapse_total_pages = timelapse_list.size() / TIMELAPSE_LIST_NUM;
    }
}

/* 获取第n页的文件类型和文件名 */
void get_one_page_timelapse(int pages)
{
    if (timelapse_list_mutex.try_lock())
    {
        timelapse_list_scan(TIMELAPSE_LIST_PATH); // 扫描延时摄影文件
        timelapse_list_mutex.unlock();
    }
    int i = 0;
    for (i = 0; i < TIMELAPSE_LIST_NUM; i++)
    {
        page_timelapse_list_name[i] = "";
    }

    // 当前文件夹中的第一个文件
    auto it = timelapse_list.begin();

    // 将指针移到当前页
    for (i = 0; i < pages * TIMELAPSE_LIST_NUM; i++)
    {
        it++;
        std::cout << "*it" << *it << std::endl;
    }

    // 获取文件名称（包含文件类型）
    for (i = 0; i < TIMELAPSE_LIST_NUM; i++)
    {
        if (it != timelapse_list.end())
        {
            page_timelapse_list_name[i] = *it;
            it++;
        }
        else
        {
            break;
        }
    }
}

/* 显示文件列表内容 */
void display_timelapse_files()
{
    page_to(TJC_PAGE_TIMELAPSE_LIST);

    for (int i = 0; i < TIMELAPSE_LIST_NUM; i++)
    {
        // 显示文件名
        if (page_timelapse_list_name[i] != "")
        {
            send_cmd_txt(tty_fd, "file_" + std::to_string(i), page_timelapse_list_name[i]);
            send_cmd_picc_picc2(tty_fd, "b" + std::to_string(i + 2), "80", "81");  // 显示有文件
            send_cmd_picc_picc2(tty_fd, "b" + std::to_string(i + 11), "80", "81"); // 显示有文件
        }
        else
        {
            send_cmd_txt(tty_fd, "file_" + std::to_string(i), "");                 // 清空文件名
            send_cmd_picc_picc2(tty_fd, "b" + std::to_string(i + 2), "82", "82");  // 显示无文件
            send_cmd_picc_picc2(tty_fd, "b" + std::to_string(i + 11), "82", "82"); // 显示无文件
        }
    }

    // 显示上一页
    if (timelapse_current_pages > 0)
    {
        send_cmd_picc_picc2(tty_fd, "b7", "30", "72");
    }
    else
    {
        send_cmd_picc_picc2(tty_fd, "b7", "29", "29");
    }
    // 显示下一页
    if (timelapse_current_pages < timelapse_total_pages)
    {
        send_cmd_picc_picc2(tty_fd, "b8", "30", "72");
    }
    else
    {
        send_cmd_picc_picc2(tty_fd, "b8", "29", "29");
    }
}

// 点击文件
void click_timelapse_file(int button)
{
    if (page_timelapse_list_name[button] != "")
    {
        current_select_timelapse_file = page_timelapse_list_name[button];
        page_to(TJC_PAGE_IF_EXPORT); // 跳转到显示页面
        send_cmd_txt(tty_fd, "t1", current_select_timelapse_file);
    }
}

/* 导出文件 */
void export_timelapse_file()
{
    char command[256];
    sprintf(command, "mv %s\"%s\" %s\"%s\"; sync", TIMELAPSE_LIST_PATH, current_select_timelapse_file.c_str(), "/home/sovol/printer_data/gcodes/sda1/", current_select_timelapse_file.c_str());
    MKSLOG_BLUE("%s", command);
    system(command);
}

// 点击删除文件
void click_delete_timelapse_file(int button)
{
    if (page_timelapse_list_name[button] != "")
    {
        delete_path = TIMELAPSE_LIST_PATH + page_timelapse_list_name[button];
        page_to(TJC_PAGE_IF_DELETE_TIMELAPSE_FILE); // 跳转到显示页面
        send_cmd_txt(tty_fd, "t1", page_timelapse_list_name[button]);
    }
}
