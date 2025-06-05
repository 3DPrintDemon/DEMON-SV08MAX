#ifndef __MKS_UPDATE_H__
#define __MKS_UPDATE_H__


#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <iostream>
#include <sstream>
#include <fstream>
#include <vector>
#include <string>
#include <map>
#include <memory>

#include "ui.h"
#include "file_list.h"
#include "event.h"
#include "send_msg.h"
#include "MakerbaseSerial.h"


extern bool no_check_flag;


void udisk_update(void);
int if_update_file_access(void);
void *update_check(void *arg);




#endif
