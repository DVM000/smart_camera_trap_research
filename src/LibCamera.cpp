// Original file from https://github.com/edward-ardu/libcamera-cpp-demo.git
// Included additional functions.

#include "LibCamera.h"

using namespace std::placeholders;

int LibCamera::initCamera() {
    int ret;
    cm = std::make_unique<CameraManager>();
    ret = cm->start();
    if (ret){
        std::cout << "Failed to start camera manager: "
              << ret << std::endl;
        return ret;
    }
    cameraId = cm->cameras()[0]->id();
    camera_ = cm->get(cameraId);
    if (!camera_) {
        std::cerr << "Camera " << cameraId << " not found" << std::endl;
        return 1;
    }

    if (camera_->acquire()) {
        std::cerr << "Failed to acquire camera " << cameraId
              << std::endl;
        return 1;
    }
    camera_acquired_ = true;
    return 0;
}

char * LibCamera::getCameraId(){
    return cameraId.data();
}

void LibCamera::configureStill(int width, int height, PixelFormat format, int buffercount, int rotation) {
    bool msg=false;
    if (msg) printf("Configuring still capture...\n");
    config_ = camera_->generateConfiguration({ StreamRole::StillCapture });
    if (width && height) {
        libcamera::Size size(width, height);
        config_->at(0).size = size;
    }
    config_->at(0).pixelFormat = format;
    if (buffercount)
        config_->at(0).bufferCount = buffercount;
    /*Transform transform = Transform::Identity;
    bool ok;
    Transform rot = transformFromRotation(rotation, &ok);
    if (!ok)
        throw std::runtime_error("illegal rotation value, Please use 0 or 180");
    transform = rot * transform;
    if (!!(transform & Transform::Transpose))
        throw std::runtime_error("transforms requiring transpose not supported");
    //config_->transform = transform;*/

    CameraConfiguration::Status validation = config_->validate();
	if (validation == CameraConfiguration::Invalid)
		throw std::runtime_error("failed to valid stream configurations");
	else if (validation == CameraConfiguration::Adjusted)
        if (msg) std::cout << "Stream configuration adjusted" << std::endl;

    if (msg) printf("Still capture setup complete\n");
}

int LibCamera::startCamera() {
    int ret;
    ret = camera_->configure(config_.get());
    if (ret < 0) {
        std::cout << "Failed to configure camera" << std::endl;
        return ret;
    }

    camera_->requestCompleted.connect(this, &LibCamera::requestComplete);

    allocator_ = std::make_unique<FrameBufferAllocator>(camera_);

    return startCapture();
}

int LibCamera::startCapture() {
    int ret;
    unsigned int nbuffers = UINT_MAX;
    for (StreamConfiguration &cfg : *config_) {
        ret = allocator_->allocate(cfg.stream());
        if (ret < 0) {
            std::cerr << "Can't allocate buffers" << std::endl;
            return -ENOMEM;
        }

        unsigned int allocated = allocator_->buffers(cfg.stream()).size();
        nbuffers = std::min(nbuffers, allocated);
    }

    for (unsigned int i = 0; i < nbuffers; i++) {
        std::unique_ptr<Request> request = camera_->createRequest();
        if (!request) {
            std::cerr << "Can't create request" << std::endl;
            return -ENOMEM;
        }

        for (StreamConfiguration &cfg : *config_) {
            Stream *stream = cfg.stream();
            const std::vector<std::unique_ptr<FrameBuffer>> &buffers =
                allocator_->buffers(stream);
            const std::unique_ptr<FrameBuffer> &buffer = buffers[i];

            ret = request->addBuffer(stream, buffer.get());
            if (ret < 0) {
                std::cerr << "Can't set buffer for request"
                      << std::endl;
                return ret;
            }
            for (const FrameBuffer::Plane &plane : buffer->planes()) {
                void *memory = mmap(NULL, plane.length, PROT_READ, MAP_SHARED,
                            plane.fd.get(), 0);
                mappedBuffers_[plane.fd.get()] =
                    std::make_pair(memory, plane.length);
            }
        }

        requests_.push_back(std::move(request));
    }

    ret = camera_->start(&this->controls_);
    // ret = camera_->start();
    if (ret) {
        std::cout << "Failed to start capture" << std::endl;
        return ret;
    }
    controls_.clear();
    camera_started_ = true;
    for (std::unique_ptr<Request> &request : requests_) {
        ret = queueRequest(request.get());
        if (ret < 0) {
            std::cerr << "Can't queue request" << std::endl;
            camera_->stop();
            return ret;
        }
    }
    viewfinder_stream_ = config_->at(0).stream();
    return 0;
}

