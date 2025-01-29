#! /bin/bash

# DVM. 2024
# based on https://qengineering.eu/install-tensorflow-on-raspberry-64-os.html

# get a fresh start
sudo apt-get update
sudo apt-get upgrade

# install pip3
sudo apt-get install git python3-pip

echo "[Info] You may need to edit pip.conf file. Add this lines:" 
echo -e " \033[0;93m [Info] You may need to edit pip.conf file. Add this lines: \033[0m " 
echo -e " \033[0;93m [global] \033[0m " 
echo -e " \033[0;93m break-system-packages=true \033[0m" 
read -p "Press Enter to continue." CONTINUE
sudo nano /etc/pip.conf

# install correct version protobuf
sudo -H pip3 install --upgrade protobuf==3.20.0

# remove old versions, if not placed in a virtual environment (let pip search for them)
sudo pip uninstall tensorflow
sudo pip3 uninstall tensorflow

# download wheel
git clone https://github.com/Qengineering/Tensorflow-io.git
cd Tensorflow-io
sudo -H pip3 install tensorflow_io_gcs_filesystem-0.23.1-cp311-cp311-linux_aarch64.whl 
cd ~

# install TensorFlow 2.14.0
sudo -H pip3 install --upgrade tensorflow==2.14.0




