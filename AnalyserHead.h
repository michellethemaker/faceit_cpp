#pragma once

#include "Keypoint.h"
#include "PoseState.h"
//#include "Calibration.h"

class AnalyserHead
{
public:
    PSCalibrateHeadState calibrateHead(const AllKeypoints& keypoint, char c);
    PSHeadState analyseHead(const AllKeypoints& keypoint); // use posestate
  

};