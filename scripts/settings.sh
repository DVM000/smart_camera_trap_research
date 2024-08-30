#! /bin/bash

on_error(){
 echo -e '\033[93m' "[WARNING]: in $(caller)" '\033[0m' >&2 
}

trap on_error ERR

# Create folders
[ -d apps ] || mkdir apps
[ -d OUTPUT ] || mkdir OUTPUT   # folder to save set of images when PIR is activated and its classification output
[ ! -d OUTPUT/training_LOG ] || mv OUTPUT/training_LOG /tmp/   # a) save this file
sudo rm -r OUTPUT/*
[ ! -d /tmp/training_LOG ] || mv /tmp/training_LOG ./OUTPUT/   # b) recover file
[ -d OUTPUT/tmp_FRAMES ] || mkdir OUTPUT/tmp_FRAMES

PWD=`pwd`
DIR=DATASET            
DIR2=${DIR}/FRAMES     # folder to save periodic images
d0=img_data_000        
DIR3=${DIR}/$d0        # folder to 1st calibration images
DIR4=${DIR}/FRAMES_tmp    # folder for night images


[ -d $DIR ] || mkdir $DIR
if [ `ls $DIR/ | wc -l` -gt 0 ]; then $(sudo rm -r $DIR/*); fi

[ -d $DIR2 ] || mkdir $DIR2
if [ `ls $DIR2 | wc -l` -gt 0 ]; then $(sudo rm -r $DIR2/*); fi
[ -d $DIR3 ] || mkdir $DIR3
[ -d $DIR4 ] || mkdir $DIR4


# Edit configuration file
echo '1) Edit configuration file'
read -p 'Edit Configuration file. Press Enter' A
nano configuration.ini 

# Create subfolders
L=`grep -i 'LABEL_FILE' configuration.ini`

./scripts/create_classes_folders.sh ${L//\LABEL_FILE = /}


echo '2) Run as: sudo ./apps/APP '



