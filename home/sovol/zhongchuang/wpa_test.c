#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <wpa_ctrl.h>

static struct wpa_ctrl *ctrl_conn = NULL;
static struct wpa_ctrl *mon_conn = NULL;

int main(int argc, char **argv) {
    char path[64];

    snprintf(path, sizeof(path), "/var/run/wpa_supplicant/wlan0");
    ctrl_conn = wpa_ctrl_open(path);
    if (!ctrl_conn) {
        fprintf(stderr, "Failed to open control interface: \n");
        goto cleanup;
    }

    mon_conn = wpa_ctrl_open(path);
    if (!mon_conn) {
        fprintf(stderr, "Failed to open monitor interface: \n");
        wpa_ctrl_close(ctrl_conn); // 关闭已经打开的ctrl_conn
        goto cleanup;
    }

    printf("Successful!\n");

    // 这里可以添加启动线程的代码，如果使用线程的话

cleanup:
    if (ctrl_conn) {
        wpa_ctrl_close(ctrl_conn);
    }
    if (mon_conn) {
        wpa_ctrl_close(mon_conn);
    }
    return 0; // 或返回一个非零值来表示错误，如果适用
}