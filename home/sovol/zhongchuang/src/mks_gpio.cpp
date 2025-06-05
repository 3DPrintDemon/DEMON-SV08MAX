#include "../include/mks_gpio.h"
#include "../include/mks_log.h"
#include "../include/event.h"

#include <iostream>

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <poll.h>

// static char gpio_path[64];
int saved_printing_info = 0;
extern bool sled1;
extern bool sled2;
extern bool sled3;

static int gpio_config(const char *attr, const char *val, const char *gpio_path)
{
    char file_path[64];
    int len;
    int fd;

    sprintf(file_path, "%s/%s", gpio_path, attr);
    // MKSLOG_BLUE("%s", file_path);
    if (0 > (fd = open(file_path, O_WRONLY)))
    {
        perror("open error");
        return fd;
    }

    len = strlen(val);
    if (len != write(fd, val, len))
    {
        perror("write error");
        close(fd);
        return -1;
    }

    close(fd);
    return 0;
}

// 32+2*8+5=53
int set_GPIO1_C5_low()
{
    if (access("/sys/class/gpio/gpio53", F_OK))
    {
        int fd;
        int len;
        char arg[] = "53";

        if (0 > (fd = open("/sys/class/gpio/export", O_WRONLY)))
        {
            perror("open error");
            return -1;
        }

        len = strlen(arg);
        if (len != write(fd, arg, len))
        {
            perror("write error");
            close(fd);
            return -1;
        }
        close(fd); // 关闭文件
    }

    /* 配置为输出模式 */
    if (gpio_config("direction", "out", "/sys/class/gpio/gpio53"))
    {
        printf("配置输出模式出错\n");
        return -1;
    }

    /* 极性设置 */
    if (gpio_config("active_low", "0", "/sys/class/gpio/gpio53"))
    {
        printf("配置极性设置出错\n");
        return -1;
    }

    /* 控制GPIO输出低电平 */
    if (gpio_config("value", "0", "/sys/class/gpio/gpio53"))
    {
        printf("配置输出低电平出错\n");
        return -1;
    }

    return 0;
}

int set_GPIO2_C4_high()
{ // 64+16+4=84
    if (access("/sys/class/gpio/gpio84", F_OK))
    {
        int fd;
        int len;
        char arg[] = "84";

        if (0 > (fd = open("/sys/class/gpio/export", O_WRONLY)))
        {
            perror("open error");
            return -1;
        }

        len = strlen(arg);
        if (len != write(fd, arg, len))
        {
            perror("write error");
            close(fd);
            return -1;
        }
        close(fd); // 关闭文件
    }

    /* 配置为输出模式 */
    if (gpio_config("direction", "out", "/sys/class/gpio/gpio84"))
    {
        printf("配置输出模式出错\n");
        return -1;
    }

    /* 极性设置 */
    if (gpio_config("active_low", "0", "/sys/class/gpio/gpio84"))
    {
        printf("配置极性设置出错\n");
        return -1;
    }

    /* 控制GPIO输出高电平 */
    if (gpio_config("value", "1", "/sys/class/gpio/gpio84"))
    {
        printf("配置输出低电平出错\n");
        return -1;
    }

    return 0;
}

// 32+1*8+2=42
int init_GPIO2_A3()
{ // 67=64+0+3
    if (access("/sys/class/gpio/gpio67", F_OK))
    {
        int fd;
        int len;
        char arg[] = "67";

        if (0 > (fd = open("/sys/class/gpio/export", O_WRONLY)))
        {
            perror("open error");
            return -1;
        }

        len = strlen(arg);
        if (len != write(fd, arg, len))
        {
            perror("write error");
            close(fd);
            exit(-1);
        }

        close(fd);
    }

    /* 配置为输入模式 */
    if (gpio_config("direction", "in", "/sys/class/gpio/gpio67"))
    {
        return -1;
    }

    /* 极性设置 */
    if (gpio_config("active_low", "0", "/sys/class/gpio/gpio67"))
    {
        return -1;
    }

    /* 配置为非中断方式 */
    if (gpio_config("edge", "falling", "/sys/class/gpio/gpio67"))
    {
        return -1;
    }

    return 0;
}

