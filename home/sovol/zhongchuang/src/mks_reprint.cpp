#include "../include/mks_reprint.h"

MKS_REPRINT_PARAMETER_T mks_reprint_parameter;

#define TMP_FILE_PATH "/home/sovol/printer_data/temp/plr_parameter.txt.tmp"

// 获取有效字符串参数
void get_param_from_str(const char *buf, const char *str, float *data_addr)
{
    char *p;
    char *tmp_index;
    char param_buf[101];

    memset(param_buf, 0, sizeof(param_buf));
    p = param_buf;

    tmp_index = strstr((char *)buf, (char *)str);
    if (tmp_index)
    {
        MKSLOG_GREEN("buf = %s", buf);
        MKSLOG_GREEN("str = %s", str);
        tmp_index += strlen(str);
    }
    else
    {
        return;
    }
    while (((*tmp_index) != '\r') && ((*tmp_index) != '\n') && ((*tmp_index) != '#'))
    {
        if ((*tmp_index == 0x20) || (*tmp_index == 0x09)) // 跳过空格与制表符
        {
            tmp_index++;
            continue;
        }

        *p++ = *tmp_index++;
        if ((p - param_buf) > 100)
            break;
    }
    MKSLOG_GREEN("param_buf = %s", param_buf);
    *data_addr = atof(param_buf);
}

// 按行获取参数
void get_param_from_line(const char *buf)
{
    get_param_from_str(buf, "IF_REPRINT", &mks_reprint_parameter.if_reprint);
    get_param_from_str(buf, "REPRINT_ZPOS", &mks_reprint_parameter.z_pos);
    get_param_from_str(buf, "REPRINT_EPOS", &mks_reprint_parameter.e_pos);
    get_param_from_str(buf, "IF_ABSOLUTE_EEXTRUDE", &mks_reprint_parameter.if_absolute_extrude);
}

int save_reprint_parameter(const char *key, float value)
{
    FILE *file;
    char *tmp_index;
    char old_buf[1024] = {0};  // 原始数据
    char new_buf[1024] = {0};  // 拼接后的新数据
    char param_buf[100] = {0}; // 拼接那行的数据
    char value_buf[100] = {0};
    sprintf(value_buf, "%.3f", value);

    strcat(param_buf, key);
    strcat(param_buf, "\t");
    strcat(param_buf, value_buf);
    strcat(param_buf, "\r\n");
    // MKSLOG_BLUE("拼接的参数为：%s", param_buf);
    file = fopen(PLR_PARAMETER_PATH, "r");
    if (file == NULL)
    {
        MKSLOG_GREEN("打开文件失败");
        return -1;
    }
    memset(old_buf, 0, sizeof(old_buf));
    int n = fread(old_buf, 1, sizeof(old_buf), file);
    if (n <= 0)
    {
        MKSLOG_GREEN("读取错误");
        fclose(file);
        return -1;
    }
    fclose(file);
    // MKSLOG_BLUE("拼接前的文本为：\n%s", old_buf);
    // MKSLOG_BLUE("匹配 %s", key);
    tmp_index = strstr((char *)old_buf, (char *)key);
    int str_len = tmp_index - old_buf;
    while (((*tmp_index) != '\n') && ((*tmp_index) != '#'))
    {
        tmp_index++;
    }
    tmp_index++;
    strncpy(new_buf, old_buf, str_len);             // 前半段
    strncat(new_buf, param_buf, strlen(param_buf)); // 中间段
    strcat(new_buf, tmp_index);                     // 后半段
    // MKSLOG_BLUE("拼接后的文本为：%s", new_buf);


    // file = fopen(PLR_PARAMETER_PATH, "w");
    file = fopen(TMP_FILE_PATH, "w");
    if (file == NULL)
    {
        mkslog_print("打开文件 plr_parameter.txt.tmp 失败");
        return -1;
    }
    n = fwrite(new_buf, 1, strlen(new_buf), file);
    if (n <= 0)
    {
        mkslog_print("写入plr_parameter.txt.tmp 错误");
        fclose(file);
        return -1;
    }
    fflush(file);   //立即写入
    fsync(fileno(file));  // VERY IMPORTANT!
    fclose(file);
    if (rename(TMP_FILE_PATH, PLR_PARAMETER_PATH) != 0)
    {
        mkslog_print("rename 替换 plr_parameter.txt 失败");
        return -1;
    }
    return 0;
}

// 读取断电续打参数
int get_reprint_parameter()
{
    bool file_end = false;
    uint8_t i = 0, j = 0;
    char buf[1024] = {0};

    FILE *file;
    if (file = fopen(PLR_PARAMETER_PATH, "r"))
    {
        while (!file_end)
        {
            MKSLOG_GREEN("解析第%d行", j);
            memset(buf, 0, sizeof(buf));
            i = 0;
            while (1)
            {
                int n = fread(&buf[i], 1, 1, file);

                if (n > 0)
                {
                    if (buf[i] == '\n')
                    {
                        get_param_from_line((char *)buf);
                        j++;
                        break;
                    }
                    if (i > sizeof(buf))
                    {
                        MKSLOG_GREEN("越界访问数组");
                        return 1;
                    }
                    i++;
                }
                else
                {
                    MKSLOG_GREEN("读取文件结束");
                    file_end = true;
                    break;
                }
            }
        }
        fclose(file);
    }
    else
    {
        MKSLOG_GREEN("打开文件失败");
        return 1;
    }

    return 0;
}

void save_z_e_value(float new_pos_z, float pos_e)
{
    static float pos_z = 0.0;
    static std::mutex cfgMutex; // 全局互斥锁
    if (new_pos_z != pos_z)
    {
        std::lock_guard<std::mutex> lock(cfgMutex);
        pos_z = new_pos_z;
        std::cout << "Update pos_z: " << pos_z / 1.0f << std::endl;
        std::string value;
        value = std::to_string(pos_z);
        std::string cmd = "/usr/bin/python3 /home/sovol/zhongchuang/build/script.py /home/sovol/printer_data/config/saved_variables.cfg Variables power_resume_z " + value.substr(0, value.find(".") + 2);
        system(cmd.c_str());

        std::cout << "Update pos_e: " << pos_e / 1.0f << std::endl;
        save_reprint_parameter("REPRINT_ZPOS", pos_z);
        save_reprint_parameter("REPRINT_EPOS", pos_e);
        save_reprint_parameter("IF_ABSOLUTE_EEXTRUDE", mks_reprint_parameter.if_absolute_extrude);
        if(open_timelapse_flag)
        {
            run_a_gcode("TIMELAPSE_TAKE_FRAME");
        }
    }
}

