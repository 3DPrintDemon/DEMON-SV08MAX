#include "../include/mks_hardware_test.h"

bool test_mode = false;
int current_extruder_temperature_back;
int current_heater_bed_temperature_back;

bool if_test_mode()
{
    if (access("/home/sovol/printer_data/gcodes/sda1/MKS_TEST", F_OK) == 0)
    {
        test_mode = true;
        return true;
    }
    else
    {
        test_mode = false;
        return false;
    }
}

void test_init()
{
    sled1 = true;
    current_extruder_temperature_back = current_extruder_temperature;
    current_heater_bed_temperature_back = current_heater_bed_temperature;
    set_extruder_target(160);
    set_heater_bed_target(60);
    // run_a_gcode(set_fan_speed(100));
    run_a_gcode("G28");
    run_a_gcode("G92 E0");
}

void test_exit()
{
    set_extruder_target(0);
    set_heater_bed_target(0);
    // run_a_gcode(set_fan_speed(0));
    run_a_gcode("G28");
    run_a_gcode("G92 E0");
}

// 测试界面按钮
void tjc_mks_test_handle(int widget_id)
{
    switch (widget_id)
    {
    case TEST_LED_SWITCH:
        led1_on_off();
        break;

    case TEST_FAN0_SWITCH:
        if ((int)(current_fan_speed * 100 + 0.5) == 100)
        {
            // run_a_gcode(set_fan_speed(0));
        }
        else
        {
            // run_a_gcode(set_fan_speed(100));
        }
        break;

    case TEST_PREHEAT:
        current_extruder_temperature_back = current_extruder_temperature;
        current_heater_bed_temperature_back = current_heater_bed_temperature;
        set_extruder_target(160);
        set_heater_bed_target(60);
        break;

    case TEST_COOL:
        set_extruder_target(0);
        set_heater_bed_target(0);
        break;

    case TEST_EXIT:
        test_mode = false;
        test_exit();
        page_to(TJC_PAGE_MAIN);
        break;
    }
}

// 刷新测试界面
void tjc_mks_test_refresh()
{
    static char eth0_ip_address[INET_ADDRSTRLEN]; // eth0_IP地址
    static char eth0_mac_address[18];             // eth0_MAC地址

    static char wlan0_ip_address[INET_ADDRSTRLEN]; // wlan0_IP地址
    static char wlan0_mac_address[18];             // wlan0_MAC地址

    int start_time = time(NULL);

    // 版本号
    send_cmd_txt(tty_fd, "test_vison", "V1.0");

    // 温度曲线
    send_cmd_txt(tty_fd, "extruder_temp", std::to_string(current_extruder_temperature) + "℃/");
    send_cmd_txt(tty_fd, "hot_bed_temp", std::to_string(current_heater_bed_temperature) + "℃/");
    send_cmd_txt(tty_fd, "extru_target", std::to_string(current_extruder_target) + "℃");
    send_cmd_txt(tty_fd, "bed_target", std::to_string(current_heater_bed_target) + "℃");

    if (current_extruder_temperature > 300)
    {
        // 大于300度，显示红色
        send_cmd_pco(tty_fd, "extruder_temp", "63488");
        send_cmd_pco(tty_fd, "extru_target", "63488");
    }
    else if ((current_extruder_temperature < current_extruder_target) && ((current_extruder_temperature - current_extruder_temperature_back) > 3))
    {
        // 加热中，显示黄色
        send_cmd_pco(tty_fd, "extruder_temp", "65504");
        send_cmd_pco(tty_fd, "extru_target", "65504");
    }
    else
    {
        // 不加热，显示绿色
        send_cmd_pco(tty_fd, "extruder_temp", "2024");
        send_cmd_pco(tty_fd, "extru_target", "2024");
    }

    if (current_heater_bed_temperature > 100)
    {
        ////大于100度，显示红色
        send_cmd_pco(tty_fd, "hot_bed_temp", "63488");
        send_cmd_pco(tty_fd, "bed_target", "63488");
    }
    else if ((current_heater_bed_temperature < current_heater_bed_target) && ((current_heater_bed_temperature - current_heater_bed_temperature_back) > 3))
    {
        // 加热中，显示黄色
        send_cmd_pco(tty_fd, "hot_bed_temp", "65504");
        send_cmd_pco(tty_fd, "bed_target", "65504");
    }
    else
    {
        // 不加热，显示绿色
        send_cmd_pco(tty_fd, "hot_bed_temp", "2024");
        send_cmd_pco(tty_fd, "bed_target", "2024");
    }

    // 显示eth0_ip地址
    if (get_interface_info("eth0", eth0_ip_address, eth0_mac_address)) // 获取以太网接口信息
    {
        send_cmd_txt(tty_fd, "eth0_ip", std::string(eth0_ip_address));
        send_cmd_pco(tty_fd, "eth0_ip", "2024");
    }
    else
    {
        send_cmd_txt(tty_fd, "eth0_ip", "-----------");
        send_cmd_pco(tty_fd, "eth0_ip", "63488");
    }

    // 显示wlan0_ip地址
    if (get_interface_info("wlan0", wlan0_ip_address, wlan0_mac_address)) // 获取wifi-wlan0接口信息
    {
        send_cmd_txt(tty_fd, "wlan0_ip", std::string(wlan0_ip_address));
        send_cmd_pco(tty_fd, "wlan0_ip", "2024");
    }
    else
    {
        send_cmd_txt(tty_fd, "wlan0_ip", "-----------");
        send_cmd_pco(tty_fd, "wlan0_ip", "63488");
    }

    // 检测引脚状态
    if (filament_switch_sensor_filament_detected)
    {
        send_cmd_txt(tty_fd, "check_status", "已检测到");
        send_cmd_pco(tty_fd, "check_status", "2024");
    }
    else
    {
        send_cmd_txt(tty_fd, "check_status", "未检测到");
        send_cmd_pco(tty_fd, "check_status", "63488");
    }

    // led状态
    if (sled1)
    {
        send_cmd_txt(tty_fd, "b_led", "已开启");
    }
    else
    {
        send_cmd_txt(tty_fd, "b_led", "已关闭");
    }

    // 风扇0
    if ((int)(current_fan_speed * 100 + 0.5) == 100)
    {
        send_cmd_txt(tty_fd, "b_fan0", "已开启");
    }
    else
    {
        send_cmd_txt(tty_fd, "b_fan0", "已关闭");
    }

    // 电机转动
    if (time_differ(5, start_time))
    {
        start_time = time(NULL);
        if (current_extruder_temperature >= 155)
        {
            run_a_gcode("G1 X20 Y20 Z10 E10 F1000");
            run_a_gcode("G1 X0 Y0 Z0 E0 F1000");
        }
        else
        {
            run_a_gcode("G1 X20 Y20 Z10 F1000");
            run_a_gcode("G1 X0 Y0 Z0 F1000");
        }
    }
}
