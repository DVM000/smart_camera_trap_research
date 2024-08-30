# Read images from specified camera, if the PIR is not activated.
# Then, launch training process (if not using --no-train option)

from time import sleep
import cv2
import sys
import os
from argparse import ArgumentParser
import datetime
import RPi.GPIO as gpio

parser = ArgumentParser()
parser.add_argument('--N', type=int, help='Number of images to take', default=240)
parser.add_argument('--sec', type=float, help='Rate of image capturing (seconds/image)', default=1)
parser.add_argument('--n_cam', type=int, help='Camera number', default=0)
parser.add_argument('--dst_folder', help='Folder to save images from camera', default='./DATASET/img_data_000/')
parser.add_argument('--pin_PIR', help='GPIO number to PIR sensor. Set -1 to avoid PIR readings', type=int, default=17)
parser.add_argument('--t_config', help='Config file for training. Use \'\' for taking default file', default='training_config.ini')
parser.add_argument('--no-train', action='store_false', dest='no_train', default=True, help='Use to only take data, but neither create dataset nor train the network')
parser.add_argument('--H', type=int, help='Frame height', default=240)
parser.add_argument('--W', type=int, help='Frame width', default=320)

args = parser.parse_args()

N_imgs = args.N
s_sleep = args.sec
NUM = args.n_cam
FOLDER = args.dst_folder
PIR_PIN = args.pin_PIR
TRAIN = args.no_train
H = args.H
W = args.W


# PIR Sensor
if PIR_PIN>-1:
    gpio.setmode(gpio.BCM)
    gpio.setup(PIR_PIN, gpio.IN, pull_up_down=gpio.PUD_DOWN)           # Set our input pin to be an input
    print('Using PIR sensor connected at pin #{}'.format(PIR_PIN) )

# Camera
cam = cv2.VideoCapture(NUM)
print('Accessing camera {}'.format(NUM))
cam.set(cv2.CAP_PROP_FRAME_WIDTH, W)
cam.set(cv2.CAP_PROP_FRAME_HEIGHT, H)

if cam.isOpened():
  print('Camera {} open'.format(NUM))
else:
  print('Cannot access to camera ' + str(NUM))
  sys.exit(1)
  
if not os.path.isdir(FOLDER+'/Blank/'):
    os.makedirs(FOLDER+'/Blank/')
else:
    os.system('rm {}/*'.format(FOLDER+'/Blank/'))
  
# Start loop
n = 0
try:
    while n < N_imgs:
        while (PIR_PIN>-1) and (gpio.input(PIR_PIN) == True):
            print('PIR detection...waiting...')
            sleep(1)
            
        r, frame = cam.read()
        now = datetime.datetime.now()
        img_name = FOLDER + '/Blank/{}_{:03d}.jpg'.format(now.strftime("%Y-%m-%d_%H:%M:%S"),n) #'%A_%B__%d_%H:%M:%S_%Y'), n)
        cv2.imwrite(img_name, frame)
        print('  Read frame #{} {}x{} --> {}'.format(n,frame.shape[0],frame.shape[1],img_name))
        
        sleep(s_sleep)
        n += 1

except KeyboardInterrupt:
    print('END')

cam.release()
print('Camera {} closed'.format(NUM))

# clean up GPIOs
gpio.cleanup()


if not TRAIN:
    sys.exit(0)


os.system('python ./ondevice_training/create_synthetic_fg_bg.py --N {} --folder_save {}/A/ --folder_bg {}/Blank/'.format(N_imgs, FOLDER, FOLDER))

print('[Info] Training model according to calibrate0 section in {}. Log file: ./OUTPUT/training_LOG_0'.format(args.t_config))
os.system('sudo -u pi python3 ./ondevice_training/A_learning.py --config calibrate0 --configfile {} >> ./OUTPUT/training_LOG_0 2>&1'.format(args.t_config))