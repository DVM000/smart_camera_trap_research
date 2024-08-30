#! /bin/bash

# Install MQTT broker:
#-------------------------------------------------------------------------------------------------
sudo apt-get update
sudo apt-get update

sudo apt-get install mosquitto mosquitto-clients

# Convert it in on-boot service:
#-------------------------------------------------------------------------------------------------
sudo systemctl enable mosquitto.service


# Configure broker:
#-------------------------------------------------------------------------------------------------
sudo touch /etc/mosquitto/passwd
sudo touch /etc/mosquitto/mosquitto.log
sudo chmod 777 /etc/mosquitto/mosquitto.log

sudo cp mosquitto.conf /etc/mosquitto/
sudo mosquitto_passwd -U passwd_mosquitto 

sudo systemctl restart mosquitto.service




