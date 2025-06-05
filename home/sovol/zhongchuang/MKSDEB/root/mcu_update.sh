#!/bin/sh

#GPIO2_C6 86  BOOT0
#GPIO2_C3 83  RESET

if [ -f /root/klipper.bin ]
then
    systemctl stop klipper.service
    mv /root/klipper.bin /home/sovol/ -f
    #enter dfu mode
    if [ -d /sys/class/gpio/gpio83 ]
    then
        echo out > /sys/class/gpio/gpio83/direction
    else
        echo 83 > /sys/class/gpio/export
        echo out > /sys/class/gpio/gpio83/direction
    fi

    if [ -d /sys/class/gpio/gpio86 ]
    then
        echo out > /sys/class/gpio/gpio86/direction
    else
        echo 86 > /sys/class/gpio/export
        echo out > /sys/class/gpio/gpio86/direction
    fi
    
    #push up boot0
    echo 0 > /sys/class/gpio/gpio86/value
    sleep 0.5
   
    #reset mcu
    echo 0 > /sys/class/gpio/gpio83/value
    sleep 0.5
    echo 1 > /sys/class/gpio/gpio83/value
   
    sleep 1

   #update
   /home/sovol/klipper/scripts/flash_usb.py -t stm32f4 -d 0483:df11 -s 0x08008000 /home/sovol/klipper.bin
   

    #return to normal mode
    echo 1 > /sys/class/gpio/gpio86/value
    sleep 0.5     
    #reset mcu
    echo 0 > /sys/class/gpio/gpio83/value
    sleep 0.5
    echo 1 > /sys/class/gpio/gpio83/value
    

   systemctl start klipper.service
else
    echo "no firmware"
fi
