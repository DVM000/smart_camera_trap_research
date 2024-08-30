# -*- coding: utf-8 -*-
"""

@author: DVM
"""
import sys
import os
import cv2
import numpy as np
from random import randint, seed

import argparse
parser = argparse.ArgumentParser()
parser.add_argument("--N", type=int, help="Number of images to generate", action="store",
                    default = 240)
parser.add_argument("--folder_bg", help="Folder with Background images data", action="store",
                    default = '/home/pi/dataset/train/N/')
parser.add_argument("--folder_save", help="Folder to save background+foreground images", action="store",
                    default = '/home/pi/dataset/train/A/')
parser.add_argument("--folder_fg", help="Folder with Foreground images data", action="store",
                    default = '/home/pi/smart_camera_trap_research/data/cat_crops/')

args = parser.parse_args()

N = args.N
folder_fg = args.folder_fg

folder_bg = args.folder_bg
save_folder = args.folder_save

TAMANO = 3 # range 1,2,3,4 (descending size)


if not os.path.isdir(save_folder):
    os.makedirs(save_folder)
    print('Created folder {}'.format(save_folder))

files_fg = [os.path.join(folder_fg,f) for f in os.listdir(folder_fg)]
files_bg = [os.path.join(folder_bg,f) for f in os.listdir(folder_bg)]
files_bg.sort()

def get_random_image(filelist):
    r = np.random.randint(len(filelist))
    img = cv2.imread(filelist[r], cv2.IMREAD_UNCHANGED)
    print(" - Read image {}".format(filelist[r]))
    return img 

def get_image_k(filelist,k):
    img = cv2.imread(filelist[k], cv2.IMREAD_UNCHANGED)
    print(" - Read image {}".format(filelist[k]))
    return img 

# https://stackoverflow.com/questions/40895785/using-opencv-to-overlay-transparent-image-onto-another-image
def overlay_transparent(background, overlay, x, y):

    background_width = background.shape[1]
    background_height = background.shape[0]

    if x >= background_width or y >= background_height: # coordinates out of background
        return background

    h, w = overlay.shape[0], overlay.shape[1]

    if x + w > background_width: # out of background width
        w = background_width - x
        overlay = overlay[:, :w]

    if y + h > background_height:
        h = background_height - y
        overlay = overlay[:h]

    if overlay.shape[2] < 4: # no alpha channel
        overlay = np.concatenate(
            [
                overlay,
                np.ones((overlay.shape[0], overlay.shape[1], 1), dtype = overlay.dtype) * 255
            ],
            axis = 2,
        )

    overlay_image = overlay[..., :3] # image
    mask = overlay[..., 3:] / 255.0  # mask

    background[y:y+h, x:x+w] = (1.0 - mask) * background[y:y+h, x:x+w] + mask * overlay_image

    return background


#https://stackoverflow.com/questions/44650888/resize-an-image-without-distortion-opencv
def image_resize(image, width = None, height = None, inter = cv2.INTER_AREA):
    # initialize the dimensions of the image to be resized and
    # grab the image size
    dim = None
    (h, w) = image.shape[:2]

    # if both the width and height are None, then return the
    # original image
    if width is None and height is None:
        return image

    # check to see if the width is None
    if width is None:
        # calculate the ratio of the height and construct the
        # dimensions
        r = height / float(h)
        dim = (int(w * r), height)

    # otherwise, the height is None
    else:
        # calculate the ratio of the width and construct the
        # dimensions
        r = width / float(w)
        dim = (width, int(h * r))

    # resize the image
    resized = cv2.resize(image, dim, interpolation = inter)

    # return the resized image
    return resized


