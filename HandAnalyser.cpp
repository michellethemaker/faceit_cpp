#include "HandAnalyser.h"

HandAnalyser::HandAnalyser()
{
}

bool HandAnalyser::initialize()
{
    initialized = true;
    return true;
}

std::vector<Hand> HandAnalyser::detect(const cv::Mat& frame)
{
    std::vector<Hand> hands;

    if (!initialized || frame.empty())
        return hands;
    return hands;
}