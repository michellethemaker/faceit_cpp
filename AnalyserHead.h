#pragma once

#include "Keypoint.h"
#include "PoseState.h"
//#include "Calibration.h"

class AnalyserHead
{
public:
    PSCalibrateHeadState calibrateHead(const AllKeypoints& keypoint, char c);
    PSHeadState analyseHead(const AllKeypoints& keypoint); // use posestate
private:
    // calibration offsets (in degrees)
    float yaw0 = 0.0f;
    float pitch0 = 0.0f;
    float roll0 = 0.0f;

};