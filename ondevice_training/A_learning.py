#!/usr/bin/env python
# -*- coding: utf-8 -*-

# Delia Velasco-Montero, ago-2022

# Partially based on:
#https://www.tensorflow.org/tutorials/images/transfer_learning
#https://github.com/ContinualAI/colab/blob/master/notebooks/intro_to_continual_learning.ipynb
#https://gist.github.com/Madhivarman/676650f71ec35a5f2802631fcfa0ff73

# Model can be restored from a previous training checkpoint:
# - FROM_CKPT: indicate path to folder containing checkpoint .index and .data-xxxx-of-xxxx files for the trained weights
# - FROM_H5:   indicate .h5 file containing both network architecture and trained weights

from __future__ import absolute_import, division, print_function, unicode_literals

import os
import sys
sys.path.insert(0,os.path.dirname(os.path.realpath(__file__))+'/util/models/')
import numpy as np
import tensorflow as tf

from tensorflow.keras.preprocessing.image import ImageDataGenerator
from tensorflow.keras.models import Model

import logging as log
tf.get_logger().setLevel(log.ERROR)
log.basicConfig(format='[%(levelname)s] %(message)s', level=log.INFO)

from util.help_functions import read_config_section, bcolors

import argparse
parser = argparse.ArgumentParser()
parser.add_argument("--config", help="Data section in Config file (config.ini by default)", action="store",
                    default = 'calibration')
parser.add_argument("--configfile", help="Config file", action="store",
                    default = '')

args = parser.parse_args()
sec  = args.config

## -- Read configuration variables -- ##
# ------------------------------------------------------------------------ #
conf = read_config_section(sec, waitkey=0, filename=args.configfile)
for key in conf:
  try:      exec(key +" = " + str( conf[key]) ) # number
  except:   exec(key +" = '" + str( conf[key]) +"'" ) # string '...'

CONVERTING = 1 # convert to OpenCV format (.pb)

pwd = os.path.dirname( os.path.abspath(__file__))
OUTPUTS_PATH = pwd+'/'+OUTPUTS_PATH
OUT_MODEL_PATH = pwd+'/'+OUT_MODEL_PATH
ckpt_folder = 'ckpts/'

# solving problem of corruputed files
from PIL import ImageFile
ImageFile.LOAD_TRUNCATED_IMAGES = True


## -- Data loading and preprocessing -- ##
# ------------------------------------------------------------------------ #
log.info('\t----- Loading training dataset... -----\n' + 
        '-------------------------------------------------------------------------------------------------\n')

# important: remove corrupt files (otherwise, training process can't be performed):
os.system("find {} -type 'f' -size -1k -delete".format(TRAIN_PATH))

from util.models.models import get_preprocess_function
preprocess_img, preprocess_img_inverse = get_preprocess_function( MODEL)#, IMG_SIZE=IMG_SIZE )

train_img_gen = ImageDataGenerator(
                                  preprocessing_function=(lambda x: preprocess_img(x, IMG_SIZE=IMG_SIZE)), 
          #validation_split=0.2, # validation split. It will only be split in case of using subset='training'/'validation' when using flow_from_directory()
          zoom_range=[0.75,1], # values <1 zoom in, values >1 zoom out
          width_shift_range=0.1,
          height_shift_range=0.1,
          horizontal_flip=True,
          rotation_range = 5,
          brightness_range = [0.4, 1],
          shear_range = 5,
          fill_mode = 'reflect',                       
          )

if NUM_OUTPUTS == 1:
  class_mode = 'binary'
else: 
  class_mode = 'categorical'

train_data_gen = train_img_gen.flow_from_directory(
        batch_size=BATCH_SIZE,
        directory=TRAIN_PATH,  # training data
        shuffle=True,
        target_size=(IMG_SIZE, IMG_SIZE),
        class_mode= class_mode, 
        color_mode="rgb")


