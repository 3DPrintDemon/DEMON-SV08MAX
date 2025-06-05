#include "../include/MoonrakerAPI.h"
#include "../include/mks_error.h"
#include "../include/event.h"

#include <sys/sysinfo.h>

void mkslog_print(const char *msg)
{
    time_t rawtime;
    struct tm * timeinfo;
    char datetime_str[20];  // "YYYY-MM-DD HH:MM:SS" 格式的字符串
    setenv("TZ", "Asia/Shanghai", 1);
    tzset(); // 使环境变量生效
    // 获取当前系统时间
    time(&rawtime);
    // 将时间转换为当地时间
    timeinfo = localtime(&rawtime);
    strftime(datetime_str, sizeof(datetime_str), "%Y-%m-%d %H:%M:%S", timeinfo);
    char command[256];
    sprintf(command, "echo \"[%s] %s\" >> %s", datetime_str, msg, MKSLOG_PATH);
    system(command);
    MKSLOG_YELLOW("%s", msg);
}

void parse_error(nlohmann::json error) {
    std::cout << error << std::endl;
}



