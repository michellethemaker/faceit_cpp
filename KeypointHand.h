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
    THUMB_BASE = 1,
    THUMB_2 = 2,
    THUMB_TIP = 3,
    THUMB_3 = 4,
    MIDDLE_2 = 5,
    MIDDLE_3 = 6,
    MIDDLE_TIP = 7,
    RING_BASE = 8,
    RING_2 = 9,
    INDEX_BASE = 10,
    INDEX_2 = 11,
    INDEX_3 = 12,
    INDEX_TIP = 13,
    MIDDLE_BASE = 14,
    RING_3 = 15,
    RING_TIP = 16,
    PINKY_BASE = 17,
    PINKY_2 = 18,
    PINKY_3 = 19,
    PINKY_TIP = 20

};