## -- Create the base model from the pre-trained convnets -- ##
# ------------------------------------------------------------------------ #
print('\n'); log.info('\t----- Loading convolutional neural network - name: '+ MODEL +' ----- \n' +
                      '-------------------------------------------------------------------------------------------------\n')
IMG_SHAPE = (IMG_SIZE, IMG_SIZE, 3)

if FROM_H5:
    print(bcolors.WARNING +'\n\n[Info] Resuming from model {}'.format( FROM_H5 ) + bcolors.ENDC)
    model = tf.keras.models.load_model( FROM_H5 )
    Nbaselayers = len(model.layers)-3 # GAP, dense, softmax
else:
    from util.models.models import create_model
    model, Nbaselayers = create_model(MODEL,img_shape=IMG_SHAPE,num_outputs=NUM_OUTPUTS, functional_model=FUNCTIONAL_MODEL)


# Resume training
# ----------------------------------------------------------------------------------
if not FROM_H5 and FROM_CKPT:
  # Load the previously saved weights
  print('\n\n'); log.info('\t----- Restoring fine-tuned model from checkpoint... ----- ')
  checkpoint_path = os.path.dirname(os.path.realpath(__file__)) + OUTPUTS_PATH.split('.')[1] + ckpt_folder + FROM_CKPT +"-{epoch:02d}".format(epoch=EPOCH_RESUME)
  # b) from epoch number
  print('\t\t· Checkpoint: ' + checkpoint_path)
  model.load_weights( checkpoint_path)
  print(bcolors.WARNING +'\n\n[Info] Resuming from epoch {:02d} in checkpoint {}'.format(EPOCH_RESUME, checkpoint_path) + bcolors.ENDC)
#--------------------------------------------------------------------------------------------------------------


## --  Training SETTINGS -- ##
# ------------------------------------------------------------------------ #
for layer in model.layers[:Nbaselayers]:
  layer.trainable =  False

optimizer = eval('tf.keras.optimizers.'+OPT)

from tensorflow.keras import backend as K
f_logits = not FROM_H5
if NUM_OUTPUTS == 1:
  lossfun = tf.keras.losses.BinaryCrossentropy(from_logits=f_logits) # True if softmax not applied
  metr = ['accuracy', 
           tf.keras.metrics.TopKCategoricalAccuracy(k=5, name='Top5_acc')] # this does not matter, but included here for ease of code
else:
  lossfun = tf.keras.losses.CategoricalCrossentropy(from_logits=f_logits) # True if softmax not applied
  metr =[ tf.keras.metrics.CategoricalAccuracy(name='accuracy'), # Top1-acc
          tf.keras.metrics.TopKCategoricalAccuracy(k=5, name='Top5_acc')]

model.compile(optimizer=tf.keras.optimizers.RMSprop(lr=5e-3, decay=1e-5), loss= lossfun, metrics= metr )


## -- Fine-tune the network from layer FINETUNE_FROM_LAYER -- ##
# ------------------------------------------------------------------------ #
if EPOCHS:
  print('\n\n'); log.info('\t----- Training network (fine-tune) ----- \n' +
                        '-------------------------------------------------------------------------------------------------\n')
  # reset our data generator
  train_data_gen.reset()

  ## Unfreeze base model: (BEFORE COMPILING THE MODEL!!)
  try:
    for layer in model.layers[:Nbaselayers]:
      layer.trainable =  True

    # Freeze all the layers before the `fine_tune_at` layer
    if Nbaselayers == 1:    base_model = model.layers[0]
    else:                   base_model = model
    fine_tune_at = FINETUNE_FROM_LAYER  # Fine-tune from this layer onwards
    for layer in base_model.layers[:fine_tune_at]:
      layer.trainable =  False
  except:
    model.trainable = True
    print('error')
    
  model.compile(optimizer= optimizer, 
                  loss= lossfun,
                  metrics= metr)

  start_epoch = 0
  acc = []; val_acc = []; loss = [];  val_loss = [];
    
  history_finetune = model.fit( #model.fit_generator(
      train_data_gen,
      steps_per_epoch=train_data_gen.samples // BATCH_SIZE,
      epochs=EPOCHS, 
      initial_epoch =  start_epoch, 
      #validation_data=val_data_gen,
      #validation_steps=val_data_gen.samples // BATCH_SIZE,
      #callbacks=[cp_callback]#, tensorboard_callback] #, batch_stats_callback]
  )

  # alternatively, other options:
  if not FROM_H5:
      if FUNCTIONAL_MODEL:
        from tensorflow.keras.models import Model
        softmax_layer = tf.keras.layers.Softmax()
        softmax_prob = softmax_layer(model.output) 
        newmodel = Model(inputs=model.input, outputs=softmax_prob)
        model = newmodel
        model.compile(optimizer= optimizer, loss= lossfun, metrics= metr)
      else:
        model.add( tf.keras.layers.Softmax() )


