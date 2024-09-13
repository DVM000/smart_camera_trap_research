import sys, os
sys.path.insert(0, os.path.abspath(os.path.join(os.getcwd(), './util/models'))) # To import keras_squeezenet.

import tensorflow as tf
from tensorflow.keras.models import Model

#https://www.tensorflow.org/tutorials/keras/save_and_load

def add_top_classifier(base_model, NUM_OUTPUTS, functional_model=False):

    features = base_model.output
    print('[Info] Base network outputs:')
    print(str(features.shape)+ '\t <- network without top') # solo es necesario Global Avg Pool si hay fmpaps con H,W >1
    
    # Apply global average pooling if features are more than 2D
    if len(features.shape) >2:
        global_average_layer = tf.keras.layers.GlobalAveragePooling2D()
        features = global_average_layer(features)
        print(str(features.shape) + '\t <- global avg pooling')

    # Prediction layer
    prediction_layer = tf.keras.layers.Dense(NUM_OUTPUTS)
    predictions = prediction_layer(features)
    print(str(predictions.shape) + '\t <- new top outputs\n') 

    if functional_model:
        print('[Info] Creating tf.keras Functional model')
        from tensorflow.keras.models import Model
        model = Model(inputs=base_model.input, outputs=predictions)
    else:
        print('[Info] Creating tf.keras Sequential model')
        # Only add GlobalAveragePooling2D to Sequential if features are not 1D
        if len(base_model.output.shape) > 2:
            model = tf.keras.Sequential([
                        base_model,
                        global_average_layer,
                        prediction_layer ])
        else:
            model = tf.keras.Sequential([
                        base_model,
                        prediction_layer ])

    return model


def create_squeezenet(NUM_OUTPUTS, functional_model=False):
    #https://github.com/rcmalli/keras-squeezenet

    print('[Info] Loading squeezenet model with a top '+str(NUM_OUTPUTS)+'-outputs classifier')

    from keras_squeezenet import SqueezeNet
    base_model = SqueezeNet(include_top=False, weights='imagenet')#, pooling='avg')

    model = add_top_classifier(base_model, NUM_OUTPUTS, functional_model)
    if not functional_model: Nbaselayers = 1 # fist layer in 'model' is the entire  model
    else:                    Nbaselayers = len(base_model.layers)

    return model, Nbaselayers 

def create_model(name, **kwords):
    # Useful if we have several model architectures.
    model = []
    if 'functional_model' not in kwords:
        kwords['functional_model']=0

    if name == 'squeezenet':
        model, Nbaselayers = create_squeezenet(kwords['num_outputs'],kwords['functional_model']) 

    return model, Nbaselayers

def get_preprocess_function(name):
    func1, func2 = [], []

    default_IMG_SIZE = 224

    # equivalent to from tensorflow.keras.applications.imagenet_utils import preprocess_input
    #https://github.com/keras-team/keras-applications/blob/master/keras_applications/imagenet_utils.py
    def preprocess_img_keras_applications(image, **kwords):
        if 'IMG_SIZE' in kwords:  IMG_SIZE = kwords['IMG_SIZE']
        else:                     IMG_SIZE = default_IMG_SIZE
        image = tf.image.resize(image, (IMG_SIZE, IMG_SIZE))
        from tensorflow.keras.applications.imagenet_utils import preprocess_input # 'RGB'->'BGR'. mu = [103.939, 116.779, 123.68]
        return preprocess_input(image)

    def preprocess_img_keras_applications_inverse(x):
        #https://github.com/keras-team/keras-applications/blob/master/keras_applications/imagenet_utils.py
        mean = [103.939, 116.779, 123.68] # BGR-mean
        x[..., 0] += mean[0]
        x[..., 1] += mean[1]
        x[..., 2] += mean[2]
        x = x[..., ::-1] # 'BGR'->'RGB'
        return x

    func1 = preprocess_img_keras_applications
    func2 = preprocess_img_keras_applications_inverse
    print(' [INFO] PREPROCESSING: Using tensorflow.keras.applications.imagenet_utils.preprocess_input ')

    return func1, func2
