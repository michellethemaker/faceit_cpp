#include "CameraHandle.h"
#include "AppConfig.h"
#define DEBUG

Camera::Camera(int index)
    : camera(index, cv::CAP_DSHOW) // initialise camera immediately w/o using a dummy variable. better. 
{
    //camera.set(cv::CAP_PROP_FOURCC, cv::VideoWriter::fourcc('M', 'J', 'P', 'G')); //TODO: check if this helps in low lighting
    camera.set(cv::CAP_PROP_FRAME_WIDTH, AppConfigCamera::kFrameWidth); // RMB! increasing res improves accuracy, lowering res improves fps
    camera.set(cv::CAP_PROP_FRAME_HEIGHT, AppConfigCamera::kFrameHeight);
}

bool Camera::isOpened() const
{
    return camera.isOpened();
}

bool Camera::getFrame(cv::Mat& frame)
{
    //std::cout << camera.get(cv::CAP_PROP_FOCUS) << "\n"; //check if AF val can b used to det depth...
    camera >> frame;
#ifndef DEBUG
    cv::cvtColor(frame, frame, cv::COLOR_BGR2GRAY); //grayscale image
#endif
    cv::GaussianBlur(frame, frame, cv::Size(5, 5), 0); // gauss blur
    return !frame.empty();
}