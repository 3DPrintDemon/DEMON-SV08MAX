#ifndef MKS_HARDWARE_TEST_H
#define MKS_HARDWARE_TEST_H

#include "ui.h"
#include "send_msg.h"
#include "mks_log.h"
#include "event.h"
#include "refresh_ui.h"
#include "mks_file.h"
#include "wifi_list.h"



extern bool test_mode;



bool if_test_mode();
void test_init();
void tjc_mks_test_handle(int widget_id);
void tjc_mks_test_refresh();

#endif
