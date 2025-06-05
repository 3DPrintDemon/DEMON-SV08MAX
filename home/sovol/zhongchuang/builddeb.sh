#! /bin/bash

cp build/src/libsrc.a MKSDEB/home/sovol/zhongchuang/build/src/libsrc.a -f

cp build/zhongchuang_klipper MKSDEB/home/sovol/zhongchuang/build/ -f

cp build/start.sh MKSDEB/home/sovol/zhongchuang/build/ -f

chmod 777 MKSDEB/home/sovol/zhongchuang/build/zhongchuang_klipper -R 

chmod 777 MKSDEB/home/sovol/zhongchuang/build/start.sh

chmod 777 MKSDEB/home/sovol/zhongchuang/build/io



# sudo rm /home/sovol/zhongchuang/MKSDEB/home/sovol/klipper -rf
rsync -av --exclude 'lib' /home/sovol/klipper/ /home/sovol/zhongchuang/MKSDEB/home/sovol/klipper/
rsync -aAXv /home/sovol/printer_data/ /home/sovol/zhongchuang/MKSDEB/home/sovol/printer_data/
rsync -aAXv /home/sovol/patch/ /home/sovol/zhongchuang/MKSDEB/home/sovol/patch/

# cp /usr/local/bin/klipper_mcu /home/sovol/zhongchuang/MKSDEB/usr/local/bin/ -f
# dpkg -b MKSDEB/ xxxx.deb
dpkg-deb -Zgzip -b MKSDEB KLP_SOC_MKS_SKIPR-08max_`date +%Y%m%d`.deb