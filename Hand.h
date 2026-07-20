#pragma once

#include <vector>

struct HandLandmark
{
    float x;
    float y;
    float z;
    float visibility = 1.0f;
};

struct Hand
{
    std::vector<HandLandmark> landmarks;
    bool isLeft = false;
    float score = 0.0f;
};