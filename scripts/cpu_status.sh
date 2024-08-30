#! /bin/bash

# cpustatus

if [ "$#" -le 0 ]; then
  echo "[ERROR] Specify time interval (sec) on 1st argument: $0 <time_s>"
  exit
else
  TIME=$1 
fi

D=`date +%Y-%m-%d_%T`

FILE=OUTPUT/temp_volt_${D// /_}


minFreq=$(cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_min_freq)
minFreq=$(($minFreq/1000))

maxFreq=$(cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_max_freq)
maxFreq=$(($maxFreq/1000))

#echo "Min freq:     $minFreq MHz" >> $FILE
#echo "Max freq:     $maxFreq MHz" >> $FILE
#echo " -- "


while true; do

  CURRENT_D=`date +%Y-%m-%d_%T`
  
  temp=$(vcgencmd measure_temp)
  temp=${temp:5:4}

  volts=$(vcgencmd measure_volts)
  volts=${volts:5:5}

  freq=$(vcgencmd measure_clock arm)
  freq=${freq:14}
  freq=$(($freq/1000000))
  
  echo "$temp C / $volts V / $freq Hz  ($CURRENT_D)" >> $FILE
  sleep $TIME
done


exit 0
