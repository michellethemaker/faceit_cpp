#include "CameraHandle.h"

Camera::Camera(int index)
    : camera(index, cv::CAP_DSHOW) // initialise camera immediately w/o using a dummy variable. better. 
{
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