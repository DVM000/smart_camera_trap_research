#! /bin/bash

cd install_libraries
read -p  "1) Installing OpenCV. Press Enter to Continue" A
./install-opencv4.sh

read -p  "2) Installing TensorFLow. Press Enter to Continue" A
./install-TF2.sh

read -p "3) Install MQTT (y/n) ? " CONTINUE
if [[ "$CONTINUE" == "y" || "$CONTINUE" == "Y" ]]; then
	cd ../mqtt-configure
        ./mqtt-install.sh
fi




