/*
 * 
 * File for image classification with OpenCV 
 *   a)
 *  - Read CNN in OpenCV
 *   b)
 *  - Read images from folder
 *  - Classify them with CNN 
 *  - Copy classified images into folder (if classes_file provided)
 */


using namespace cv;
using namespace dnn;


//#include <unistd.h>
//#include <stdio.h>

//  -----------------------



class ModelOpenCVClass{
    
    public:
        Net net; // opencv network object
	
	std::vector<int> result_vector; // inference results
	
	std::string outfile = ""; // where to print results. If outfile=="", we will use 'stdout'
	
	void A_load_CNN(std::string modelfile, std::string configfile, std::string framework); 
	
	void B_execute_inference(std::string folder, std::string image_pattern, std::vector<std::string> classes,
			          int Nmax, int HW);

    
};
    
    
void ModelOpenCVClass::A_load_CNN(std::string modelfile, std::string configfile, std::string framework) {
    /* a) Load CNN */    
    net = readNet(modelfile, configfile, framework);
    net.setPreferableBackend(3);
    net.setPreferableTarget(0); 
    fprintf(stdout, "[INFO] [OpenCV] Network loaded\n");
    //return net; 
}


void ModelOpenCVClass::B_execute_inference(std::string folder, std::string image_pattern, std::vector<std::string> classes,
					    int Nmax, int HW) {
    /* b) Execute inference CNN */
    
    // - output LOG file for report:
    int error;
    FILE* pdesc;
    if (!outfile.empty())  
	pdesc = openFile(outfile, &error);
    else
        pdesc = stdout;
    
    cv::Scalar mean(103.939f,116.779f,123.68f);  
    float scale = 1.0f;    
    
    // i) Read images and categories names
 
    // IMAGE FILES READING
    std::vector<cv::String> fn;
    int Nimgs = Nmax; // number of images actually read. 
    bool result = getImageFiles(folder, image_pattern, &fn, &Nimgs); // Nmax will not be modified. Nimgs can be modified
    if(result)
	return;
    Nimgs = std::min(Nimgs,Nmax);
    
	
    bool CLASSES = !classes.empty();
    std::vector<int> number_per_class;
    
    if (CLASSES)
        for (int i = 0; i < classes.size(); i++) 
	    number_per_class.push_back(0);       // initialize vector to zeros
    else
        for (int i = 0; i < 1001; i++) 
	    number_per_class.push_back(0);       // initialize vector to zeros

 
    // ii) Inference over Nimgs (<=Nmax) images
    Mat frame, blob, prob;
    std::string image_file;
    Point classIdPoint;
    double confidence;
    int classId;
    
    //std::string image_file; 		
    std::string dst_imgfile;
    std::string pred_class;

    //! time  
    struct timespec t0, t1;
    clock_gettime(CLOCK_MONOTONIC, &t0);
    
    for (int i=0; i<Nimgs; i++) 
    {
	image_file=fn[i];
	frame = imread( image_file );
	if (frame.empty())
	    break;
	    
	blobFromImage(frame, blob, scale, Size(HW,HW), mean, false, false);
	net.setInput(blob);
	prob = net.forward();

	minMaxLoc(prob.reshape(1, 1), 0, &confidence, 0, &classIdPoint);
	classId = classIdPoint.x;
	
	if (classId >= classes.size()) { // Warning: predicted class out of labeled classes 
	        std::string label = format("%s (prob %.1f\%)", format("Class #%d", classId).c_str(), 100*confidence);
		fprintf(pdesc, " %s classified as %s %s[WARNING] Predicted class %d out of labels in file%s\n", image_file.c_str(), label.c_str(), BOLDYELLOW, classId, RESET);
	        //std::cout << image_file << " classified as: " << label << BOLDYELLOW << "[WARNING] Predicted class " << classId << " out of labels in file" << RESET << "\n";		
		continue;	
	    }
		
	    pred_class = (CLASSES) ? classes[classId] : std::to_string(classId);
	    // Print predicted class.
	    std::string label = format("%s (prob %.1f\%)", (classes.empty() ? format("Class #%d", classId).c_str() :
								pred_class.c_str()), 100*confidence);
	    fprintf(pdesc, " %s classified as %s\n", image_file.c_str(), label.c_str());
	    //fprintf(stdout, " %s classified as %s\n", image_file.c_str(), label.c_str());
	    
	    number_per_class[classId] ++;
	    
	    if (CLASSES && classId != unrelevant_CLASS) {
		dst_imgfile = RemovePrefix_AddSubstring(image_file, "_"+pred_class, "/", ".");
		execCommand( ("mv '" + image_file + "' '" + OUTPUT_FOLDER + pred_class +"/" + dst_imgfile + "'").c_str() );   // copy to corresponding folder 		
	    }
	    
	
    } // end for   
    
    //! time
    clock_gettime(CLOCK_MONOTONIC, &t1);
    double elapsed_t = getTimeInterval_ms(t0, t1);
    //std::cout << "(CNN runtime " << elapsed_t << " ms)" << std::endl;
    fprintf(pdesc, "(CNN runtime %.0f ms)\n\n", elapsed_t);
	    
    for (int i = 0; i < classes.size(); i++) 
	//std::cout << std::endl << classes[i] << ": " << number_per_class[i] << " imgs";
        fprintf(pdesc, "%s: %d imgs\n", classes[i].c_str(), number_per_class[i]);
	
    // Return information:
    result_vector = number_per_class;
    
    // - close LOG file
    if (!outfile.empty())
    {
	fclose(pdesc);
	fflush(pdesc); // write buffers into disk // man fflush
	//fsync(pdesc); // write buffers into disk
    }

}      



