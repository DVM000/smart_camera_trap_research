/*
 * Functions and libraries required for the aaplication
 */

#include <fstream>
#include <sstream>
//#include <iostream>
//#include <string>
#include <vector>

#include <stdio.h>
#include <pigpio.h>

#include <opencv2/opencv.hpp>

//#include <chrono>         // std::chrono::seconds
#include <ctime>  

//#include <algorithm>
//#include <boost/filesystem.hpp>
//#include <unistd.h>
#include <numeric>

#include "common.hpp"

using namespace cv;
//using namespace std;


/* USER VARIABLES SPECIFIED IN THIS CONFIGURATION FILE */
std::string CONFIG_FILE = "configuration.ini";

/* USER VARIABLES -- default values -- */
int INPUT_PIN = 4; 
int OUTPUT_PIN = 3; 
float READ_DELAY = 1; 
float READ_FPS = 2; 
int WIDTH = 320;
int HEIGHT = 240;
int POST_CAPTURE = 5;
int CONFIRM_GAP = 5; 
float READ_DELAY_CONFIRM = 0.25;
float DET_PERIOD = 0.5;
int EVENT_GAP = 5;
float MAX_RUNTIME = 60.0;
float CALIB_PERIOD = 0;
std::string FRAMES_FOLDER = "./OUTPUT/tmp_FRAMES/";

std::string CNN_FILE = "/home/pi/path/file.pb";
int CNN_SIZE = 227;
std::string LABEL_FILE = "/home/pi/path/file.txt";
std::vector<int> TARGET_LABELS = {2,3};


std::string CAMERA_NAME = "RPi-Camera";
std::string DST_EMAIL = "hello@hello.com";
    
std::string OUTPUT_FILE = "Inference_LOG.txt";

float CALIB_PERIOD_red = 1; // reduction of CALIB_PERIOD
int n_calib = 0;            // number of re-calibrations done
int NIMGS_CAL = 240;        // number of images 

bool LED = 1;          // whether to use a LED to show camera status

/* 
 * Testing and developing variables ------------------------------------
 */
 
bool verbose = 0;//1;  // shows extra information in command line
bool MQTT    = 1;      // send information via MQTT (it requires internet connection and mosquitto service active)
std::string hostIP = ""; // host IP

// for testing on a particular image folder
std::string TEST_IMG_FOLDER = "";  // Images to be classified by the CNN. Set an empty value ("") for using real images 
std::string MAX_CNN_IMAGES = "50";  // maximum number of images to be classified by the CNN


int SEC_TEST = 30; // take 1 frame each SEC_TEST seconds --> set to 30
double last_frame; // instant of last captured frame
std::string FRAMES_TEST_FOLDER = "./DATASET/FRAMES/";
std::string FRAMES_tmp_FOLDER = "./DATASET/FRAMES_tmp/";

/* --------------------------------------------------------------------- */

/* GLOBAL VARIABLES */
#include "opencv_classif_functions.cpp"
ModelOpenCVClass modelObj; 

//#include "tflite_classif_functions.cpp"
//ModelTFLiteClass modelObj; 

/* EDIT to change framework:
 * - modelObj class and its corresponding #include <>.cpp
 * - libraries at compilation time
 * - configuration_<>.ini file, variable CNN_FILE
 * /


/* 
 * FUNCTIONS 
 */
 
