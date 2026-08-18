#include "CameraHandle.h"

Camera::Camera(int index)
    : camera(index, cv::CAP_DSHOW) // initialise camera immediately w/o using a dummy variable. better. 
{
    //camera.set(cv::CAP_PROP_FOURCC, cv::VideoWriter::fourcc('M', 'J', 'P', 'G')); //TODO: check if this helps in low lighting
}

bool Camera::isOpened() const
{
    return camera.isOpened();
}

bool Camera::getFrame(cv::Mat& frame)
{
    camera >> frame;
    return !frame.empty();
}