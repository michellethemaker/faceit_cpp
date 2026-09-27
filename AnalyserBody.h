#pragma once

#include <string>
#include "Keypoint.h"
#include "PoseState.h"

class AnalyserBody
{
public:
    PSCalibrateBodyState calibrateBody(const AllKeypoints& keypoint, char c);
    PSBodyState analyseBody(const AllKeypoints& keypoint); // use posestate
};