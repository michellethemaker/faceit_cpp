#pragma once
#include <onnxruntime_cxx_api.h>
#include <opencv2/opencv.hpp>
#include <memory>
#include <string>
#include <vector>
#include "KeypointHand.h"
//threading headers
#include <thread>
#include <mutex>
#include <atomic>

struct KeypointHand
{
    float x;
    float y;
    float confidence;
};

struct AllHandKeypoints
{
    std::vector<KeypointHand> keypointshand;
    float score = 0.0f;//overall pose conf

};

enum HandPart
{
    WRIST = 0,
    THUMB_1 = 1,
    THUMB_2 = 2,
    THUMB_3 = 3,
    THUMB_TIP = 4,
    INDEX_1 = 5,
    INDEX_2 = 6,
    INDEX_3 = 7,
    INDEX_TIP = 8,
    MIDDLE_1 = 9,
    MIDDLE_2 = 10,
    MIDDLE_3 = 11,
    MIDDLE_TIP = 12,
    RING_1 = 13,
    RING_2 = 14,
    RING_3 = 15,
    RING_TIP = 16,
    PINKY_1 = 17,
    PINKY_2 = 18,
    PINKY_3 = 19,
    PINKY_TIP = 20
};