void *monitor_GPIO2_A3(void *arg)
{
    struct pollfd pfd;
    int ret;
    char val;
    /* 打开 value 属性文件 */
    if (0 > (pfd.fd = open("/sys/class/gpio/gpio67/value", O_RDONLY)))
    {
        perror("打开value出错");
        // return ;
    }

    /* 调用 poll */
    pfd.events = POLLPRI; // 只关心高优先级数据可读（中断）

    read(pfd.fd, &val, 1); // 先读取一次清除状态
    int power_off_count = 0;
    for (;;)
    {
        ret = poll(&pfd, 1, -1);
        if (0 > ret)
        {
            perror("poll error");
            // return ;
        }
        else if (0 == ret)
        {
            fprintf(stderr, "poll timeout.\n");
            continue;
        }

        /* 检验高优先级数据是否可读 */
        if (pfd.revents & POLLPRI)
        {
            if (0 > lseek(pfd.fd, 0, SEEK_SET))
            { // 将读位置移动到头部
                perror("lseek error");
                // return ;
            }

            if (0 > read(pfd.fd, &val, 1))
            {
                perror("read error");
                // return ;
            }
            if ((val - '0') == 1)
            {
                // 检测到低电平之后要执行的
                power_off_count++;
                std::cout << "\033[7m\033[5m"
                          << "power off:" << power_off_count << "\033[0m" << std::endl;
                if (saved_printing_info == 0)
                {
                    // save_printing_gcode();
                    saved_printing_info = 1;
                }
                system("sync");
                sleep(4);
                std::cout << "sleep" << std::endl;
                system("sync; shutdown -h now");
            }
            else
            {
                power_off_count = 0;
            }
        }
        usleep(5000); // 检测电平减少一点
    }
}

int set_GPIO1_B3_low()
{
    if (access("/sys/class/gpio/gpio43", F_OK))
    {
        int fd;
        int len;
        char arg[] = "43";

        if (0 > (fd = open("/sys/class/gpio/export", O_WRONLY)))
        {
            perror("open error");
            return -1;
        }

        len = strlen(arg);
        if (len != write(fd, arg, len))
        {
            perror("write error");
            close(fd);
            return -1;
        }
        close(fd); // 关闭文件
    }

    /* 配置为输出模式 */
    if (gpio_config("direction", "out", "/sys/class/gpio/gpio43"))
    {
        printf("配置输出模式出错\n");
        return -1;
    }

    /* 极性设置 */
    if (gpio_config("active_low", "0", "/sys/class/gpio/gpio43"))
    {
        printf("配置极性设置出错\n");
        return -1;
    }

    /* 控制GPIO输出低电平 */
    if (gpio_config("value", "0", "/sys/class/gpio/gpio43"))
    {
        printf("配置输出低电平出错\n");
        return -1;
    }

    return 0;
}

void *sled1_ctrl(void *arg)
{ // GPIO2_B7 gpio79
    struct pollfd pfd;
    int ret;
    char val;
    /* 打开 value 属性文件 */
    if (0 > (pfd.fd = open("/sys/class/gpio/gpio79/value", O_RDONLY)))
    {
        perror("打开value出错");
        std::cout << "open value failed" << std::endl;
    }

    /* 调用 poll */
    pfd.events = POLLIN;

    read(pfd.fd, &val, 1); // 先读取一次清除状态

    for (;;)
    {
        ret = poll(&pfd, 1, -1);
        if (0 > ret)
        {
            perror("poll error");
            std::cout << "poll error" << std::endl;
        }
        else if (0 == ret)
        {
            fprintf(stderr, "poll timeout.\n");
            std::cout << "无法进入检测。。。" << std::endl;
            continue;
        }
        /* 检验高优先级数据是否可读 */
        if (pfd.revents & POLLIN)
        {
            if (0 > lseek(pfd.fd, 0, SEEK_SET))
            { // 将读位置移动到头部
                perror("lseek error");
                std::cout << "lseek error" << std::endl;
            }

            if (0 > read(pfd.fd, &val, 1))
            {
                perror("read error");
                std::cout << "read error" << std::endl;
            }
            if ((val - '0') == 1)
            {
                // 检测到高电平之后要执行的
                if (sled1 == false)
                {
                    // MKSLOG_HIGHLIGHT("LED1输出低电平");
                    if (gpio_config("value", "0", "/sys/class/gpio/gpio79"))
                    {
                        printf("配置输出低电平出错\n");
                    }
                }
            }
            else
            {
                // 检测到低电平之后要执行的
                if (sled1 == true)
                {
                    // MKSLOG_HIGHLIGHT("LED1输出高电平");
                    if (gpio_config("value", "1", "/sys/class/gpio/gpio79"))
                    {
                        printf("配置输出高电平出错\n");
                    }
                }
            }
        }
        usleep(10000); // 检测电平减少一点
    }
}

