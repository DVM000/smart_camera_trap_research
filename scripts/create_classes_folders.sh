#! /bin/bash

on_error(){
 echo -e '\033[93m' "[WARNING]: in $(caller)" '\033[0m' >&2 
}

trap on_error ERR

# Read lines from file (1st argument) and create corresponding subfolders inside FOLDER
folder=OUTPUT
if [ "$#" -le 0 ]; then
  echo "[Error] Use with 1 argument: $0 <file>"
  exit
else
  file=$1
fi


echo 'Creating one folder per class in' file:

[ -f $file ] || echo -e '\033[91m' "[ERROR] FILE $file DOES NOT EXIST. Select another file" '\033[0m' 

while IFS= read -r line
do
  echo ' --' ${line// /}
  [ -d $folder/${line// /} ] || mkdir $folder/${line// /}
done < $file

# last line
echo ' --' ${line// /}
[ -d $folder/${line// /} ] || mkdir $folder/${line// /}