void LibCamera::StreamDimensions(Stream const *stream, uint32_t *w, uint32_t *h, uint32_t *stride) const
{
	StreamConfiguration const &cfg = stream->configuration();
	if (w)
		*w = cfg.size.width;
	if (h)
		*h = cfg.size.height;
	if (stride)
		*stride = cfg.stride;
}

Stream *LibCamera::VideoStream(uint32_t *w, uint32_t *h, uint32_t *stride) const
{
	StreamDimensions(viewfinder_stream_, w, h, stride);
	return viewfinder_stream_;
}

int LibCamera::queueRequest(Request *request) {
    std::lock_guard<std::mutex> stop_lock(camera_stop_mutex_);
    if (!camera_started_)
        return -1;
    {
        std::lock_guard<std::mutex> lock(control_mutex_);
        request->controls() = std::move(controls_);
    }
    return camera_->queueRequest(request);
}

void LibCamera::requestComplete(Request *request) {
    if (request->status() == Request::RequestCancelled)
        return;
    processRequest(request);
}

void LibCamera::processRequest(Request *request) {
    requestQueue.push(request);
}

void LibCamera::returnFrameBuffer(LibcameraOutData frameData) {
    uint64_t request = frameData.request;
    Request * req = (Request *)request;
    req->reuse(Request::ReuseBuffers);
    queueRequest(req);
}

bool LibCamera::readFrame(LibcameraOutData *frameData){
    std::lock_guard<std::mutex> lock(free_requests_mutex_);
    // int w, h, stride;
    if (!requestQueue.empty()){
        Request *request = this->requestQueue.front();

        const Request::BufferMap &buffers = request->buffers();
        for (auto it = buffers.begin(); it != buffers.end(); ++it) {
            FrameBuffer *buffer = it->second;
            for (unsigned int i = 0; i < buffer->planes().size(); ++i) {
                const FrameBuffer::Plane &plane = buffer->planes()[i];
                const FrameMetadata::Plane &meta = buffer->metadata().planes()[i];
                
                void *data = mappedBuffers_[plane.fd.get()].first;
                int length = std::min(meta.bytesused, plane.length);

                frameData->size = length;
                frameData->imageData = (uint8_t *)data;
            }
        }
        this->requestQueue.pop();
        frameData->request = (uint64_t)request;
        return true;
    } else {
        Request *request = nullptr;
        frameData->request = (uint64_t)request;
        return false;
    }
}

void LibCamera::set(ControlList controls){
    std::lock_guard<std::mutex> lock(control_mutex_);
	this->controls_ = std::move(controls);
}

int LibCamera::resetCamera(int width, int height, PixelFormat format, int buffercount, int rotation) {
    stopCamera();
    configureStill(width, height, format, buffercount, rotation);
    return startCamera();
}

void LibCamera::stopCamera() {
    if (camera_){
        {
            std::lock_guard<std::mutex> lock(camera_stop_mutex_);
            if (camera_started_){
                if (camera_->stop())
                    throw std::runtime_error("failed to stop camera");
                camera_started_ = false;
            }
        }
        camera_->requestCompleted.disconnect(this, &LibCamera::requestComplete);
    }
    while (!requestQueue.empty())
        requestQueue.pop();

    for (auto &iter : mappedBuffers_)
	{
        std::pair<void *, unsigned int> pair_ = iter.second;
		munmap(std::get<0>(pair_), std::get<1>(pair_));
	}

    mappedBuffers_.clear();

    requests_.clear();

    allocator_.reset();

    controls_.clear();
}

void LibCamera::closeCamera(){
    if (camera_acquired_)
        camera_->release();
    camera_acquired_ = false;

    camera_.reset();

    cm.reset();
}


/* ------------------------------------------------------------------ */
/* DVM: Functions for the application */

int openLibcamera(LibCamera *cam, uint32_t* pW, uint32_t* pH, uint32_t* pstride, int READ_FPS) {
    
    // Initialize camera
    int ret = cam->initCamera();
    if (ret){
        std::cerr << BOLDYELLOW << "[ERROR] Cannot initialize camera" << RESET << std::endl;
        return 1;
        }
    else
        std::cout << "[INFO] Camera open" << std::endl;        
    // Configure H, W, FPS
 
    cam->configureStill(*pW, *pH, formats::RGB888, 1, 0);
    ControlList controls_;
    int64_t frame_time = 1000000 / READ_FPS;
	controls_.set(controls::FrameDurationLimits, libcamera::Span<const int64_t, 2>({ frame_time, frame_time })); // set Frame-rate
    cam->set(controls_);
    // Start camera
    cam->startCamera();
    cam->VideoStream(pW, pH, pstride);

    return 0;
    }
    
