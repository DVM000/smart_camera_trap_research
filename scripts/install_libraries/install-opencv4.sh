#! /bin/bash

# Install dependences
sudo apt-get update && sudo apt-get upgrade
sudo apt install -y cmake

read -p " Install dependences for OpenCV (y/n) ? " CONTINUE
if [[ "$CONTINUE" == "y" || "$CONTINUE" == "Y" ]]; then
	sudo apt-get purge wolfram-engine
	sudo apt-get purge libreoffice*
	sudo apt-get clean
	sudo apt-get autoremove

	sudo apt-get update && sudo apt-get upgrade
	sudo apt-get install -y build-essential cmake pkg-config
	sudo apt-get install -y libjpeg-dev libtiff5-dev libjasper-dev libpng12-dev
	sudo apt-get install -y libavcodec-dev libavformat-dev libswscale-dev libv4l-dev
	sudo apt-get install -y libxvidcore-dev libx264-dev
	sudo apt-get install -y libgtk2.0-dev libgtk-3-dev
	sudo apt-get install libcanberra-gtk*
	sudo apt-get install -y libatlas-base-dev gfortran
	sudo apt-get install -y python2.7-dev python3-dev
fi


# Install opencv
export OPENCV_VERSION="4.4.0"
wget https://github.com/opencv/opencv/archive/${OPENCV_VERSION}.zip && unzip ${OPENCV_VERSION}.zip
cd opencv-${OPENCV_VERSION}
mkdir build
cd build
cmake -D CMAKE_BUILD_TYPE=RELEASE -D CMAKE_INSTALL_PREFIX=/usr/local -D INSTALL_PYTHON_EXAMPLES=OFF -D BUILD_EXAMPLES=OFF -D ENABLE_NEON=ON -D ENABLE_VFPV3=ON -D WITH_GTK=ON ..

#
# -----------------------------------------------------------------------------------------------------------------------
#

# Add swap memory: 
read -p "[Info] You must increase swap size. Change the line CONF_SWAPSIZE=100 to CONF_SWAPSIZE=1024, save and exit. Press Enter to continue." CONTINUE
sudo nano /etc/dphys-swapfile
sudo service dphys-swapfile restart

FREE_MEM="$(free -m | awk '/^Mem/ {print $2}')"
# Use "-j 4" only memory is larger than 2GB
if [[ "FREE_MEM" -gt "2000" ]]; then
  NO_JOB=4
else
  NO_JOB=1
fi
cd opencv-${OPENCV_VERSION}/build
make -j ${NO_JOB}
  # in case of stopping, use: make -j2
sudo make install
sudo ldconfig

# Important for OpenCV-4.x.x
cd ../../
sudo cp opencv_github.pc /usr/local/lib/opencv.pc
export PKG_CONFIG_PATH=:/usr/local/lib
read -p "[Info] You edit .bashrc file. Add this line 'export PKG_CONFIG_PATH=$PKG_CONFIG_PATH:/usr/local/lib/', save and exit. Press Enter to continue." CONTINUE
sudo nano ~/.bashrc
# (add line: export PKG_CONFIG_PATH=$PKG_CONFIG_PATH:/usr/local/lib/)

pkg-config --cflags --libs opencv # check libraries

# remove zip
rm ${OPENCV_VERSION}.zip

read -p "[Info] You can restore swap size. Change the line CONF_SWAPSIZE=1024 to CONF_SWAPSIZE=100, save and exit. Press Enter to continue." CONTINUE
sudo nano /etc/dphys-swapfile
sudo service dphys-swapfile restart
