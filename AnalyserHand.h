#pragma once

#include "KeypointHand.h"
#include "PoseState.h"

class AnalyserHand
{
public:
    PSHandState analyseHand(const AllHandKeypoints& keypoint); // use posestate
};