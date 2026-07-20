#pragma once
#include <opencv2/opencv.hpp>

class Camera
{
public:
	Camera(int index = 0);
	bool isOpened() const;
	bool getFrame(cv::Mat& frame);
private:
	cv::VideoCapture camera;
};