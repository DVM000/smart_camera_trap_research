#! /bin/bash

# DVM. 2020
# based on https://qengineering.eu/install-tensorflow-2.1.0-on-raspberry-pi-4.html

# get a fresh start
sudo apt-get update
sudo apt-get upgrade

# remove old versions, if not placed in a virtual environment (let pip search for them)
sudo pip uninstall tensorflow
sudo pip3 uninstall tensorflow

# install the dependencies (if not already onboard)
if [ ]; then
sudo apt-get install gfortran
#sudo apt-get install libhdf5-dev libc-ares-dev libeigen3-dev
sudo apt-get install libatlas-base-dev libopenblas-dev libblas-dev
sudo apt-get install liblapack-dev cython
sudo pip3 install pybind11
#sudo pip3 install h5py
fi

# for loading hdf5 models:
sudo apt-get install libhdf5-dev libc-ares-dev libeigen3-dev
sudo pip3 install h5py

# download the wheel
wget https://github.com/Qengineering/Tensorflow-Raspberry-Pi/raw/master/tensorflow-2.1.0-cp37-cp37m-linux_armv7l.whl

# install TensorFlow
sudo -H pip3 install tensorflow-2.1.0-cp37-cp37m-linux_armv7l.whl

# Change h5py version according to warning shown when 'import tensorflow'
python3 -c 'import tensorflow'
HDF5_VERSION=1.10.6 pip3 install --no-binary=h5py h5py==3.1.0

pip3 install protobuf==3.20.0  # this may be needed (RPi Zero)

sudo pip3 install -r requirements_TF.txt

# and complete the installation by rebooting
reboot
