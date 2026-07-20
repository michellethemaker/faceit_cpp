#pragma once

#include <string>
#include "Keypoint.h"
#include "PoseState.h"

class BodyAnalyser
{
public:
    BodyState analyseBody(const AllKeypoints& keypoint); // use posestate
};