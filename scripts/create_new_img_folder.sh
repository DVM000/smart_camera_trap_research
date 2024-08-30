#! /bin/bash

# Check if there are at least $1 imgs in $IMGS_DIR 
#  -> capture additional images if not enough images (./scripts/Calibration_fromCamera.py)
# Move images from $IMGS_DIR to $DIR/img_data_${2}/Blank/ (all of them)
# Remove $DIR/img_data_${3} if provided    

on_error(){
 echo -e '\033[93m' "[WARNING]: in $(caller)" '\033[0m' >&2 
}

trap on_error ERR

IMGS_DIR=`pwd`/DATASET/FRAMES/
DIR=`pwd`/DATASET/


if [ "$#" -le 1 ]; then
  echo "[Error] Use with 2 arguments: $0 <Nimgs> <N> <pin> [<N2>]"
  echo " to create ${DIR}img_data_<N>/ folder with <Nimgs> images [and delete ${DIR}img_data_<N2>/]."
  echo " New images will be taken (when reading from PIR connected at <pin> is low) in case not enough data."
  exit
else
  Nimgs=$1
  N=$2
  pin_PIR=$3
fi


# Add zero padding to number N
N=`seq -f '%03g' $N $N`

## ---- Check if $IMGS_DIR contains at least $Nimgs images. Otherwise, take images up to $Nimgs (and print 1-error) ----
Ncur=`ls $IMGS_DIR/ | wc -l`
if [ $Ncur -lt $Nimgs ]; then
 echo '1[WARNING] NO images enough. Need '$(($Nimgs-$Ncur))' additional images.'
 # Read images into $IMGS_DIR/Blank and move to $IMGS_DIR/
 python scripts/Calibration_fromCamera.py --N $(($Nimgs-$Ncur)) --dst_folder $IMGS_DIR --pin_PIR $pin_PIR --no-train

 [ ! -d $IMGS_DIR/Blank/ ] && exit 1;                                                # if error -> exit
 [ -d $IMGS_DIR/Blank/ ] && (mv $IMGS_DIR/Blank/* $IMGS_DIR/; rm -r $IMGS_DIR/Blank) # if no error -> copy to $IMGS_DIR/
fi

echo '0[OK] Enough number of images.'

## ---- Create image_data_$N folder with subfolders (remove data if it already exists) ----
[ -d $DIR/img_data_${N}/ ] || mkdir $DIR/img_data_${N}/ 

[ ! -d $DIR/img_data_${N}/Blank/ ] || rm -r $DIR/img_data_${N}/Blank/
[ -d $DIR/img_data_${N}/Blank/ ] || mkdir $DIR/img_data_${N}/Blank/
#[ ! -d $DIR/img_data_${N}/A/ ] || rm -r $DIR/img_data_${N}/A/

## ---- Move ALL data from $IMGS_DIR to 'Blank' folder in image_data_$N (maybe >$Nimgs images) ----
#sudo mv $IMGS_DIR/* $DIR/img_data_${N}/Blank/                                # it may cause error in 'mv' if argument list is too long
sudo find $IMGS_DIR/ -name '*.jpg' -exec mv {} $DIR/img_data_${N}/Blank/ \;   # alternative to 'mv' (working)
echo ' - created '$DIR'/img_data_'${N}/Blank/


## ---- If specified in 4th argument, remove img_data_$4 folder ----
if [ "$#" -ge 4 ]; then
  N=`seq -f '%03g' $4 $4`
  [ ! -d $DIR/img_data_$N ] || (rm -r $DIR/img_data_$N/; echo ' - removed '$DIR'/img_data_'$N)
fi

exit 0
