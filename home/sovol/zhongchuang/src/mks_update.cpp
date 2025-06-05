#include "../include/mks_update.h"

#define UPDATE_FILE_PATH "/home/sovol/printer_data/gcodes/sda1/UPDATE_DIR/zc_app.deb"
#define UPDATE_FACTORY_FILE_PATH "/home/sovol/printer_data/gcodes/sda1/UPDATE_DIR/zc_app_factory.deb"
enum
{
    ZC_APP,
    ZC_APP_FACTORY,
} app_type;

bool no_check_flag = false;

int if_update_file_access(void)
{
    int ret = 0;

    // 判断是否有生厂固件
    ret = access(UPDATE_FACTORY_FILE_PATH, F_OK);
    if (ret == 0)
    {
        app_type = ZC_APP_FACTORY;
        return 1;
    }
    // 判断是否有更新固件
    ret = access(UPDATE_FILE_PATH, F_OK);
    if (ret == 0)
    {
        app_type = ZC_APP;
        return 1;
    }

    return 0;
}

void udisk_update(void)
{
    char cmd[255];

    if (if_update_file_access() != 1)
        return;

    if (app_type == ZC_APP_FACTORY)
    {
        sprintf(cmd, "dpkg -i --force-overwrite %s; sync; sleep 5; reboot", UPDATE_FACTORY_FILE_PATH);
    }
    else
    {
        sprintf(cmd, "dpkg -i --force-overwrite %s; rm %s -f; sync; sleep 5; reboot", UPDATE_FILE_PATH, UPDATE_FILE_PATH);
    }
    printf(cmd);
    system(cmd);
}

// U盘更新线程
void *update_check(void *arg)
{
    while (1)
    {
        // if (webhooks_state == "ready")
        {
            if (!no_check_flag && if_update_file_access()) // 只检测一次
            {
                MKSLOG_HIGHLIGHT("检测到有可更新固件");
                page_to(TJC_PAGE_UPDATING);
                udisk_update();
                no_check_flag = true;
            }
        }

        if (!if_udisk_insert())
        {
            no_check_flag = false; // 拔出重新插入时检测
        }
        sleep(3);
    }
    pthread_exit(NULL);
}