static bool readConfiguration(std::string CONFIG_FILE){
    /* Read particular parameters from CONFIG_FILE */
    
      configuration::data configdata;
      std::ifstream f( CONFIG_FILE );
      f >> configdata;
      f.close();
      std::cout << std::endl << "[INFO] Read " << CONFIG_FILE << ". Options set to:" << std::endl;
      std::cout << configdata;
      std::cout << " ------------------------------------------------------------ " << std::endl << std::endl;
                
     if (configdata.find("INPUT_PIN") != configdata.end())       INPUT_PIN = std::atoi(configdata["INPUT_PIN"].c_str());
     if (configdata.find("OUTPUT_PIN") != configdata.end())      OUTPUT_PIN = std::atoi(configdata["OUTPUT_PIN"].c_str());
     if (configdata.find("READ_DELAY") != configdata.end())      READ_DELAY = std::atof(configdata["READ_DELAY"].c_str());
     if (configdata.find("READ_FPS") != configdata.end())        READ_FPS = std::atof(configdata["READ_FPS"].c_str());
     if (configdata.find("WIDTH") != configdata.end())           WIDTH = std::atoi(configdata["WIDTH"].c_str());
     if (configdata.find("HEIGHT") != configdata.end())          HEIGHT = std::atoi(configdata["HEIGHT"].c_str());
     if (configdata.find("POST_CAPTURE") != configdata.end())    POST_CAPTURE = std::atoi(configdata["POST_CAPTURE"].c_str());
     if (configdata.find("CONFIRM_GAP") != configdata.end())     CONFIRM_GAP = std::atoi(configdata["CONFIRM_GAP"].c_str());
     if (configdata.find("READ_DELAY_CONFIRM") != configdata.end())      READ_DELAY_CONFIRM = std::atof(configdata["READ_DELAY_CONFIRM"].c_str());
     if (configdata.find("DET_PERIOD") != configdata.end())       DET_PERIOD = std::atof(configdata["DET_PERIOD"].c_str());
     if (configdata.find("EVENT_GAP") != configdata.end())        EVENT_GAP = std::atoi(configdata["EVENT_GAP"].c_str());
     if (configdata.find("MAX_RUNTIME") != configdata.end())      MAX_RUNTIME = std::atof(configdata["MAX_RUNTIME"].c_str());   
     if (configdata.find("CALIB_PERIOD") != configdata.end())     CALIB_PERIOD = std::atof(configdata["CALIB_PERIOD"].c_str()); 
     if (configdata.find("NIMGS_CAL") != configdata.end())        NIMGS_CAL = std::atoi(configdata["NIMGS_CAL"].c_str());   
     //if (configdata.find("NCAL_RESUME") != configdata.end())      n_calib = std::atoi(configdata["NCAL_RESUME"].c_str());    
     if (configdata.find("FRAMES_FOLDER") != configdata.end())    FRAMES_FOLDER = std::string( configdata["FRAMES_FOLDER"] );    
     if (configdata.find("CNN_FILE") != configdata.end())         CNN_FILE = std::string( configdata["CNN_FILE"] ); 
     if (configdata.find("CNN_SIZE") != configdata.end())         CNN_SIZE = std::atoi(configdata["CNN_SIZE"].c_str());
     if (configdata.find("LABEL_FILE") != configdata.end())       LABEL_FILE = std::string( configdata["LABEL_FILE"] ); 
     if (configdata.find("TARGET_LABELS") != configdata.end()) {  
            TARGET_LABELS.clear();
            std::vector<string> target_l = split( configdata["TARGET_LABELS"], " ");
            for (int i = 0; i < target_l.size(); i++) 
                  TARGET_LABELS.push_back( std::atoi(target_l[i].c_str()) );
       }    
     if (configdata.find("CAMERA_NAME") != configdata.end())       CAMERA_NAME = std::string( configdata["CAMERA_NAME"] ); 
     if (configdata.find("DST_EMAIL") != configdata.end())         DST_EMAIL = std::string( configdata["DST_EMAIL"] );     
     
    return false;
}


std::vector<int>  CNN_INFERENCE(std::string dt_s ){
    /* RUN COMMAND FOR CNN INFERENCE FOR FRAMES TAKEN AT dt_s TIMESTAMP*/
    
    std::string cnn_folder = FRAMES_FOLDER+ " "+ "'"+dt_s+"*'";
    if (!TEST_IMG_FOLDER.empty())  cnn_folder = TEST_IMG_FOLDER;
    
    std::string command = "./apps/opencv_classification " +CNN_FILE+ " '' "
                + cnn_folder + 
                " "+MAX_CNN_IMAGES+" "+std::to_string(CNN_SIZE)+" "+LABEL_FILE+" 1>>"+OUTPUT_FOLDER+OUTPUT_FILE;//+dt_s+OUTPUT_FILE;
    if (verbose) std::cout << command.c_str() << std::endl;

    double start = time_time();
    execCommand(command.c_str());
    if (verbose) std::cout << "CNN time " << 1000*(time_time()-start ) << std::endl;
       
    // get result
    std::string last_line = getFileLastLine(OUTPUT_FOLDER+OUTPUT_FILE);//dt_s+OUTPUT_FILE);
    if (last_line == "Err") { // error handling
        std::vector<int> result_vector{0};
        return result_vector;
    }
    std::vector<string> result_vector_string = split(last_line, " ");
 
    std::vector<int> result_vector;
    for (int i = 0; i < result_vector_string.size()-1; i++) {  // last element is a 0
        result_vector.push_back( std::atoi((result_vector_string[i]).c_str()) );
        //std::cout << result_vector[i] << std::endl;
    }
     
    return result_vector;
}