void *sled2_ctrl(void *arg)
{ // GPIO2_A2 gpio66
    struct pollfd pfd;
    int ret;
    char val;
    /* 打开 value 属性文件 */
    if (0 > (pfd.fd = open("/sys/class/gpio/gpio66/value", O_RDONLY)))
    {
        perror("打开value出错");
        std::cout << "open value failed" << std::endl;
    }

    /* 调用 poll */
    pfd.events = POLLIN;

    read(pfd.fd, &val, 1); // 先读取一次清除状态

    for (;;)
    {
        ret = poll(&pfd, 1, -1);
        if (0 > ret)
        {
            perror("poll error");
            std::cout << "poll error" << std::endl;
        }
        else if (0 == ret)
        {
            fprintf(stderr, "poll timeout.\n");
            std::cout << "无法进入检测。。。" << std::endl;
            continue;
        }
        /* 检验高优先级数据是否可读 */
        if (pfd.revents & POLLIN)
        {
            if (0 > lseek(pfd.fd, 0, SEEK_SET))
            { // 将读位置移动到头部
                perror("lseek error");
                std::cout << "lseek error" << std::endl;
            }

            if (0 > read(pfd.fd, &val, 1))
            {
                perror("read error");
                std::cout << "read error" << std::endl;
            }
            if ((val - '0') == 1)
            {
                // 检测到高电平之后要执行的
                if (sled2 == false)
                {
                    if (gpio_config("value", "0", "/sys/class/gpio/gpio66"))
                    {
                        printf("配置输出低电平出错\n");
                    }
                }
            }
            else
            {
                // 检测到低电平之后要执行的
                if (sled2 == true)
                {
                    if (gpio_config("value", "1", "/sys/class/gpio/gpio66"))
                    {
                        printf("配置输出高电平出错\n");
                    }
                }
            }
        }
        usleep(10000); // 检测电平减少一点
    }
}

void *sled3_ctrl(void *arg)
{ // GPIO3_A5 gpio101
    struct pollfd pfd;
    int ret;
    char val;
    /* 打开 value 属性文件 */
    if (0 > (pfd.fd = open("/sys/class/gpio/gpio101/value", O_RDONLY)))
    {
        perror("打开value出错");
        std::cout << "open value failed" << std::endl;
    }

    /* 调用 poll */
    pfd.events = POLLIN;

    read(pfd.fd, &val, 1); // 先读取一次清除状态

    for (;;)
    {
        ret = poll(&pfd, 1, -1);
        if (0 > ret)
        {
            perror("poll error");
            std::cout << "poll error" << std::endl;
        }
        else if (0 == ret)
        {
            fprintf(stderr, "poll timeout.\n");
            std::cout << "无法进入检测。。。" << std::endl;
            continue;
        }
        /* 检验高优先级数据是否可读 */
        if (pfd.revents & POLLIN)
        {
            if (0 > lseek(pfd.fd, 0, SEEK_SET))
            { // 将读位置移动到头部
                perror("lseek error");
                std::cout << "lseek error" << std::endl;
            }

            if (0 > read(pfd.fd, &val, 1))
            {
                perror("read error");
                std::cout << "read error" << std::endl;
            }
            if ((val - '0') == 1)
            {
                // 检测到高电平之后要执行的
                if (sled3 == false)
                {
                    if (gpio_config("value", "0", "/sys/class/gpio/gpio101"))
                    {
                        printf("配置输出低电平出错\n");
                    }
                }
            }
            else
            {
                // 检测到低电平之后要执行的
                if (sled3 == true)
                {
                    if (gpio_config("value", "1", "/sys/class/gpio/gpio101"))
                    {
                        printf("配置输出高电平出错\n");
                    }
                }
            }
        }
        usleep(10000); // 检测电平减少一点
    }
}

int set_GPIO2_B7_low()
{
    if (access("/sys/class/gpio/gpio79", F_OK))
    {
        int fd;
        int len;
        char arg[] = "79";

        if (0 > (fd = open("/sys/class/gpio/export", O_WRONLY)))
        {
            perror("open error");
            return -1;
        }

        len = strlen(arg);
        if (len != write(fd, arg, len))
        {
            perror("write error");
            close(fd);
            return -1;
        }
        close(fd); // 关闭文件
    }

    /* 配置为输出模式 */
    if (gpio_config("direction", "out", "/sys/class/gpio/gpio79"))
    {
        printf("配置输出模式出错\n");
        return -1;
    }

    /* 极性设置 */
    if (gpio_config("active_low", "0", "/sys/class/gpio/gpio79"))
    {
        printf("配置极性设置出错\n");
        return -1;
    }

    /* 控制GPIO输出低电平 */
    if (gpio_config("value", "0", "/sys/class/gpio/gpio79"))
    {
        printf("配置输出低电平出错\n");
        return -1;
    }

    return 0;
}
int set_GPIO2_A2_low()
{
    if (access("/sys/class/gpio/gpio66", F_OK))
    {
        int fd;
        int len;
        char arg[] = "66";

        if (0 > (fd = open("/sys/class/gpio/export", O_WRONLY)))
        {
            perror("open error");
            return -1;
        }

        len = strlen(arg);
        if (len != write(fd, arg, len))
        {
            perror("write error");
            close(fd);
            return -1;
        }
        close(fd); // 关闭文件
    }

    /* 配置为输出模式 */
    if (gpio_config("direction", "out", "/sys/class/gpio/gpio66"))
    {
        printf("配置输出模式出错\n");
        return -1;
    }

    /* 极性设置 */
    if (gpio_config("active_low", "0", "/sys/class/gpio/gpio66"))
    {
        printf("配置极性设置出错\n");
        return -1;
    }

    /* 控制GPIO输出低电平 */
    if (gpio_config("value", "0", "/sys/class/gpio/gpio66"))
    {
        printf("配置输出低电平出错\n");
        return -1;
    }
    return 0;
}
int set_GPIO3_A5_low()
{
    if (access("/sys/class/gpio/gpio101", F_OK))
    {
        int fd;
        int len;
        char arg[] = "101";

        if (0 > (fd = open("/sys/class/gpio/export", O_WRONLY)))
        {
            perror("open error");
            return -1;
        }

        len = strlen(arg);
        if (len != write(fd, arg, len))
        {
            perror("write error");
            close(fd);
            return -1;
        }
        close(fd); // 关闭文件
    }

    /* 配置为输出模式 */
    if (gpio_config("direction", "out", "/sys/class/gpio/gpio101"))
    {
        printf("配置输出模式出错\n");
        return -1;
    }

    /* 极性设置 */
    if (gpio_config("active_low", "0", "/sys/class/gpio/gpio101"))
    {
        printf("配置极性设置出错\n");
        return -1;
    }

    /* 控制GPIO输出低电平 */
    if (gpio_config("value", "0", "/sys/class/gpio/gpio101"))
    {
        printf("配置输出低电平出错\n");
        return -1;
    }
    return 0;
}

