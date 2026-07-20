#pragma once
#include <vector>
#include <string>
#include <opencv2/opencv.hpp>
#include "Hand.h"

class HandAnalyser
{
public:
    HandAnalyser();
    bool initialize();
    std::vector<Hand> detect(const cv::Mat& frame);

private:
    bool initialized = false;
};