std::vector<int> OPENCV_TFLITE_CNN_INFERENCE(std::vector<std::string> classes, std::string dt_s){
    /* RUN COMMAND FOR CNN INFERENCE FOR FRAMES TAKEN AT dt_s TIMESTAMP*/
   
    std::string pattern = dt_s+"*";
    if (verbose) std::cout << "Performing inference over images in " << FRAMES_FOLDER << " with format: " << pattern << std::endl; // " << READ_FPS*POST_CAPTURE << "
    double start = time_time();
    modelObj.B_execute_inference(FRAMES_FOLDER, pattern, classes, 1000, CNN_SIZE); // In general number of image will be READ_FPS*POST_CAPTURE. But not always (e.g., checking re-calibration). Set Nmax=1000 
    
    if (verbose) std::cout << "CNN time " << 1000*(time_time()-start ) << std::endl;
       
    // print result
    if (verbose) for (int i = 0; i < modelObj.result_vector.size(); i++) {  
        std::cout << modelObj.result_vector[i] << std::endl;
    }
     
    return modelObj.result_vector;
}



/* Capture frames:
 * two alternatives: CaptureFrames (from opencv) or Capture_Raspistill (from raspstill RPi tool)
 */
std::string CaptureFrames(VideoCapture cap, int N, std::string FRAMES_FOLDER="./") {
    /* Read N frames from camera and save them into FRAMES_FOLDER, with a timestamp 
     * Return: prefix name for the images (for future classification) */
    
     std::string dt_s = getTimeString();
     double start = time_time();
     bool bSuccess;  
     Mat frame;
     for (int n = 0; n < N; n++) {
          bSuccess = cap.read( frame ); // read a new frame from video
          if (!bSuccess) { std::cout << BOLDYELLOW << "[ERROR] Reading frames" << RESET << std::endl; return "ERR"; }
          cv::imwrite(FRAMES_FOLDER+dt_s+"_"+"img_"+std::to_string(n)+".jpg", frame);
          if (verbose) std::cout << FRAMES_FOLDER+dt_s+"_"+std::to_string(n)+".jpg" << std::endl;
      }
     if (verbose) std::cout << "capture time " << 1000*(time_time()-start ) << std::endl;
     //std::cout << "[INFO] ("+dt_s+") Recorded " << N << " frames" << std::endl;
     return dt_s;
}

std::string Capture_Raspistill(int post_capture, std::string FRAMES_FOLDER="./"){
    /* Read frames from camera during post_capture seconds, and save them into FRAMES_FOLDER, with a timestamp 
     * Return: prefix name for the images (for future classification) */
    
    std::string dt_s = getTimeString();
    double start = time_time();
    //   If a time-lapse value of 0 is entered (-tl 0), the application will take pictures as fast as possible
    std::string command_raspistill = "raspistill --nopreview -w "+std::to_string(WIDTH)+" -h "+std::to_string(HEIGHT)+" -t "+std::to_string(1000*post_capture)+" -tl "+std::to_string(0)+" -o "+FRAMES_FOLDER+dt_s+"_"+"img%04d.jpg";
    if (verbose) {
        std::cout << "capturing " << post_capture << "seconds at maximum raspistill speed (-t TIME -t0 0) " << std::endl;
        std::cout << command_raspistill.c_str() << std::endl; }
    execCommand(command_raspistill.c_str());
    if (verbose) std::cout << "capture time " << 1000*(time_time()-start ) << std::endl;
    std::cout << "[INFO] ("+dt_s+") Recorded frames (" << post_capture <<  " sec)" << std::endl;
    
    return dt_s;
}

