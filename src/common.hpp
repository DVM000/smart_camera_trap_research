#include <sys/stat.h>

#include <sstream>
#include <iostream>
//#include <fstream>
#include <map>
#include <string>
//#include <stdio.h>
//#include <unistd.h>
//#include <fcntl.h>

using namespace std;

using namespace cv;

std::string OUTPUT_FOLDER = "OUTPUT/";

int unrelevant_CLASS = -1; // ignore this category when saving classified images

#define RESET   "\033[0m"
#define BLACK   "\033[30m"      /* Black */
#define RED     "\033[31m"      /* Red */
#define GREEN   "\033[32m"      /* Green */
#define YELLOW  "\033[33m"      /* Yellow */
#define BLUE    "\033[34m"      /* Blue */
#define MAGENTA "\033[35m"      /* Magenta */
#define CYAN    "\033[36m"      /* Cyan */
#define WHITE   "\033[37m"      /* White */
#define BOLDBLACK   "\033[1m\033[30m"      /* Bold Black */
#define BOLDRED     "\033[1m\033[31m"      /* Bold Red */
#define BOLDGREEN   "\033[1m\033[32m"      /* Bold Green */
#define BOLDYELLOW  "\033[1m\033[33m"      /* Bold Yellow */
#define BOLDBLUE    "\033[1m\033[34m"      /* Bold Blue */
#define BOLDMAGENTA "\033[1m\033[35m"      /* Bold Magenta */
#define BOLDCYAN    "\033[1m\033[36m"      /* Bold Cyan */
#define BOLDWHITE   "\033[1m\033[37m"      /* Bold White */


/* ------------------------------------
 * FILE READING 
 * ------------------------------------  
 * */
FILE* openFile(std::string fileName, int* err)
{
	/* Open file and return file descriptor */
	
	FILE* fd = fopen(fileName.c_str(),"a"); 	// Open the File in append mode
	if(!fd){//ferror(fd)){
	    std::cout << BOLDYELLOW << "[ERROR] Cannot open File '" << fileName << "'" << RESET << std::endl;
	    *err = 1;
	    return fd;
	    } 
        else {
	    *err = 0;
	    return fd;
	    }
	
}
 
static bool getFileContent(std::string fileName, std::vector<std::string>  *classesvector)
{
	/* Get file content and save into classesvector items*/
	
	std::ifstream in(fileName.c_str()); 	// Open the File
	if(!in.is_open()){
	    std::cout << BOLDYELLOW << "[ERROR] File '" << fileName << "' not found" << RESET << std::endl;
	    return true;
	    } 

	std::string line;
	// Read the next line from File untill it reaches the end.
	while (std::getline(in, line))
	{
	    // Line contains string of length > 0 then save it in vector
	    if(line.size()>0) (*classesvector).push_back(line);
	}
	// Close The File
	in.close();
	
	return false;
}

std::string getFileLastLine(std::string fileName)
{
	/* Return last line in file content */
	
	std::string lastLine;
	
	std::ifstream in(fileName.c_str()); 	// Open the File
	if(!in.is_open()){
	    std::cout << BOLDYELLOW << "[ERROR] File " << fileName << " not found" << RESET << std::endl;
	    return "Err" ;
	    } 

	std::string line;
	// Read the next line from File untill it reaches the end.
	while (std::getline(in, line))
	    lastLine = line;
	    
	// Close The File
	in.close();
	
	return lastLine;
}

