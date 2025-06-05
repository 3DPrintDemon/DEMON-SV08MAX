#!/bin/bash

echo "Start extra_mcu_update_fw.sh now!!!"

   # Check if extra_mcu_klipper.bin exists
if [ -f /home/sovol/printer_data/build/extra_mcu_klipper.bin ]; then

      # Check flash_can.py exited
   if [ -f /home/sovol/printer_data/build/flash_can.py ]; then
      echo "Found flash_can.py"
   else
      echo "No flash_can.py found in /home/sovol/printer_data/build.Exiting..."
      exit 1
   fi
                                                                                        
   # get into mcu bootloader
   echo "Get into bootloader ..."
   python3 ~/printer_data/build/flash_can.py -i can0 -f ~/printer_data/build/extra_mcu_klipper.bin -u 58a72bb93aa4 &
   check_pid=$!
   sleep 5
   kill $check_pid

   #get bootloader id
   bootloader_id=$(python3 ~/printer_data/build/flash_can.py -i can0 -q |  grep -oP 'Detected UUID: \K[a-f0-9]+')

   # Check if bootloader id is detected
   if [ -z "$bootloader_id" ]; then
      echo "Failed to detect bootloader id! Exiting..."
      exit 1
   fi

   # Flash MCU firmware
   python3 ~/printer_data/build/flash_can.py -i can0 -f ~/printer_data/build/extra_mcu_klipper.bin -u "$bootloader_id"

else
   echo "No extra_mcu_klipper.bin found in /home/sovol/printer_data/build. Exiting..."
fi