std::string CaptureFrames_noSensor(VideoCapture cap, int N, std::string FRAMES_FOLDER="./"){
    /* Read N frames from camera (if the PIR is not activated) and save them into FRAMES_FOLDER, with a timestamp 
     * Return: prefix name for the images (for future classification) */
     
     std::string dt_s = getTimeString();
     bool bSuccess;  
     Mat frame;
     for (int n = 0; n < N; n++) {
          while (gpioRead(INPUT_PIN)) {
               std::cout << "PIR detection...waiting..." << std::endl; 
               time_sleep(1);
          }
          bSuccess = cap.read( frame ); // read a new frame from video
          if (!bSuccess) { std::cout << BOLDYELLOW << "[ERROR] Reading frames" << RESET << std::endl; return "ERR"; }
          cv::imwrite(FRAMES_FOLDER+dt_s+"_"+"img_"+std::to_string(n)+".jpg", frame);
          if (verbose) std::cout << FRAMES_FOLDER+dt_s+"_"+std::to_string(n)+".jpg" << std::endl;
      }
     return dt_s;
}  


std::string checkIP(){
    /* Check if there is IP, or return '' otherwise */
    std::string IP = execCommand("hostname -I"); 
    return split(IP," ")[0]; // in case it returns more than one value
    }
    
    
int publishMQTT(std::string IP, std::string topic,  std::string msg, std::string user, std::string passwd){
    /* Publish MQTT message. It requires mosquitto (broker) service active */
    
    std::string command_pub = "mosquitto_pub -d -h "+IP+" -u "+user+" -P "+passwd+" -t "+topic+" -m '"+msg+"'";
    if (verbose) std::cout << "MQTT COMMAND: " << command_pub.c_str() << std::endl;
    std::string result = execCommand(command_pub.c_str());
    //std::cout << "[INFO] MQTT sent" << std::endl;
    if (result=="1") { // disable MQTT in case of error (failure in connection)
        MQTT = 0;
        std::cout << "[Info] Disabled MQTT messages [MQTT=0]" << std::endl;
    }
    return 0;
}


//https://stackoverflow.com/questions/30078756/super-fast-median-of-matrix-in-opencv-as-fast-as-matlab
double medianMat(cv::Mat img){
    cvtColor(img, img, cv::COLOR_BGR2GRAY);//CV_BGR2GRAY);
    img= img.reshape(0,1); // spread Input Mat to single row
    std::vector<double> vecFromMat;
    img.copyTo(vecFromMat); // Copy Input Mat to vector vecFromMat
    std::nth_element(vecFromMat.begin(), vecFromMat.begin() + vecFromMat.size() / 2, vecFromMat.end());
    return vecFromMat[vecFromMat.size() / 2];
}
int Check_Night(VideoCapture cap, int median_T = 10, int std_T = 20) {
    /* Read a frame from camera and check if it is night */
    
     std::string dt_s = getTimeString();
     bool bSuccess; 
     int night = 0; 
     Mat frame;
     bSuccess = cap.read( frame ); // read a new frame from video
     if (!bSuccess) { std::cout << BOLDYELLOW << "[ERROR] Reading frame" << RESET << std::endl; return -1; }
     
     cv::Scalar mean_img, std_img;
     cv::meanStdDev(frame, mean_img, std_img);
     if (verbose) std::cout << "Night??  std: " << std_img[0] << ",  median: " << medianMat(frame) << std::endl;
     if (medianMat(frame) < median_T && std_img[0] < std_T) {
            night = 1; 
            std::cout << "[INFO] ("<< getTimeString() << ") Night detected... ";
            if (MQTT) publishMQTT(hostIP, "info", "Night detected."+getTimeString(), "pi", "pipasswd");
            cv::imwrite(FRAMES_tmp_FOLDER+dt_s+"_NIGHT"+".jpg", frame); 
            if (verbose) std::cout << FRAMES_tmp_FOLDER+dt_s+"_NIGHT"+".jpg" << std::endl; 
            for (int n = 0; n < 10; n++) {  // following readings are also black
                bSuccess = cap.read( frame );
                //cv::imwrite(FRAMES_tmp_FOLDER+"Night_"+dt_s+"_"+std::to_string(n)+".jpg", frame); std::cout << "  saved " << "Night_"+dt_s+"_"+std::to_string(n)+".jpg" << std::endl;
            }                
    }
     if (verbose and night) std::cout << dt_s+" --> NIGHT" << std::endl;
     return night;
}