def change_size_fg(fg, bg_shape, angle=10, scale=1, shift_h=0, shift_w=0):
    '''
        - Resize fg according to 'bg_shape'. Then, scale fg a 'scale' ratio w.r.t. bg_shape.
        - Shift scaled fg (not exceed bg_shape)
        - Rotate 'angle' degrees    
    '''
    H, W = bg_shape[:2]  
    h, w = fg.shape[:2]
    if h > w:
        fg = image_resize(fg, None, H ) 
    else:
        fg = image_resize(fg, W, None)
    h, w = fg.shape[:2]

    #print(h,w,H,W); cv2.imshow("fg", fg); cv2.waitKey(00)
 
    FG = cv2.resize(fg,(int(w*scale),int(h*scale))) # Fisrt scaling, to keep fg in the center
    #cv2.imshow("FG scaled", FG); cv2.waitKey(00)
   
    # add borders to fg
    borderType = cv2.BORDER_CONSTANT  
    if shift_h + h*scale > H: # not exceed image limits: fg completely contained in image
        shift_h = H - h*scale
    if shift_w + w*scale > W:
        shift_w = W - w*scale
    
    top =    max( int(shift_h ), 0) 
    bottom = max(H-top-int(h*scale), 0)
    left =   max( int(shift_w ), 0) 
    right =  max(W-left-int(w*scale), 0)
    #print(top,bottom,left,right)
    
    FG = cv2.copyMakeBorder(FG, top, bottom, left, right, borderType, None, 0)        
    #FG = FG[:H,:W,...]
    
    FG = rotation(FG,angle,1.0) # rotation after adding borders to keep original fg shape      
    FG = FG[:H, :W,...]
    
    return FG

def rotation(img, angle, scale):
    h, w = img.shape[:2]
    M = cv2.getRotationMatrix2D((h/2,w/2), angle, scale)
    if scale>1:
        dsize = (int(scale*w),int(scale*h))
    else:
        dsize= (w,h)
    rot_img = cv2.warpAffine(img, M, dsize)
    return rot_img

def brightness(img, scale):
    img_max = img.max()
    b = img
    b[:,:,:3] = b[:,:,:3]*scale  # do not scale alpha channel
    I = b>img_max      # out of range
    b[I] = img_max     # keep range [0,255]
    b = np.uint8(b)
    return b

def horizontal_flip(img):
    return cv2.flip(img,1) 


# ----------------------------------------------------------
## MAIN CODE
# ----------------------------------------------------------
for k in range(N):
    
    #bg = get_random_image(files_bg)
    bg = get_image_k(files_bg, k%len(files_bg))
    fg = get_random_image(files_fg)
    #fg = get_image_k(files_fg,5)
    #fg_bgr = fg[:,:,:3]

    H,W = bg.shape[:2]

    # position and data augmentation parameters
    flip = randint(0,1)
    brig = randint(-40,40)/100.0 + 1
    angle   = randint(-10,10)
    shift_h = randint(int(0.0*H),int(H)) # not at the top
    shift_w = randint(0,int(W))
    s = randint(0,100)
    if TAMANO == 1:
        if s<85:        scale   = randint(15,50)/100.0
        elif s<95:      scale   = randint(50,100)/100.0
        else:           scale   = randint(100,150)/100.0;
    if TAMANO == 2:    
        if s<85:        scale   = randint(15,30)/100.0
        elif s<95:      scale   = randint(30,50)/100.0
        else:           scale   = randint(50,100)/100.0;
    if TAMANO == 3:
        if s<85:        scale   = randint(10,20)/100.0
        elif s<95:      scale   = randint(20,50)/100.0
        else:           scale   = randint(50,100)/100.0;
    if TAMANO == 4:
        if s<85:        scale   = randint(5,15)/100.0
        elif s<95:      scale   = randint(15,40)/100.0
        else:           scale   = randint(40,60)/100.0; 

    print("Shift=({},{}), angle={}º, scale={}, brightness={}, Flip={}".format(shift_h,shift_w,angle,scale,brig,bool(flip)))

    if flip:
        fg = horizontal_flip(fg.copy())
    
    fg = change_size_fg(fg.copy(), bg.shape, angle, scale, shift_h,shift_w) 
    fg = brightness(fg.copy(), 0.5)
    #cv2.imshow("full-size FG", fg); cv2.waitKey(00)

    added_image = overlay_transparent(bg.copy(),fg,0,0)
    #cv2.imshow('Combined image', added_image); cv2.waitKey(1) 
    
    # Save generated image
    save_name = save_folder + 'img-{:03d}.png'.format(k)
    cv2.imwrite(save_name, added_image )
    print(" - Saved image {}\n".format(save_name))
    

#cv2.destroyAllWindows()


