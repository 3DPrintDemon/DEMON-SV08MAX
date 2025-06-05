#!/bin/bash
echo "Start makerbase-client"
time=$(date "+%Y%m%d%H%M%S")
chmod 777 /home/sovol/zhongchuang/build/io
/home/sovol/zhongchuang/build/io -4 0xff100028 0x010000
echo 79 > /sys/class/gpio/export
echo out > /sys/class/gpio/gpio79/direction
chmod 777 /sys/class/gpio/gpio79/value

# if [[ -e "/home/sovol/klipper_config/printer.cfg" && -s "/home/sovol/klipper_config/printer.cfg" ]];then
#     echo "配置文件存在且不为空"
# else
#     echo "配置文件为空"
#     cp /home/sovol/klipper_config/printer.cfg.bak /home/sovol/klipper_config/printer.cfg -f
#     sync
# fi

# if [[ -e "/home/sovol/klipper_config/Macro.cfg" && -s "/home/sovol/klipper_config/Macro.cfg" ]];then
#     echo "配置文件存在且不为空"
# else
#     echo "配置文件为空"
#     cp /home/sovol/klipper_config/Macro.cfg.bak /home/sovol/klipper_config/Macro.cfg -f
#     sync
# fi

# if [[ -e "/home/sovol/klipper_config/plr.cfg" && -s "/home/sovol/klipper_config/plr.cfg" ]];then
#     echo "配置文件存在且不为空"
# else
#     echo "配置文件为空"
#     cp /home/sovol/klipper_config/plr.cfg.bak /home/sovol/klipper_config/plr.cfg -f
#     sync
# fi

# if [[ -e "/home/sovol/klipper_config/plr.cfg" && -s "/home/sovol/klipper_config/plr.cfg" ]];then
#     echo "配置文件存在且不为空"
# else
#     echo "配置文件为空"
#     cp /home/sovol/klipper_config/plr.cfg.bak /home/sovol/klipper_config/plr.cfg -f
#     sync
# fi

# if [[ -e "/home/sovol/klipper_config/saved_variables.cfg" && -s "/home/sovol/klipper_config/saved_variables.cfg" ]];then
#     echo "配置文件存在且不为空"
# else
#     echo "配置文件为空"
#     cp /home/sovol/klipper_config/saved_variables.cfg.bak /home/sovol/klipper_config/saved_variables.cfg -f
#     sync
# fi

# if [[ -e "/home/sovol/klipper_config/moonraker.conf" && -s "/home/sovol/klipper_config/moonraker.cfg" ]];then
#     echo "配置文件存在且不为空"
# else
#     echo "配置文件为空"
#     cp /home/sovol/klipper_config/moonraker.cfg.bak /home/sovol/klipper_config/moonraker.cfg -f
#     sync
# fi

/home/sovol/zhongchuang/build/zhongchuang_klipper localhost

# >/dev/null 2>&1
