#include "HandDetector.h"

HandDetector::HandDetector()
{
}

bool HandDetector::initialize()
{
    initialized = true;
    return true;
}

std::vector<Hand> HandDetector::detect(const cv::Mat& frame)
{
    std::vector<Hand> hands;

    if (!initialized || frame.empty())
        return hands;
    return hands;
}