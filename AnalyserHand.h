#pragma once

#include "Keypoint.h"
#include "PoseState.h"

class AnalyserHand
{
public:
    PSHandState analyseHand(const AllKeypoints& keypoint); // use posestate
};