int sendEmail(std::string dst_addr, std::string subject, std::string body, std::string attachment){
    /* Send email. It requires previous e-mail configuration */
    
    double start = time_time();
    std::string command_email = "echo '"+body+"' | mutt -s '"+subject+"' "+dst_addr+" -a "+attachment;
    if (verbose) std::cout << "EMAIL COMMAND: " << command_email.c_str() << std::endl;
    std::string result = execCommand(command_email.c_str());
    if (verbose) std::cout << "email time " << 1000*(time_time()-start ) << std::endl;
    std::cout << "[INFO] E-mail was sent!" << std::endl;

    return 0;
}

int check_and_sendAlarm(std::vector<std::string> classes, std::vector<int> result_classes, std::string date_string){
    /* Check if there is alarm trigger (classifications in result_classes belong to TARGET_LABELS), and send corresponding alarm e-mail */
    bool alarm = false;               // send alarm
    std::string alarm_folders;        // images to send
    std::string alarm_classes = "- ";        // classes detected
    //alarm_folders.clear();         alarm_classes.clear();
    
    if (!TEST_IMG_FOLDER.empty()) date_string = "";
    
    for (auto i : TARGET_LABELS)  { // send alarm??
        if (i < 1 || i > classes.size() || i > result_classes.size()) continue;
            if (result_classes[i-1] ){
                alarm = true;
                alarm_folders += OUTPUT_FOLDER+classes[i-1]+"/"+ date_string + "* ";
                alarm_classes += classes[i-1]+" - ";
            }
        }

    if (alarm){  // send alarm
        if (DST_EMAIL == "LOCAL")
            std::cout << "[INFO] ["+CAMERA_NAME+"] Alarm triggered by images classified as: " << std::endl << alarm_classes+". ("+date_string+")" 
                          << " -- Check folders: " << alarm_folders << std::endl;
        else  {     
            std::cout << "[INFO] SENDING ALARM concerning " << alarm_classes << std::endl;
            sendEmail(DST_EMAIL, "["+CAMERA_NAME+"] Alarm triggered", 
                                     "Please, find attached images classified as: "+alarm_classes+". ("+date_string+")", 
                                     alarm_folders);
            }
        if (MQTT) publishMQTT(hostIP, "info", "Alarm: "+alarm_classes, "pi", "pipasswd");
        }
    else
        std::cout << "[INFO] NOT sending alarm " << std::endl;
    return 0;    
}


void CALIBRATE(std::string config){

    std::cout << "[INFO] Starting calibration #" << n_calib << "... " << std::endl;
    std::string command;
    command = "sudo -u pi python3 ./ondevice_training/A_learning.py --config "+config+" --configfile training_config.ini >> "+OUTPUT_FOLDER+"training_LOG 2>&1";
        
    if (verbose) std::cout << command.c_str() << std::endl;
    double start = time_time();
    std::string result = execCommand(command.c_str()); //std::cout << resultado << std::endl;
    if (verbose) std::cout << "Calib time (min): " << (time_time()-start )/60.0 << std::endl;
    std::cout << "[INFO] ("+getTimeString()+") ...calibration #" << n_calib << " finished. " << std::endl;   
    }
    
