#include "AnalyserHand.h"

AnalyserHand::AnalyserHand()
{
}

bool AnalyserHand::initialize()
{
    initialized = true;
    return true;
}

std::vector<Hand> AnalyserHand::detect(const cv::Mat& frame)
{
    std::vector<Hand> hands;

    if (!initialized || frame.empty())
        return hands;
    return hands;
}