std::string execCommand(const char *command, int MAXPERLINE=1024, bool verbose=false){
    /* Execute external command nd return its output (only first line)
     * Example: std:string result = execCommand("echo HELLO; echo BYE") would return "HELLO" in 'result' */
	
    // Setup pipe for reading and execute our command
    FILE *pf = popen(command,"r"); 
    char buffer[MAXPERLINE];
    char buffer2[MAXPERLINE];
    int i=0;

    // Error handling
    if  (!pf){
	std::cout << BOLDYELLOW << "[ERROR] Failed to start command: " << std::endl << "[" << RESET << command << BOLDYELLOW << "]" << RESET << std::endl;
        return "1";
        }
    // Get 1st line of data 
    fgets(buffer, MAXPERLINE, pf);
    if (verbose) std::cout << buffer;
    
    // Get more data 
    while( true ){
	if (fgets(buffer2, MAXPERLINE, pf) == NULL) break;
	i++;
	if (verbose) std::cout << buffer2;// << "\n";
	}

    if (pclose(pf) != 0) {
        std::cout << BOLDYELLOW << "[ERROR] Failed to close command stream: " << std::endl << "[" << RESET << command << BOLDYELLOW << "]" << RESET << std::endl;
        return "1";
        }
	
    // Save 1st line of data
    std::string result(buffer);
    result.erase(std::remove(result.begin(), result.end(), '\n'), result.end());
    return result;
}

/* ------------------------------------
 * FILESYSTEM MANAGEMENT
 * ------------------------------------  
 * */
static bool getImageFiles(std::string folder, std::string pattern, std::vector<cv::String> *files, int *Nmax)
{
    /* Get files from folder, just those following the specified pattern. Save number of files if it is superior to the maximum */
    struct stat sb;
    if (stat(folder.c_str(), &sb)==0 && S_ISDIR(sb.st_mode)) {
		if (!pattern.length())
			glob(folder+"*", *files, false);
		else
			glob(folder+pattern, *files, false);

		if (!(*files).size()){
			std::cout << BOLDYELLOW << "[ERROR] Not image found" << RESET << std::endl;
			return true;	    
			}
			
		if ((*files).size() < *Nmax)   // if there are less images than the maximum
			//std::cout << BOLDYELLOW << "[WARNING] Less than " << *Nmax << " images found: " << (*files).size() << RESET << std::endl;
			*Nmax = (*files).size();
    }
    else {
		std::cout << BOLDYELLOW << "[ERROR] Specified input is not a folder. Please check 'input' argument." << RESET << std::endl;
        return true;
    }
	
	return false;
}

static bool createFolders(std::string folder, std::vector<std::string> subfolders){
	/* Create subfolders in folder, with subfolder names specified in vector*/
	struct stat sb;
	std::string folder_create;
	for (int i = 0; i < subfolders.size(); i++) {
		folder_create = folder + subfolders[i];
		if ( !(stat(folder_create.c_str(), &sb)==0 && S_ISDIR(sb.st_mode)) ) {
			execCommand( ("mkdir " + folder_create).c_str() ); }  // create subfolders   
	   }
	   
	return false;
}


/* ------------------------------------
 * std::string UTILS
 * ------------------------------------  
 * */
std::vector<string> split (std::string s, std::string delimiter) {
   /* Split a string using a string delimeter */
   // https://stackoverflow.com/questions/14265581/parse-split-a-string-in-c-using-string-delimiter-standard-c
    size_t pos_start = 0, pos_end, delim_len = delimiter.length();
    std::string token;
    std::vector<string> res;

    while ((pos_end = s.find (delimiter, pos_start)) != std::string::npos) {
        token = s.substr (pos_start, pos_end - pos_start);
        pos_start = pos_end + delim_len;
        res.push_back (token);
    }

    res.push_back (s.substr (pos_start));
    return res;
}

std::string RemovePrefix_AddSubstring(std::string orig_s, std::string substring, std::string del1, std::string del2){
      /* Remove string part before last del1
       * Then, add substring. Position: before LAST del2.  
       * */
	std::vector<std::string> split_s;     
	std:string new_s;
	split_s = split(orig_s, del1);
	new_s = split_s[split_s.size()-1];  // take only last substring, without prefix
	split_s = split(new_s, del2);
	new_s = split_s[ split_s.size()-2 ] + substring + del2 + split_s[ split_s.size()-1 ]; // add substring just before last del2

	return new_s;
}

/* ------------------------------------
 * cv UTILS 
 * ------------------------------------  
 * */
