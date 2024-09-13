/*
 * READING PIR SENSOR + RUN INFERENCE + SET ALARM
 * 
 */

#include "common_functions.cpp"

/* APPLICATION VARIABLES */
int readValue = 0;
bool detected_once = false;
bool detected = false;


/* ........................................................
 * MAIN
 * ........................................................
 * */
int main(int argc, char *argv[])
{
    
    int error;
    double start_PIR;                  // App starting time
    double start_det_once;             // detection (to be confirmed)
    double last_calib = time_time();   // last calibration time
    bool recalib;
    bool run_always = false; 
    
    std::string date_string;          // detection timestamp
    std::vector<std::string> classes; // labels from file
    std::vector<int> result_classes;  // classification results
        
    /* ------------------------------------------------------------------
    *  READ PARAMETERS FROM CONFIGUATION FILE
    * -----------------------------------------------------------------*/
   readConfiguration(CONFIG_FILE);
   std::cout << "[INFO] Starting date: " + getTimeString() << std::endl;
   
   // Check MQTT
    if (MQTT) {
        hostIP = checkIP();
        if (strcmp(hostIP.c_str(),"")==0) {
            std::cout << "[INFO] NO Internet connection available." << std::endl;
            MQTT = 0;
            }
        else 
            std::cout << "[INFO] Internet connection available (" << hostIP << "). Check MQTT messages on 'info' channel." << std::endl;
    } 

   if (MQTT) publishMQTT(hostIP, "info", "Start. "+getTimeString(), "pi", "pipasswd");
   
    /* ------------------------------------------------------------------
    *  OPEN THE CAMERA, ready for detections
    * -----------------------------------------------------------------*/
    double start = time_time();
    VideoCapture Camera(0);
	//set camera params
	Camera.set(CAP_PROP_FRAME_WIDTH, WIDTH);
    Camera.set(CAP_PROP_FRAME_HEIGHT, HEIGHT);
	Camera.set(CAP_PROP_FPS, READ_FPS);	

    if (verbose) std::cout << "Time to open the camera: " << 1000.0*(time_time()-start) << std::endl;
    //Open camera
	if (!Camera.isOpened()) { std::cout << BOLDYELLOW << "[ERROR] Cannot open the camera" << RESET << std::endl;  return 1; }  
    else {  std::cout << "[INFO] Camera Open" << std::endl;   }

        
    /* ------------------------------------------------------------------
    *  1st CALIBRATION
    * -----------------------------------------------------------------*/
    // done before launching this app
    /* ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/
    // load CNN
    modelObj.A_load_CNN(CNN_FILE, "", "");
    modelObj.outfile = OUTPUT_FOLDER+OUTPUT_FILE;
        
    /* ------------------------------------------------------------------
    *  OPEN GPIO
    * -----------------------------------------------------------------*/
    start = time_time();
    if (gpioInitialise() < 0) { std::cout << BOLDYELLOW << "[ERROR] pigpio initialisation failed" << RESET << std::endl; return 1; }
    if (verbose) std::cout << "Time to open GPIO: " << 1000.0*(time_time()-start) << std::endl;
     
    /* Set GPIO modes */
    gpioSetMode(INPUT_PIN, PI_INPUT);
    gpioSetPullUpDown(INPUT_PIN, PI_PUD_DOWN); // Sets a pull-down.
    if (LED) {
        gpioSetMode(OUTPUT_PIN, PI_OUTPUT);
        gpioWrite(OUTPUT_PIN,0); }//printf("LED-off "); }
    
    /* Read Labels from file */
    error = getFileContent(LABEL_FILE, &classes);

    start_PIR = time_time();
    last_frame = time_time();
    if (!MAX_RUNTIME)   run_always = true; // always running. Ensure loop condition  
    
   std::cout << "[INFO] Started, waiting trigger" << std::endl;
   while ( run_always || (time_time() - start_PIR) < MAX_RUNTIME )
   {
       // read PIR sensor
      readValue = gpioRead(INPUT_PIN);

      /* If DET_PERIOD, filter detection based on DET_PERIOD (ignore CONFIRM_GAP & READ_DELAY_CONFIRM) ------ */
      if (DET_PERIOD > 0 && readValue){       
          readValue = filterSignal( DET_PERIOD ); // positive value if readings are 1 during X seconds (and sets CONFIRM_GAP=0)
      }
      /* -----------------------------------------------*/
      
      // Sensor Detection 
      if (readValue) {
         if (verbose) printf("+++ \n"); 
         
         // without second confirmation 
         if (DET_PERIOD || !CONFIRM_GAP)   
             detected = true; 
             
        // with second confirmation    
         else {  
             if (!detected_once) {        // first detection
                 start_det_once = time_time();
                 detected_once = true;
                 }
             else  {                     // second detection: CONFIRMED
                 detected = true; detected_once = false;   // 
             }
        }
      }  
      // no sensor detection
      else 
         if (verbose) printf(" - \n");  
        
    // Reset detection if CONFIRM_GAP is exceeded
    if (CONFIRM_GAP && ( detected_once && (time_time() - start_det_once > CONFIRM_GAP) ) ){
        detected_once = false;  
        if (LED) { gpioWrite(OUTPUT_PIN,0); }
        if (verbose) std::cout << "Detection NOT confirmed. Re-starting detection" << std::endl; }

    // if confirmed detection:  MAIN OPERATION STAGES
    /* -----------------------------------------------*/
    if ( detected ) {
        if (LED) { gpioWrite(OUTPUT_PIN,1); }   
        start = time_time();
        std::cout << "[INFO] ("<< getTimeString() << ") PIR DETECTION " << std::endl;
        if (MQTT) publishMQTT(hostIP, "info", "PIR detection. "+getTimeString(), "pi", "pipasswd");
        
        if (!LED) gpioTerminate(); // Release resources to reduce power consumption  
        
        // 1- Read frames
        date_string = CaptureFrames(Camera, READ_FPS*POST_CAPTURE, FRAMES_FOLDER); 
            
        // 2- Perform CNN classification
        result_classes = OPENCV_TFLITE_CNN_INFERENCE( classes, date_string );
        if (verbose)  for (auto i : result_classes) std::cout << i << std::endl;

        // 3- Send alarm?
        check_and_sendAlarm(classes, result_classes, date_string);
        
        if (LED) { gpioWrite(OUTPUT_PIN,0); } 
    
        // 4- Sleep for EVENT_GAP seconds
        time_sleep(EVENT_GAP); 
        
        // 5- Re-open GPIO
        if (!LED)    if (gpioInitialise() < 0) { std::cout << BOLDYELLOW << "[ERROR] pigpio initialisation failed" << RESET << std::endl; return 1;  } 
        
        detected = false; 
        std::cout << "[INFO] Resumed, waiting trigger" << std::endl;
      } // end if confirmed detection
      
     // Time interval between readings
      if (!CONFIRM_GAP || (CONFIRM_GAP && !detected_once))  // time interval between readings
          time_sleep(READ_DELAY);  
       else {
           if (!LED) time_sleep(READ_DELAY_CONFIRM); // time interval between readings in order to confirm detection
           if (LED){ 
                   time_sleep(READ_DELAY_CONFIRM/2); 
                   gpioWrite(OUTPUT_PIN,1); //printf("LED-on "); } 
                   time_sleep(READ_DELAY_CONFIRM/2); 
                   gpioWrite(OUTPUT_PIN,0); } //printf("LED-off "); }
            
        }
      
      // Capture image if SEC_TEST time gap is exceeded
      if  (time_time() - last_frame > SEC_TEST)  {
            last_frame = time_time();
            date_string = CaptureFrames(Camera, 1, FRAMES_TEST_FOLDER); 
            //if ( date_string == "ERR") { return 1; } 
            }
            
      // Calibration check based on CALIB_PERIOD
      // if time gap from last calibration (or calibration attempt) exceeds CALIB_PERIOD, CALIBRATE again (if needed)
      if  (time_time() - last_calib > CALIB_PERIOD*60*CALIB_PERIOD_red)  {
            std::cout << "\n[INFO] Checking if calibration is needed... " << std::endl;
            if (MQTT) publishMQTT(hostIP, "info", "Calibration? Checking.", "pi", "pipasswd");
            CALIB_PERIOD_red /=2;
            if (CALIB_PERIOD_red < 1.0/8) {
                 CALIB_PERIOD_red = 1;
                 // Force new calibration: (comment for only restart CALIB_PERIOD)
                 //n_calib++; 
                 //recalib = true;
                 //std::cout << "[INFO] ("+date_string+") Calibration #" << n_calib << " triggered by TIMEOUT." << std::endl;
            }
            else {
                //1- Read frames
                date_string = CaptureFrames_noSensor(Camera, READ_FPS*10, FRAMES_FOLDER); // take images during 10 seconds (only while PIR is off )
                // 2- CNN
                result_classes = OPENCV_TFLITE_CNN_INFERENCE( classes, date_string );
                if (verbose)  for (auto i : result_classes) std::cout << i << std::endl;
                // 3- Calibrate?
                recalib = check(classes, result_classes, date_string);
            }
            if (recalib){
                    if (MQTT) publishMQTT(hostIP, "info", "Starting re-calibration.", "pi", "pipasswd");
                    create_dataset(&Camera);
                    std::cout << "[INFO] Training..." << std::endl;     
                    CALIBRATE("calibrate1");
                    std::cout << "[INFO] Training done." << std::endl; 
                    // load CNN
                    modelObj.A_load_CNN(CNN_FILE, "", ""); 
                }
            last_calib = time_time();
            std::cout << "[INFO] Next calibration in " << CALIB_PERIOD*CALIB_PERIOD_red << " min." << std::endl;
            if (MQTT) publishMQTT(hostIP, "info", "End re-calibration.", "pi", "pipasswd");
            }
        
      // Check night (every 15 min)
      if ((int)(time_time() - start_PIR) % (15*60) == 0 || (int)(time_time() - start_PIR + 2*READ_DELAY ) % (15*60) == 0)  {
          int night = Check_Night(Camera); 
          if ( night == -1) { night = 1; } 
          
          while(night == 1) {                                    // if night detected, sleep 15 min
                std::cout << "Waiting 15 min" << std::endl;
                time_sleep(15*60);
                night = Check_Night(Camera); 
                if ( night == -1) { night = 1; } //return 1; } 
                if ( night == 0) { 
                    std::cout << "[INFO] Resumed after night, waiting trigger" << std::endl;
                    if (MQTT) publishMQTT(hostIP, "info", "End re-calibration.", "pi", "pipasswd");
                }
                last_calib = time_time();
            } 
      }
        
   } // end App

   /* Stop DMA, release resources */
   gpioTerminate();

   /* Stop Camera, release resources */   
   Camera.release();
    
    std::cout << "[INFO] Correctly finished. Bye!" << std::endl;

   return 0;
}