int closeLibcamera(LibCamera *cam){
    cam->stopCamera();
    cam->closeCamera();
    return 0;
    }


int flushFrames(LibCamera *cam, int flushCount = 5) {
    // This function clears out oldframes, ensuring the buffer and queue are synchronized with the current scene.
    // libcamera library maintains a pool of buffers, old frames may remain in these buffers.
    // 
    LibcameraOutData frameData;
    int flushed = 0;
    int retryCount = 0;
    const int maxRetries = 2000; // maximum 2 seconds

    while (flushed < flushCount) {
        if (cam->readFrame(&frameData)) {
            cam->returnFrameBuffer(frameData); // Solution: frames are returned back to the buffer pool (discarded)
            flushed++;
        } 
        // Important: if no frames are immediately available, wait for a small duration
        else {
            retryCount++;
            if (retryCount >= maxRetries) {
                std::cerr << BOLDYELLOW << "[WARNING] Unable to flush frames after retries" << RESET << std::endl;
                return -1; // Failure to flush frames
            }
            time_sleep(0.01); // Wait for frames
        }
    }
    return 0; // Success
}

std::string CaptureFrames_libcamera(LibCamera *cam, int N, int W, int H, uint32_t stride, std::string FRAMES_FOLDER="./") {
    /* Read N frames from camera and save them into FRAMES_FOLDER, with a timestamp 
     * Return: prefix name for the images (for future classification) */
    
    std::string dt_s = getTimeString();
    double start = time_time();
    
    LibcameraOutData frameData;
    bool bSuccess; 
    int n = 0;
    flushFrames(cam); // ensure no old frames are returned from buffers
    while (n < N) {
          bSuccess = cam->readFrame( &frameData ); // read a new frame from video
          if (!bSuccess) continue; //{ std::cout << "[INFO] not read " << n << std::endl; continue; }
          Mat frame(H, W, CV_8UC3, frameData.imageData, stride);
          cv::imwrite(FRAMES_FOLDER+dt_s+"_"+"img_"+cv::format("%02d", n)+".jpg", frame);
          if (verbose) std::cout << FRAMES_FOLDER+dt_s+"_"+std::to_string(n)+".jpg" << std::endl;
          cam->returnFrameBuffer(frameData);
          n++;
      }
      
     if (verbose) std::cout << "capture time " << 1000*(time_time()-start ) << std::endl;
     //std::cout << "[INFO] ("+dt_s+") Recorded " << N << " frames" << std::endl;
     if (verbose) std::cerr << "Read " << N << " frames " << dt_s <<"_img_nn.jpg" << std::endl;
     return dt_s;
}


std::string CaptureFrames_noSensor_libcamera(LibCamera *cam, int N, int W, int H, uint32_t stride, std::string FRAMES_FOLDER="./"){
    /* Read N frames from camera (if the PIR is not activated) and save them into FRAMES_FOLDER, with a timestamp 
     * Return: prefix name for the images (for future classification) */
         
     std::string dt_s = getTimeString();
     LibcameraOutData frameData;
     bool bSuccess;  
     int n = 0;
     flushFrames(cam); // ensure no old frames are returned from buffers
     while (n < N) {
          while (gpioRead(INPUT_PIN)) {
               std::cout << "PIR detection...waiting..." << std::endl; 
               time_sleep(1);
          }
          bSuccess = cam->readFrame( &frameData ); // read a new frame from video
          if (!bSuccess) continue; //{ std::cout << "[INFO] not read " << n << std::endl; continue; }
          Mat frame(H, W, CV_8UC3, frameData.imageData, stride);
          cv::imwrite(FRAMES_FOLDER+dt_s+"_img_" + cv::format("%02d", n) + ".jpg", frame);
          if (verbose) std::cout << FRAMES_FOLDER+dt_s+"_"+std::to_string(n)+".jpg" << std::endl;
          cam->returnFrameBuffer(frameData);
          n++;
      }
     return dt_s;  
}