# --------------------------------------------------------------------------------
## CONVERT TO .pb FILE 
# --------------------------------------------------------------------------------
if not CONVERTING:
    sys.exit(0)
    
H5_file = OUT_MODEL_PATH + OUT_MODEL_NAME +'.h5' 

pb_folder = OUT_MODEL_PATH 
pb_filename = OUT_MODEL_NAME +'.pb'
np.random.seed(0)

model.save(H5_file)  # need to save model, otherwise model.predict() does not work (But .pb model may work?)
print('\n[Info] Saved {}'.format(H5_file)) 

tf_version = int( tf.__version__.split('.')[0] ) 
tf.keras.backend.clear_session()
tf.keras.backend.set_learning_phase(0)  

# Needed to correctly generate the TensorFlow Graph in TensorFlow 2.x:
if tf_version > 1:   
  tf.compat.v1.disable_eager_execution()


## FREEZE MODEL TO TF FROZEN GRAPH
## ----------------------------------------------------------------
def freeze_session(session, keep_var_names=None, output_names=None, clear_devices=True):
    """
    Freezes the state of a session into a pruned computation graph.

    Creates a new computation graph where variable nodes are replaced by
    constants taking their current value in the session. The new graph will be
    pruned so subgraphs that are not necessary to compute the requested
    outputs are removed.
    @param session The TensorFlow session to be frozen.
    @param keep_var_names A list of variable names that should not be frozen,
                          or None to freeze all the variables in the graph.
    @param output_names Names of the relevant graph outputs.
    @param clear_devices Remove the device directives from the graph for better portability.
    @return The frozen graph definition.
    """
    from tensorflow.python.framework.graph_util import convert_variables_to_constants
    graph = session.graph
    #init = tf.global_variables_initializer() # ** anadido esto
    #session.run(init) # **
    with graph.as_default():
        freeze_var_names = list(set(v.op.name for v in tf.compat.v1.global_variables()).difference(keep_var_names or []))
        output_names = output_names or []
        output_names += [v.op.name for v in tf.compat.v1.global_variables()]
        # Graph -> GraphDef ProtoBuf
        input_graph_def = graph.as_graph_def()
        if clear_devices:
            for node in input_graph_def.node:
                node.device = ""
        frozen_graph = convert_variables_to_constants(session, input_graph_def,
                                                      output_names, freeze_var_names)
        return frozen_graph


## LOAD KERAS MODEL
## ----------------------------------------------------------------
model = tf.keras.models.load_model(H5_file) 
print('[Info] Model correctly loaded')
SIZE = model.inputs[0].shape[1]


## FREEZE MODEL AND SAVE IT AS .PB FILE
## ----------------------------------------------------------------
if tf_version > 1:    session = tf.compat.v1.keras.backend.get_session() 
else:                 session = tf.keras.backend.get_session() 

frozen_graph = freeze_session(session,
                              output_names=[out.op.name for out in model.outputs])

tf.compat.v1.train.write_graph(frozen_graph, pb_folder, pb_filename, as_text=False)
print('\n[Info] Saved {}{}'.format(pb_folder,pb_filename))
print('---------------------------------------------------------- \n')


sys.exit(0)