int set_GPIO2_B7_high()
{
    if (access("/sys/class/gpio/gpio79", F_OK))
    {
        int fd;
        int len;
        char arg[] = "79";

        if (0 > (fd = open("/sys/class/gpio/export", O_WRONLY)))
        {
            perror("open error");
            return -1;
        }

        //导出gpio79
        len = strlen(arg);
        if (len != write(fd, arg, len))
        {
            perror("write error");
            close(fd);
            return -1;
        }
        close(fd); // 关闭文件
    }

    /* 配置为输出模式 */
    if (gpio_config("direction", "out", "/sys/class/gpio/gpio79"))
    {
        printf("配置输出模式出错\n");
        return -1;
    }

    /* 极性设置 */
    if (gpio_config("active_low", "0", "/sys/class/gpio/gpio79"))
    {
        printf("配置极性设置出错\n");
        return -1;
    }

    /* 控制GPIO输出高电平 */
    if (gpio_config("value", "1", "/sys/class/gpio/gpio79"))
    {
        printf("配置输出高电平出错\n");
        return -1;
    }

    return 0;
}
int set_GPIO2_A2_high()
{
    if (access("/sys/class/gpio/gpio66", F_OK))
    {
        int fd;
        int len;
        char arg[] = "66";

        if (0 > (fd = open("/sys/class/gpio/export", O_WRONLY)))
        {
            perror("open error");
            return -1;
        }

        len = strlen(arg);
        if (len != write(fd, arg, len))
        {
            perror("write error");
            close(fd);
            return -1;
        }
        close(fd); // 关闭文件
    }

    /* 配置为输出模式 */
    if (gpio_config("direction", "out", "/sys/class/gpio/gpio66"))
    {
        printf("配置输出模式出错\n");
        return -1;
    }

    /* 极性设置 */
    if (gpio_config("active_low", "0", "/sys/class/gpio/gpio66"))
    {
        printf("配置极性设置出错\n");
        return -1;
    }

    /* 控制GPIO输出高电平 */
    if (gpio_config("value", "1", "/sys/class/gpio/gpio66"))
    {
        printf("配置输出高电平出错\n");
        return -1;
    }

    return 0;
}

int set_GPIO3_A5_high()
{
    if (access("/sys/class/gpio/gpio101", F_OK))
    {
        int fd;
        int len;
        char arg[] = "101";

        if (0 > (fd = open("/sys/class/gpio/export", O_WRONLY)))
        {
            perror("open error");
            return -1;
        }

        len = strlen(arg);
        if (len != write(fd, arg, len))
        {
            perror("write error");
            close(fd);
            return -1;
        }
        close(fd); // 关闭文件
    }

    /* 配置为输出模式 */
    if (gpio_config("direction", "out", "/sys/class/gpio/gpio101"))
    {
        printf("配置输出模式出错\n");
        return -1;
    }

    /* 极性设置 */
    if (gpio_config("active_low", "0", "/sys/class/gpio/gpio101"))
    {
        printf("配置极性设置出错\n");
        return -1;
    }

    /* 控制GPIO输出低电平 */
    if (gpio_config("value", "1", "/sys/class/gpio/gpio101"))
    {
        printf("配置输出低电平出错\n");
        return -1;
    }
    return 0;
}