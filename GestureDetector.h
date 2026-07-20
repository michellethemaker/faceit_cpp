#pragma once

#include <string>
#include "Keypoint.h"

class GestureDetector
{
public:
    bool loadModel(const std::wstring& modelPath);
    std::string detectGesture(const AllKeypoints& keypoint);
};