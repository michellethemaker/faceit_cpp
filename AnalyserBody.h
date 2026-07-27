#pragma once

#include <string>
#include "Keypoint.h"
#include "PoseState.h"

class AnalyserBody
{
public:
    PSBodyState analyseBody(const AllKeypoints& keypoint); // use posestate
};