void create_dataset(VideoCapture* Camera) {

    /* (1) Create dataset folder for last experience (only if there are enough images) */
    
    // *) Close camera before launching script (script may need to take more images and conflict would happen)
    if (Camera->isOpened())  Camera->release(); 
    
    std::cout << "[INFO] Moving data to new folder ("+std::to_string(n_calib)+") -- and maybe taking additional data --" << std::endl; 
    //std::string command = "./scripts/create_new_img_folder.sh "+std::to_string(NIMGS_CAL)+" "+std::to_string(n_calib)+" "+std::to_string(INPUT_PIN)+" "+std::to_string(n_calib-4);
    std::string command = "./scripts/create_new_img_folder.sh "+std::to_string(NIMGS_CAL)+" "+std::to_string(n_calib)+" "+std::to_string(INPUT_PIN);//+" "+std::to_string(n_calib-4); // cambiado para tests.
    if (verbose) std::cout << command.c_str() << std::endl;
    std::string result = execCommand(command.c_str(), 1024, true); 
  
    //error = std::atoi(result.substr(0,1).c_str());   if (error)  std::cout << "[WARNING] Not enough images. << std::endl;  
    
    Camera->open(0); // *) re-open camera and set parameters
	Camera->set(CAP_PROP_FRAME_WIDTH, WIDTH);
    Camera->set(CAP_PROP_FRAME_HEIGHT, HEIGHT);
	Camera->set(CAP_PROP_FPS, READ_FPS);
    
    // *) Script may have accessed to GPIO INPUT_PIN, so let's reconfigure it again: 
    gpioSetMode(INPUT_PIN, PI_INPUT);
    gpioSetPullUpDown(INPUT_PIN, PI_PUD_DOWN); // Sets a pull-down.
    
    
    /* (2) Create training dataset from various folders */
    std::cout << "[INFO] Copying random data from last folders (beginning from "+std::to_string(n_calib)+") to training folder" << std::endl;     
    command = "python scripts/Copy_imgs_to_calibrate.py --N "+std::to_string(NIMGS_CAL);
    if (verbose) std::cout << command.c_str() << std::endl;
    result = execCommand(command.c_str(), 1024, true); 
    }
    
bool check(std::vector<std::string> classes, std::vector<int> result_classes, std::string date_string, float PERC=0.75){
    /* Check if there are more than PERC % classifications in result_classes which belong to TARGET_LABELS */
    bool alarm = false;               // alarm
    int  n = 0;                       // number of classifications within TARGET_LABELS
    std::string alarm_classes = "- ";        // classes detected

    if (!TEST_IMG_FOLDER.empty()) date_string = "";
    
    for (auto i : TARGET_LABELS)  { // alarm??
        if (i < 1 || i > classes.size() || i > result_classes.size()) continue;
            if (result_classes[i-1] ){
                n += result_classes[i-1];  // number of classifications from TARGET_LABEL i 
                alarm_classes += classes[i-1]+" - "; // associated class
            }
        }

    std::cout << " > Found " << n << " relevant frames (out of " << std::accumulate(result_classes.begin(),result_classes.end(),0) 
              << " captured frames) -- limit: " << std::accumulate(result_classes.begin(),result_classes.end(),0)*PERC << " -- " << std::endl;
    if (n >= std::accumulate(result_classes.begin(),result_classes.end(),0)*PERC)
        alarm = true;
        
    if (alarm) {  // alarm -> calibrate
            n_calib++;
            std::cout << "[INFO] ("+date_string+") Calibration #" << n_calib << " triggered by images classified as: " << std::endl << alarm_classes+"." << std::endl;
            CALIB_PERIOD_red = 1;
            // if alarm==true (return true): NEED TO call to create_dataset() and CALIBRATE
    }
    else {
            std::cout << "[INFO] ("+date_string+") NOT calibration is needed." << std::endl;
    }

    return alarm;    
}


int filterSignal(float DETECTION_PERIOD){
   /* Filter a signal. 
    * If status changes to LOW in a shorter period than PERIOD, then it is considered noise */
   double startT = time_time();
   int read = 1; 
   //std::string dt = getTimeString(); std::cout << dt << std::endl;
   
   while (read && (time_time() - startT) < DETECTION_PERIOD) {
       time_sleep(0.05);
       read = gpioRead(INPUT_PIN);
       if (verbose) printf("%d \n", read); 
       //dt = getTimeString(); std::cout << dt << std::endl;
       //printf("%f \n",time_time());
      }
   //if (!read) printf(" --> %d\n", read); 
   //dt = getTimeString(); std::cout << dt << std::endl;
   CONFIRM_GAP = 0;
   return read;
   }
   
