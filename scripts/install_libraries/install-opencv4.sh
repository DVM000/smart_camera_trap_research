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
	sudo apt-get install build-essential cmake git unzip pkg-config
        sudo apt-get install libjpeg-dev libpng-dev
        sudo apt-get install libavcodec-dev libavformat-dev libswscale-dev
        sudo apt-get install libgtk2.0-dev libcanberra-gtk* libgtk-3-dev
        sudo apt-get install libgstreamer1.0-dev gstreamer1.0-gtk3
        sudo apt-get install libgstreamer-plugins-base1.0-dev gstreamer1.0-gl
        sudo apt-get install libxvidcore-dev libx264-dev
        sudo apt-get install python3-dev python3-numpy python3-pip
        sudo apt-get install libtbb2 libtbb-dev libdc1394-22-dev
        sudo apt-get install libv4l-dev v4l-utils
        sudo apt-get install libopenblas-dev libatlas-base-dev libblas-dev
        sudo apt-get install liblapack-dev gfortran libhdf5-dev
        sudo apt-get install libprotobuf-dev libgoogle-glog-dev libgflags-dev
        sudo apt-get install protobuf-compiler
fi


# Install opencv
export OPENCV_VERSION="4.8.0"
wget https://github.com/opencv/opencv/archive/${OPENCV_VERSION}.zip && unzip ${OPENCV_VERSION}.zip
cd opencv-${OPENCV_VERSION}
mkdir build
cd build
cmake -D ENABLE_NEON=OFF -D ENABLE_VFPV3=OFF -D WITH_OPENMP=ON -D WITH_OPENCL=OFF -D BUILD_TIFF=ON -D WITH_FFMPEG=ON -D WITH_TBB=ON -D BUILD_TBB=ON -D BUILD_TESTS=OFF -D WITH_EIGEN=OFF -D WITH_GSTREAMER=ON -D WITH_V4L=ON -D WITH_LIBV4L=ON -D WITH_VTK=OFF -D WITH_QT=OFF -D OPENCV_ENABLE_NONFREE=ON -D INSTALL_C_EXAMPLES=OFF -D INSTALL_PYTHON_EXAMPLES=OFF -D BUILD_NEW_PYTHON_SUPPORT=ON -D BUILD_opencv_python3=TRUE -D OPENCV_FORCE_LIBATOMIC_COMPILER_CHECK=1 -D OPENCV_GENERATE_PKGCONFIG=ON -D BUILD_EXAMPLES=OFF ..

#
# -----------------------------------------------------------------------------------------------------------------------
#

# Add swap memory: 
read -p "[Info] You must increase swap size. Change the line CONF_SWAPSIZE=200 to CONF_SWAPSIZE=1024, save and exit. Press Enter to continue." CONTINUE
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

#read -p "[Info] You can restore swap size. Change the line CONF_SWAPSIZE=1024 to CONF_SWAPSIZE=100, save and exit. Press Enter to continue." CONTINUE
#sudo nano /etc/dphys-swapfile
#sudo service dphys-swapfile restart