int Check_Night_libcamera(LibCamera *cam, int W, int H, uint32_t stride, bool last_night = false, int median_T = 10, int std_T = 20) {
    /* Read a frame from camera and check if it is night */
    // last_night = true indicates that last time we checked night status, it was confirmed
     std::string dt_s = getTimeString();
     LibcameraOutData frameData;
     bool bSuccess = false; 
     Mat frame;
     int night = 0; 
     int n = 0; 
     flushFrames(cam); // ensure no old frames are returned from buffers
     if (last_night){        // if last read frame was black, following read frames may be white/black
            while (n < 50) { // following readings are white
                bSuccess = cam->readFrame( &frameData ); // read a new frame from video
                if (!bSuccess) continue; //{ std::cout << "[INFO] not read " << n << std::endl; continue; }
                //Mat frame(H, W, CV_8UC3, frameData.imageData, stride);
                //cv::imwrite(FRAMES_tmp_FOLDER+"Night_"+dt_s+"_"+std::to_string(n)+".jpg", frame); std::cout << "  saved " << "Night_"+dt_s+"_"+std::to_string(n)+".jpg" << std::endl;
                cam->returnFrameBuffer(frameData);
                n++;
            }
         }
     n = 0;
     while (n < 1) {
          bSuccess = cam->readFrame( &frameData ); // read a new frame from video
          if (!bSuccess) { continue; }
          frame = cv::Mat(H, W, CV_8UC3, frameData.imageData, stride);
          //cv::imwrite(FRAMES_tmp_FOLDER+dt_s+"_isitNIGHT"+".jpg", frame); 
          cam->returnFrameBuffer(frameData);
          n++;
      } 
     if (!bSuccess) { std::cout << BOLDYELLOW << "[ERROR] Reading frame" << RESET << std::endl; return -1; }
     
     cv::Scalar mean_img, std_img;
     cv::meanStdDev(frame, mean_img, std_img);
     if (verbose) std::cout << "Night??  std: " << std_img[0] << ",  median: " << medianMat(frame) << std::endl;
     if (medianMat(frame) < median_T && std_img[0] < std_T) {
            night = 1; 
            std::cout << "[INFO] ("<< getTimeString() << ") Night detected... ";
            if (MQTT) publishMQTT(hostIP, "camera", "Night detected."+getTimeString(), "pi", "pipasswd");
            cv::imwrite(FRAMES_tmp_FOLDER+dt_s+"_NIGHT"+".jpg", frame); 
            if (verbose) std::cout << FRAMES_tmp_FOLDER+dt_s+"_NIGHT"+".jpg" << std::endl; 
     }
     
     if (verbose and night) std::cout << dt_s+" --> NIGHT" << std::endl;
     return night;
}

void create_dataset_libcamera(LibCamera *cam, uint32_t* pstride) {

    /* (1) Create dataset folder for last experience (only if there are enough images) */
    
    // *) Close camera before launching script (script may need to take more images and conflict would happen)
    closeLibcamera(cam);
    
    std::cout << "[INFO] Moving data to new folder ("+std::to_string(n_calib)+") -- and maybe taking additional data --" << std::endl; 
    //std::string command = "./scripts/create_new_img_folder.sh "+std::to_string(NIMGS_CAL)+" "+std::to_string(n_calib)+" "+std::to_string(INPUT_PIN)+" "+std::to_string(n_calib-4);
    std::string command = "./scripts/create_new_img_folder.sh "+std::to_string(NIMGS_CAL)+" "+std::to_string(n_calib)+" "+std::to_string(INPUT_PIN);//+" "+std::to_string(n_calib-4); // cambiado para tests.
    if (verbose) std::cout << command.c_str() << std::endl;
    std::string result = execCommand(command.c_str(), 1024, true); 
    //std::cout << result << std::endl;   
    //error = std::atoi(result.substr(0,1).c_str());   if (error)  std::cout << "[WARNING] Not enough images. << std::endl;  
    
    // *) re-open camera and set parameters
    uint32_t W = (uint32_t) WIDTH; 
    uint32_t H = (uint32_t) HEIGHT;
    openLibcamera(cam, &W, &H, pstride, READ_FPS);

    // *) Script may have accessed to GPIO INPUT_PIN, so let's reconfigure it again: 
    gpioSetMode(INPUT_PIN, PI_INPUT);
    gpioSetPullUpDown(INPUT_PIN, PI_PUD_DOWN); // Sets a pull-down.
    
    /* (2) Create training dataset from various folders */
    std::cout << "[INFO] Copying random data from last folders (beginning from "+std::to_string(n_calib)+") to training folder" << std::endl;     
    command = "python scripts/Copy_imgs_to_calibrate.py --N "+std::to_string(NIMGS_CAL);
    if (verbose) std::cout << command.c_str() << std::endl;
    result = execCommand(command.c_str(), 1024, true); 
    }
    

