#pragma once
#include <opencv2/opencv.hpp>

class Camera
{
public:
	Camera(int index = 1); //0 for inbuilt webcam, 1 + cv.CAP_DSHOW for ext
	bool isOpened() const;
	bool getFrame(cv::Mat& frame);
private:
	cv::VideoCapture camera;
};