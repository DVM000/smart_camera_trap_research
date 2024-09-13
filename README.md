# Research on smart camera traps

## Citing

Please cite our paper if you find this repository useful.

```
@article{VELASCOMONTERO2024102815,
  title = {Reliable and efficient integration of AI into camera traps for smart wildlife monitoring based on continual learning},
  journal = {Ecological Informatics},
  volume = {83},
  pages = {102815},
  year = {2024},
  issn = {1574-9541},
  doi = {https://doi.org/10.1016/j.ecoinf.2024.102815},
  url = {https://www.sciencedirect.com/science/article/pii/S1574954124003571},
  author = {Delia Velasco-Montero and Jorge Fernández-Berni and Ricardo Carmona-Galán and Ariadna Sanglas and Francisco Palomares},
}
```

https://www.sciencedirect.com/science/article/pii/S1574954124003571


## Hardware

Compatible with Raspberry Pi devices. Tested, in particular, on Raspberry Pi 3B, Raspberry Pi 4B, and Raspberry Pi Zero 2W.


## Installation

1. **Download this repository**:

```
git clone https://www.github.com/DVM000/smart_camera_trap_research.git 
```

 - Installation scripts assume that code is located at `/home/pi/smart_camera_trap_research/`
 - You must give execution permission to all scripts.

2. **Install dependences** (it will take a long while):

```
cd scripts
sudo ./install-dependences.sh
cd ..
```


3. **Build the application**. It will generate the executable file in `/apps/APP`

```
./scripts/COMPILE_APP.sh
```

Enable camera using `raspi-config`.


*Note: tested on Rasbian GNU/Linux 10 (buster) kernel Linux 6.1.61-v8+ aarch64 32 bits with OpenCV version 4.4.0 and TensorFlow 2.1.0 (Python 3.7)*


## Configuration

Run this script to pre-configure the applicaction:

```
./scripts/settings.sh
```

You can use parameters by default or you can edit `configuration.ini` according to required application parameters. 

You can use parameters by default or you can edit `training_config.ini` for configuration of on-device training parameters.



## Usage

You can direcly run the application with:

```
sudo ./scripts/service_app/Launch_service.sh
```

Or you can also establish it as a system service for executing on boot:
 
```
sudo cp ./scripts/service_app/camera-trap.service /lib/systemd/system/  
sudo systemctl enable camera-trap.service
sudo reboot
```


## Outputs

 - Application log will be `./OUTPUT/App_log_<date>` . This log contains chronologically-ordered information about camera status, 
    PIR activation, animal detection, re-training status, night detection, etc (including timestamps);   
    as well as possible source or errors encountered during execution.
 - Classified frames will be located into `./OUTPUT/<classname>` folders. 
 - Periodically collected frames for on-device training are stored into `./DATASET/FRAMES/`.
 - Training image datasets will be built into `./DATASET/img_data_XXX` folders.










 	





