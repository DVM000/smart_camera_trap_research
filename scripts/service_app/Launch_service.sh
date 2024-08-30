#! /bin/bash

# 0) Get pinout information:
iP=`grep -i 'INPUT_PIN' configuration.ini`
iP=${iP//\INPUT_PIN = /}

oP=`grep -i 'OUTPUT_PIN' configuration.ini`
oP=${oP//\OUTPUT_PIN = /}

# Set time of the system
td=`grep -i 'TIMEDATE' configuration.ini`
td=${td//\TIMEDATE = /}

if [ "$td" != "NONE" ]; then
  echo 'Setting system time to ' $td
  sudo timedatectl set-ntp false
  sudo timedatectl set-time "$td"
  sudo timedatectl set-ntp true
fi

# 1) Create dataset:
raspi-gpio set $oP op dh
[ -d DATASET/img_data_000/A ] || python scripts/Calibration_fromCamera.py --pin_PIR $iP
raspi-gpio set $oP op dl

# 2) Launch App:
export DATE_=`date +%Y-%m-%d_%T`

# a) only if not previously executed:
#ls OUTPUT/App_log_* 2>/dev/null || sudo ./apps/APP 1>OUTPUT/App_log_${DATE_// /}  

 # b) always on re-boot
sudo ./apps/APP 1>OUTPUT/App_log_${DATE_// /}  