void get_video_props(VideoCapture  cap) {
    /* Print Video Information */
    double fps = cap.get(CAP_PROP_FPS);         // Get the frame rate
    double W = cap.get(CAP_PROP_FRAME_WIDTH);  //get the width of frames of the video
    double H = cap.get(CAP_PROP_FRAME_HEIGHT); //get the height of frames of the video

    std::cout << MAGENTA << "[INFO] Video information:";
    std::cout << "\tFrame resolution: " << W << " x " << H;
    std::cout << ", \tFrames per second: " << fps << RESET << std::endl;
}

/* ------------------------------------
 * EXTRA 
 * ------------------------------------  
 * */
std::string getTimeString(){
  /* Get time-date string */
 
     time_t now = time(0);
     struct tm* now_tm = localtime(&now);
     char buffer[80];
     strftime(buffer,80,"%Y-%m-%d_%H:%M:%S", now_tm);
     std::string dt_s(buffer);   
     return dt_s;
}

double getTimeInterval_ms(struct timespec t0, struct timespec t1){
    /* Return ms from two time points */
    return (double) (t1.tv_sec * 1000.0 + t1.tv_nsec / 1000000 - t0.tv_sec * 1000 - t0.tv_nsec / 1000000);
}

/* ------------------------------------
 * CONFIGURATION FILE MANAGEMENT
 * ------------------------------------  
 * */
/* 
 * http://www.cplusplus.com/forum/general/21115/
 * 
 */
namespace configuration
  {

  //---------------------------------------------------------------------------
  // The configuration::data is a simple map string (key, value) pairs.
  // The file is stored as a simple listing of those pairs, one per line.
  // The key is separated from the value by an equal sign '='.
  // Commentary begins with the first non-space character on the line a hash or
  // semi-colon ('#' or ';').
  //
  // Example:
  //   # This is an example
  //   source.directory = C:\Documents and Settings\Jennifer\My Documents\
  //   file.types = *.jpg;*.gif;*.png;*.pix;*.tif;*.bmp
  //
  // Notice that the configuration file format does not permit values to span
  // more than one line, commentary at the end of a line, or [section]s.
  //   
  struct data: std::map <std::string, std::string>
    {
    // Here is a little convenience method...
    bool iskey( const std::string& s ) const
      {
      return count( s ) != 0;
      }
    };

  //---------------------------------------------------------------------------
  // The extraction operator reads configuration::data until EOF.
  // Invalid data is ignored.
  //
  std::istream& operator >> ( std::istream& ins, data& d )
    {
    std::string s, key, value;

    // For each (key, value) pair in the file
    while (std::getline( ins, s ))
      {
      std::string::size_type begin = s.find_first_not_of( " \f\t\v" );

      // Skip blank lines
      if (begin == std::string::npos) continue;

      // Skip commentary
      if (std::string( "#;" ).find( s[ begin ] ) != std::string::npos) continue;

      // Extract the key value
      std::string::size_type end = s.find( '=', begin );
      key = s.substr( begin, end - begin );

      // (No leading or trailing whitespace allowed)
      key.erase( key.find_last_not_of( " \f\t\v" ) + 1 );

      // No blank keys allowed
      if (key.empty()) continue;

      // Extract the value (no leading or trailing whitespace allowed)
      begin = s.find_first_not_of( " \f\n\r\t\v", end + 1 );
      end   = s.find_last_not_of(  " \f\n\r\t\v" ) + 1;

      value = s.substr( begin, end - begin );

      // Insert the properly extracted (key, value) pair into the map
      d[ key ] = value;
      }

    return ins;
    }

  //---------------------------------------------------------------------------
  // The insertion operator writes all configuration::data to stream.
  //
  std::ostream& operator << ( std::ostream& outs, const data& d )
    {
    data::const_iterator iter;
    for (iter = d.begin(); iter != d.end(); iter++)
      outs << iter->first << " = " << iter->second << std::endl;
    return outs;
    }

  } // namespace configuration 



