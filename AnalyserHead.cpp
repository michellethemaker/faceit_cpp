#include "AnalyserHead.h"
#include "CommonMath.h"
#include <algorithm>
#include <cmath>
#include <iostream>

PSCalibrateHeadState AnalyserHead::calibrateHead(const AllKeypoints& keypoint, char c)
{
    //PSCalibrateHeadState calibrateheadstate;
    CommonMath commonmath;
    if (keypoint.keypoints.size() < 17)
        return g_calibHeadState;

    const auto& leye = keypoint.keypoints[LEFT_EYE];
    const auto& reye = keypoint.keypoints[RIGHT_EYE];
    const auto& lear = keypoint.keypoints[LEFT_EAR];
    const auto& rear = keypoint.keypoints[RIGHT_EAR];
    const auto& nose = keypoint.keypoints[NOSE];

    //calibrateheadstate.noseXcoord = nose.x;
    //calibrateheadstate.noseYcoord = nose.y;
    switch (c)
    {
    case 'a':
        std::cout << "yawLeft saved\n";
        g_calibHeadState.noseLeftX = nose.x;
        //calculate other relevant points here
        break;
    case'd':
        std::cout << "yawRight saved\n";
        g_calibHeadState.noseRightX = nose.x;
        //calculate other relevant points here
        break;
    case 'w':
        std::cout << "pitchUp saved\n";
        g_calibHeadState.noseUpY = nose.y;
        //calculate other relevant points here
        break;
    case 's':
        g_calibHeadState.noseDownY = nose.y;
        std::cout << "pitchDown saved: "<< g_calibHeadState.noseUpY << "/n";
        
        //calculate other relevant points here
        break;
    case 'p':
        std::cout << "CALIB DONE\n";
        return g_calibHeadState;
    };
    return g_calibHeadState;
}
PSHeadState AnalyserHead::analyseHead(const AllKeypoints& keypoint)
{
    PSHeadState headstate;
    CommonMath commonmath;
    if (keypoint.keypoints.size() < 17)
        return headstate;

    const auto& leye = keypoint.keypoints[LEFT_EYE];
    const auto& reye = keypoint.keypoints[RIGHT_EYE];
    const auto& lear = keypoint.keypoints[LEFT_EAR];
    const auto& rear = keypoint.keypoints[RIGHT_EAR];
    const auto& nose = keypoint.keypoints[NOSE];
    
    double leftvsrightdist = commonmath.EuclDist(lear, nose) - commonmath.EuclDist(rear, nose);
    double eyedist = commonmath.EuclDist(leye, reye);
    double leftvsrightdist_normalised = leftvsrightdist / eyedist;
    double pitch = commonmath.SignedAngle(lear, nose, rear);

    if (leftvsrightdist_normalised < 0 && leftvsrightdist_normalised < -0.9)
    {
        //std::cout << "<<<<<<<<<<<\n";
        headstate.headXleft_val = leftvsrightdist_normalised;
        headstate.headXleft = true;
        headstate.headXright = false;
    }
    if (leftvsrightdist_normalised > 0 && leftvsrightdist_normalised > 0.9)
    {
        //std::cout << "           >>>>>>>>>>>>\n";
        headstate.headXright_val = leftvsrightdist_normalised;
        headstate.headXleft = false;
        headstate.headXright = true;
    }
    
    
    if (pitch> 1 && pitch <2.8)
    {
        //std::cout << "\n^^^^^^^^UP\n";
        headstate.headYup_val = pitch;
        headstate.headYup = true;
        headstate.headYdown = false;
        
    }
    if (pitch<-1 && pitch > -2.8)
    {
        //std::cout << "\n______DOWN\n";
        headstate.headYdown_val = pitch;
        headstate.headYup = false;
        headstate.headYdown = true;

    }

    //std::cout <<"ANGLE: " << pitch << "\n";




    ////x coords
    //float yawLeftMax = g_calibHeadState.noseLeftX;
    //float yawRightMax = g_calibHeadState.noseRightX;
    //std::cout <<                            yawLeftMax << "||" << yawRightMax << "\n";
    //float yawRaw = nose.x;
    //float x = (yawRaw - yawLeftMax) / (yawRightMax - yawLeftMax);
    //headstate.headXcoord = std::clamp(x, 0.0f, 1.0f);


    ////y coords
    //float pitchUpMax = g_calibHeadState.noseUpY;
    //float pitchDownMax = g_calibHeadState.noseDownY;
    //std::cout << pitchUpMax<<"||"<<pitchDownMax << "\n";
    //float pitchRaw = nose.y;
    //float y = (pitchRaw - pitchUpMax) / (pitchDownMax - pitchUpMax);
    //headstate.headYcoord = std::clamp(y, 0.0f, 1.0f);
   
    return headstate;
}

