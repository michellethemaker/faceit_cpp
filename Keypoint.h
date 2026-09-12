#pragma once

#include <vector>

struct Keypoint
{
	float x;
	float y;
	float confidence;
};

struct AllKeypoints
{
	std::vector<Keypoint> keypoints;
	float score = 0.0f;//overall pose conf
	
};

enum BodyPart
{
    // FACE: 23 - 90
    // LEFT HAND: 112 - 132
    // RIGHT HAND: 91 - 111

    NOSE = 53,
    LEFT_EYE = 2,
    MIDDLE_EYE = 50,
    RIGHT_EYE = 1,
    LEFT_EAR = 4,
    RIGHT_EAR = 3,
    LEFT_EAR_SIDE = 24,
    RIGHT_EAR_SIDE = 38,
    MOUTH = 80,
    LEFT_MOUTH = 71,
    RIGHT_MOUTH = 77,
    CHIN = 31,

    LEFT_ELBOW = 8,
    RIGHT_ELBOW = 7,
    LEFT_WRIST = 10,
    RIGHT_WRIST = 9,
    
    LEFT_PINKY = 132,
    LEFT_PINKY_2 = 131,
    LEFT_PINKY_BASE = 129,
    LEFT_INDEX = 120,
    LEFT_INDEX_2 = 119,
    LEFT_INDEX_BASE = 117,
    LEFT_THUMB = 116,
    LEFT_THUMB_2 = 115,
    LEFT_THUMB_BASE = 114,

    RIGHT_PINKY = 111,
    RIGHT_PINKY_2 = 110,
    RIGHT_PINKY_BASE = 108,
    
    RIGHT_INDEX = 99,
    RIGHT_INDEX_2 = 98,
    RIGHT_INDEX_BASE = 96,
    
    RIGHT_THUMB = 95,
    RIGHT_THUMB_2 = 94,
    RIGHT_THUMB_BASE = 93,

    LEFT_SHOULDER = 6,
    RIGHT_SHOULDER = 5,
    LEFT_HIP = 12,
    RIGHT_HIP = 11,
    LEFT_KNEE = 13,
    RIGHT_KNEE = 14,
    LEFT_ANKLE = 16,
    RIGHT_ANKLE = 15,
    LEFT_HEEL = 22,
    RIGHT_HEEL = 19,
    LEFT_INNERFOOT = 20,
    RIGHT_INNERFOOT = 17,
    LEFT_OUTERFOOT = 21,
    RIGHT_OUTERFOOT = 18
};