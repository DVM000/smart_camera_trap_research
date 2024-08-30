# Delia Velasco-Montero


import os
import sys
import numpy as np

import logging
#tf.get_logger().setLevel(logging.ERROR)

import logging as log
log.basicConfig(format='[%(levelname)s] %(message)s', level=log.INFO)

class bcolors:
    HEADER = '\033[95m'
    OKBLUE = '\033[94m'
    OKCYAN = '\033[96m'
    OKGREEN = '\033[92m'
    WARNING = '\033[93m'
    FAIL = '\033[91m'
    ENDC = '\033[0m'
    BOLD = '\033[1m'
    UNDERLINE = '\033[4m'


## read configuration from config.ini file
    # Configuraciones personalizadas: https://docs.python.org/3/library/configparser.html
    # -----------------------------------------------------------------------------------
def read_config_section(section_data='DEFAULT', waitkey=True, filename=''):  
    '''
    Return a dictionary

    Undo dictionary: 
        for key in conf:
            try:      exec(key +" = " + str( conf[key]) ) # number
            except:   exec(key +" = '" + str( conf[key]) +"'" ) # string '...'
    '''

    import configparser
    if filename == '':
        pwd = os.path.dirname( os.path.abspath(__file__))
        config_file = pwd + '/config.ini'
    else:
        config_file = filename
    config = configparser.ConfigParser()
    config.read(config_file)

    print('[Info] Loading configuration settings:')   
    try:
      cfg = config[section_data]
    except:
      log.error('Config section ' + section_data + ' not found. Select one of the following (specified in config file):')
      print(list(config._sections.keys()))
      
      import sys; sys.exit(1)
    conf = {} # dictionary

    for key in cfg:  # load associated values in that section

        try:        value = int(str(cfg[key])) # int?
        except:
            try:    value = float(str(cfg[key])) # float?
            except: 
                value = "'" + cfg[key] + "'"     # default: string

        print('------ '+ key.upper() +" = " + str(value) )
        exec("conf['"+key.upper() +"'] = " + str( value ) )

     # Additional configuration
    conf['OUTPUTS_PATH'] = './output/'
    conf['PLOTS_PATH'] = conf['OUTPUTS_PATH'] + 'figures/'
    
    if waitkey:
      text = input("\n[Info] Is this OK? If yes, type in enter (otherwise, any key): ")  # or raw_input in python2
      if text != "":
          import sys; sys.exit(0)

    return conf

def read_config_key_from_section(section_data, key):
    pwd = os.path.dirname( os.path.abspath(__file__))
    import configparser
    config_file = pwd + '/config.ini'
    config = configparser.ConfigParser()
    config.read(config_file)
    cfg = config[section_data]
    return cfg[key]
