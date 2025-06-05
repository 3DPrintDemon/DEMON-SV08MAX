#include <stdlib.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pthread.h>

#include <wpa_ctrl.h>

static struct wpa_ctrl *ctrl_conn;
static struct wpa_ctrl *mon_conn;

pthread_t mon_thread;

static void msg_cb(char *msg, size_t len);
void *wpa_cli_hdlevent_thread(void *arg);

int main(int argc, char **argv) {
    char path[64] = {"\0"};

    sprintf(path, "/var/run/wpa_supplicant/wlan0");
    ctrl_conn = wpa_ctrl_open(path);
    mon_conn = wpa_ctrl_open(path);

    if (!ctrl_conn || !mon_conn) {
        printf("open wpa control interface failed!\n");
    } else {
        printf("successful!\n");
    }

    if (wpa_ctrl_attach(mon_conn) == 0) {
        pthread_create(&mon_thread, NULL, wpa_cli_hdlevent_thread, NULL);
    }

    char replyBuff[2048] = {"\0"};
    size_t reply_len;
    int ret;
    reply_len = sizeof(replyBuff) - 1;
    ret = wpa_ctrl_request(ctrl_conn, "SET_NETWORK 0 psk \"makerbase318\"", strlen("SET_NETWORK 0 psk \"makerbase318\""), replyBuff, &reply_len, msg_cb);
    if (ret == 0) {
        replyBuff[reply_len] = '\0';
        printf("主线程输出: \n%s\n", replyBuff);
    } else if (ret == -2) {
        printf("Command time out\n");
    } else if (ret == -1) {
        printf("Command failed\n");
    }

    
    /*
    sleep(2);
    reply_len = sizeof(replyBuff) - 1;
    ret = wpa_ctrl_request(ctrl_conn, "SCAN_RESULTS", strlen("SCAN_RESULTS"), replyBuff, &reply_len, msg_cb);
    if (ret == 0) {
        replyBuff[reply_len] = '\0';
        printf("主线程输出: %s\n", replyBuff);
    }

    sleep(2);
    */

    wpa_ctrl_detach(mon_conn);
    wpa_ctrl_close(ctrl_conn);
    wpa_ctrl_close(mon_conn);

    return 0;
}

static void msg_cb(char *msg, size_t len) {
    printf("%s\n", msg);
}

void *wpa_cli_hdlevent_thread(void *arg) {
    char replyBuff[2048] = {"\0"};
    size_t reply_len;
    int ret;

    while (1)
    {
        if (wpa_ctrl_pending(mon_conn) > 0) {
            char buf[2048];
            size_t len = sizeof(buf) - 1;
            if (wpa_ctrl_recv(mon_conn, buf, &len) == 0) {
                buf[len] = '\0';
                printf("收到以下消息:\n%s\n", buf);
            }
        } else {
            usleep(100000);
        }
    }
    // pthread_exit(NULL);
}