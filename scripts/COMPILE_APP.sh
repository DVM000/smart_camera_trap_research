#! /bin/bash

on_error(){
 echo -e '\033[93m' "[WARNING]: in $(caller)" '\033[0m' >&2 
}

trap on_error ERR

[ -d ./apps ] || mkdir apps

# Compile App-test2b with OpenCV:
g++ src/APP.cpp -o apps/APP -lpigpio -lrt -lpthread `pkg-config --cflags --libs opencv`


