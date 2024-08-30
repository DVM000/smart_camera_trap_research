# Take N images from folders, according to specified percentages and copy to dst_folder/Blank/
# Also create images into dst_folder/A/
# Example:
#    take 100 images from ./DATASET/img_data_004-3-2-1 with percentages 44.8%, 36.7%, 13.5%, 5%,
#    and copy to ./DATASET/Train/Blank/
#    and create images into ./DATASET/Train/A/

import sys
import os
import numpy as np
from argparse import ArgumentParser

parser = ArgumentParser()
parser.add_argument('--N', type=int, help='Number of images to take', default=240)
parser.add_argument('--pattern_dataset', help='Pattern in folders with data. Example: ./DATASET/img_data_*', default='./DATASET/img_data_*')
parser.add_argument('--dst_folder', help='Folder to save images', default='./DATASET/Train/')
#parser.add_argument('--t_config', help='Config file for training. Use \'\' for taking default file', default='training_config.ini')
args = parser.parse_args()

N_imgs = args.N
N_imgs_sub = N_imgs #10 # number of images per experience
DST_FOLDER = args.dst_folder
patt_data = args.pattern_dataset

PATHp = patt_data.split('/')[:-1]
PATH = ''
for k in PATHp:
    PATH += k+'/'

patt = patt_data.split('/')[-1].split('*')[0]
FOLDERS_DATA = []
for f in os.listdir(PATH):
    if patt in f:
        FOLDERS_DATA.append(f)
        #print(f)

## we assume numbers with zero padding, thus folders are correctly ordered
FOLDERS_DATA.sort()
#for k in FOLDERS_DATA:
#    print(k)
    #if not os.path.isdir(DST_FOLDER+'/A/'):    os.makedirs(FOLDER+'/A/')

percent = [0.448, 0.367, 0.135, 0.05]


def load_image_names(PATH, N=0, shuffle=0): 
  # Create array 'data' containing filenames from PATH directory.
  # if N>0, take only N imgaes. Otherwise return all images
  # if shuffle, images are randomly selected

  data = []
  if not os.path.isdir( PATH ):
      print('{}: No images found.'.format(PATH))
  for f in os.listdir(PATH):
      data.append(f)

  print('{}: Found {} images.'.format(PATH, len(data)))
  data.sort()
      
  if shuffle:
      idx = np.random.permutation( np.arange(len(data)) ) # random permutation
      if N: idx = idx[:N]
      data = [PATH+data[k] for k in idx]
      
  if N:  print('       but taken up to {} images.'.format(N))
  else:  print('       but taken {} images.'.format(len(data)))
       
  return data


all_files = []

for k in range(len(FOLDERS_DATA)):
    data = load_image_names(PATH + FOLDERS_DATA[k] + '/Blank/', N=N_imgs_sub, shuffle=1)
    all_files.append(data)


mixX = []; 
for p in range(len(FOLDERS_DATA)):  # for p in past experiences
    if p>=len(percent):     perc = 0
    else:                   perc = percent[p]
    
    if len(FOLDERS_DATA)<len(percent) and p==0:  # if percentages does not add up 100% (only few past experiences)
        perc = 1 - sum(percent[1:len(FOLDERS_DATA)])  # this last experience takes higher percentage (100% - past) ["past"=1:last]
        if perc <= 0: perc = 1 # for case in which percent = [1, 1, 1, 1, ...]
        
    # take only a percentage of data
    Ntake = int(np.ceil(perc*N_imgs))
    if len(all_files[::-1][p][:Ntake]) < Ntake:
        print('[WARNING] No enough images in {} to take {} imgs.\t\t[WARNING]'.format(FOLDERS_DATA[len(FOLDERS_DATA)-p-1], Ntake))
        Ntake = len(all_files[::-1][p])
    mixX += all_files[::-1][p][:Ntake]
    print(' - Taking {} imgs ({:.1f}%) from experience i-{} ({})'.format(Ntake, 100*perc, p, FOLDERS_DATA[len(FOLDERS_DATA)-p-1]))


print('Total of {} imgs selected from {} experiences (max {} experiences).'.format(len(mixX),np.min((len(FOLDERS_DATA),len(percent))), len(percent)))

if not os.path.isdir(DST_FOLDER+'/Blank/'):
    os.makedirs(DST_FOLDER+'/Blank/')
else:
    os.system('rm {}/*'.format(DST_FOLDER+'/Blank/'))
if not os.path.isdir(DST_FOLDER+'/A/'):
    os.makedirs(DST_FOLDER+'/A/')
else:
    os.system('rm {}/*'.format(DST_FOLDER+'/A/'))
    
for k in mixX:
    os.system('cp {} {}'.format(k, DST_FOLDER+'/Blank/'))
print('Copied images to {}/Blank'.format(DST_FOLDER))


os.system('python ./ondevice_training/create_synthetic_fg_bg.py --N {} --folder_save {}/A/ --folder_bg {}/Blank/ > /dev/null'.format(len(mixX), DST_FOLDER, DST_FOLDER))
print('Created images into {}/A/'.format(DST_FOLDER))
