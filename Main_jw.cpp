#include <opencv2/opencv.hpp>
#include <opencv2/dnn.hpp>
#include <opencv2/dnn/shape_utils.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>
#include <fstream>
#include <iostream>
#include "NT.h"
#include "StrCommon.h"

using namespace std;
using namespace cv;
using namespace cv::dnn;

NT *_tt = NULL;


void DrawData(cv::Mat mm, cv::Mat frame, const std::map<int, DSResult> &map, 
        const std::vector<cv::Rect> &outRcs,
        bool detect){
    std::map<int, DSResult>::const_iterator it;
    for(it = map.begin(); it != map.end(); ++it){
        CvScalar clr = cvScalar(0, 255, 0);
        cv::Rect rc = it->second.rc_;
            cv::rectangle(frame, rc, clr);
        std::string disp = toStr(it->first);
        cv::putText(frame, 
            disp, 
            cvPoint(rc.x, rc.y), 
            CV_FONT_HERSHEY_SIMPLEX, 
            0.6, 
            cv::Scalar(0, 0, 255));
    }
    //
    CvScalar clr = cvScalar(0, 0, 255);
    for(cv::Rect rc:outRcs){
        cv::rectangle(mm, rc, clr); 
        if(detect){
            std::string disp = "detect";
            cv::putText(frame, 
                disp, 
                cvPoint(100, 100), 
                CV_FONT_HERSHEY_SIMPLEX, 
                1, 
                cv::Scalar(0, 0, 255));
        }
    }
}


std::map<int, std::vector<cv::Rect>> _rcMap;

std::string _rcFile = "";
std::string _imgDir;
cv::VideoWriter *_vw = NULL;
bool _isShow = false;
int _imgCount = 0;

void CB(cv::Mat &frame, int num){
	
	// if (_rcMap.empty()) {
	// 	ReadRcFileTotal(_rcFile);
	// }
	std::vector<cv::Rect> rcs;
	std::map<int, std::vector<cv::Rect>>::iterator it = _rcMap.find(num);
	if (it != _rcMap.end()) {
		rcs = it->second;
	}
	std::vector<cv::Rect> outRcs;
	
	std::map<int, DSResult> map = _tt->UpdateAndGet(frame, rcs, num, outRcs);

	Mat mm = frame.clone();
	bool detect = (!rcs.empty());
	DrawData(mm, frame, map, outRcs, detect);
	//(*_vw) << frame;
	if(_isShow){
		std::string disp = "frame";
		cv::resize(mm, mm, cv::Size(mm.cols/2, mm.rows/2));
		cv::resize(frame, frame, cv::Size(frame.cols/2, frame.rows/2));
		cv::imshow("mm", mm);
		cv::imshow(disp, frame);
		cv::waitKey(1);
	}
}


static const char* params =
"{ camera_device  | 0     | camera device number}"
"{ source         |       | video or image for detection}"
"{ out            |       | path to output video file}"
"{ fps            | 3     | frame per second }"
"{ min_confidence | 0.24  | min confidence      }"
"{ show 		  | 1     | show or not      }";

int main(int argc, char **argv){

	_tt = new NT();
	if(!_tt->Init()){
		return 0;
	}
	cv::CommandLineParser parser(argc, argv, params);

	_isShow = parser.get<int>("show");
	float confidenceThreshold = parser.get<float>("min_confidence");
	String modelConfiguration = "/home/joseph/Github/metricnet/tracking/data/models/head_face_last/yolo_metric_train.cfg";
	String modelBinary = "/home/joseph/Github/metricnet/tracking/data/models/head_face_last/yolo_metric_train_30000.weights";
	String class_names = "/home/joseph/Github/metricnet/tracking/data/models/head_face_last/yolo_metric_train.names";

    dnn::Net net = readNetFromDarknet(modelConfiguration, modelBinary);
    if (net.empty()){
        cerr << "Can't load network by using the following files: " << endl;
        cerr << "cfg-file:     " << modelConfiguration << endl;
        cerr << "weights-file: " << modelBinary << endl;
        cerr << "Models can be downloaded here:" << endl;
        cerr << "https://pjreddie.com/darknet/yolo/" << endl;
        exit(-1);
    }
	cv::VideoCapture cap;
    int codec = CV_FOURCC('M', 'J', 'P', 'G');
    double fps = parser.get<float>("fps");
    if (parser.get<String>("source").empty()){
        int cameraDevice = parser.get<int>("camera_device");
        cap = cv::VideoCapture(cameraDevice);
        if(!cap.isOpened()){
            cout << "Couldn't find camera: " << cameraDevice << endl;
            return -1;
        }
    }
    else{
        cap.open(parser.get<String>("source"));
        if(!cap.isOpened()){
            cout << "Couldn't open image or video: " << parser.get<String>("video") << endl;
            return -1;
        }
    }
	if(!parser.get<String>("out").empty()){
    	_vw = new cv::VideoWriter(parser.get<String>("out"), codec, fps, Size((int)cap.get(CAP_PROP_FRAME_WIDTH),(int)cap.get(CAP_PROP_FRAME_HEIGHT)), 1);
    }
    else{
    	_vw = new cv::VideoWriter("out.avi", codec, fps, Size((int)cap.get(CAP_PROP_FRAME_WIDTH),(int)cap.get(CAP_PROP_FRAME_HEIGHT)), 1);
    }
    std::vector<String> classNamesVec;
    ifstream classNamesFile(class_names.c_str());
    if (classNamesFile.is_open()){
        string className = "";
        while (std::getline(classNamesFile, className))
            classNamesVec.push_back(className);
    }
    for(int frame_id = 1; ; frame_id++){
        Mat frame;
        cap >> frame; // get a new frame from camera/video or read image
        if (frame.empty()){
            waitKey();
            break;
        }
        if (frame.channels() == 4)
            cvtColor(frame, frame, COLOR_BGRA2BGR);
        Mat inputBlob = blobFromImage(frame, 1 / 255.F, Size(416, 416), Scalar(), true, false); //Convert Mat to batch of images
        net.setInput(inputBlob, "data");                   //set the network input
        Mat detectionMat = net.forward("detection_out");   //compute output
        
        for (int i = 0; i < detectionMat.rows; i++)
        {
            const int probability_index = 5;
            const int probability_size = detectionMat.cols - probability_index;
            std::vector<cv::Rect> rcs;
            float *prob_array_ptr = &detectionMat.at<float>(i, probability_index);
            size_t objectClass = max_element(prob_array_ptr, prob_array_ptr + probability_size) - prob_array_ptr;
            float confidence = detectionMat.at<float>(i, (int)objectClass + probability_index);
            if (confidence > confidenceThreshold){
                cv:Rect rc;
                float x_center = detectionMat.at<float>(i, 0) * frame.cols;
                float y_center = detectionMat.at<float>(i, 1) * frame.rows;
                rc.width = detectionMat.at<float>(i, 2) * frame.cols;
                rc.height = detectionMat.at<float>(i, 3) * frame.rows;
                rc.x = cvRound(x_center - rc.width / 2);
                rc.y = cvRound(y_center - rc.height / 2);
            }
            _rcMap.insert(std::make_pair(frame_id, rcs));
        }
        CB(frame, frame_id);
        if(_vw -> isOpened())
        {
            _vw -> write(frame);
        }
        if (waitKey(1) >= 0) break;
    }



	return 0;
}


