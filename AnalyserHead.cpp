#include "AnalyserHead.h"
#include "CommonMath.h"
#include "AppConfig.h"
#include <algorithm>
#include <cmath>
#include <iostream>

PSCalibrateHeadState AnalyserHead::calibrateHead(const AllKeypoints& keypoint, char c)
{
    //PSCalibrateHeadState calibrateheadstate;
    CommonMath commonmath;
    //if (keypoint.keypoints.size() < 17)
    //    return g_calibHeadState;

    const auto& leye = keypoint.keypoints[LEFT_EYE];
    const auto& reye = keypoint.keypoints[RIGHT_EYE];
    const auto& meye = keypoint.keypoints[MIDDLE_EYE];
    const auto& lear = keypoint.keypoints[LEFT_EAR];
    const auto& rear = keypoint.keypoints[RIGHT_EAR];
    const auto& learside = keypoint.keypoints[LEFT_EAR_SIDE];
    const auto& rearside = keypoint.keypoints[RIGHT_EAR_SIDE];
    const auto& nose = keypoint.keypoints[NOSE];


    float leftvsrightdist = commonmath.EuclDist(lear, nose) - commonmath.EuclDist(rear, nose);
    float eyedist = commonmath.EuclDist(leye, reye);
    float leftvsrightdist_normalised = leftvsrightdist / eyedist;


    float upvsdowndist = ((lear.y - learside.y) + (rear.y - rearside.y)) / 2;
    float vertdist = commonmath.EuclDist(meye, nose);
    float pitch_normalised = (upvsdowndist / vertdist);

    switch (c)
    {
    case 'a':
        std::cout << "yawLeft saved "<< g_calibHeadState.noseLeftX << leftvsrightdist_normalised <<"\n";
        if (leftvsrightdist_normalised < -0.01) //keep it negative
        {
            g_calibHeadState.noseLeftX = leftvsrightdist_normalised;
        }
        else
        {
            g_calibHeadState.noseLeftX = -0.5;
            std::cout << "nope not good "<< leftvsrightdist_normalised <<"\n";
        }
        break;
    case'd':
        std::cout << "yawRight saved " << g_calibHeadState.noseRightX << leftvsrightdist_normalised << "\n";
        if (leftvsrightdist_normalised > 0.5) //keep it pos
        {
            g_calibHeadState.noseRightX = leftvsrightdist_normalised;
        }
        else
            g_calibHeadState.noseRightX = 1.5;
        break;
    case 'w':
        std::cout << "pitchUp saved\n";
        if (upvsdowndist > 0.5) //keep it positive
        {
            g_calibHeadState.noseUpY = upvsdowndist;
        }
        else
        {
            g_calibHeadState.noseUpY = 1.5;
            std::cout << "nope not good " << upvsdowndist << "\n";
        }
            
        std::cout << "pitchUp saved: " << g_calibHeadState.noseUpY << "/n";
        
        break;
    case 's':
        if (upvsdowndist < -0.5) //keep it negative
        {
            g_calibHeadState.noseDownY = upvsdowndist;
        }
        else
        {
            g_calibHeadState.noseDownY = -1.5;
            std::cout << "nope not good " << upvsdowndist << "\n";
        }
            
        std::cout << "pitchDown saved: "<< g_calibHeadState.noseDownY << "/n";
        
        
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
    const auto& meye = keypoint.keypoints[MIDDLE_EYE];
    const auto& reye = keypoint.keypoints[RIGHT_EYE];
    const auto& lear = keypoint.keypoints[LEFT_EAR];
    const auto& rear = keypoint.keypoints[RIGHT_EAR];
    const auto& learside = keypoint.keypoints[LEFT_EAR_SIDE];
    const auto& rearside = keypoint.keypoints[RIGHT_EAR_SIDE];
    const auto& nose = keypoint.keypoints[NOSE];
    const auto& mouth = keypoint.keypoints[MOUTH];
    const auto& chin = keypoint.keypoints[CHIN];
    
    float leftvsrightdist = commonmath.EuclDist(lear, nose) - commonmath.EuclDist(rear, nose);
    float eyedist = commonmath.EuclDist(leye, reye);
    float leftvsrightdist_normalised = leftvsrightdist / eyedist;

    float upvsdowndist = ((lear.y- learside.y) + (rear.y- rearside.y)) /2 ;
    float vertdist = commonmath.EuclDist(meye, nose);
    float pitch_normalised = (upvsdowndist / vertdist);

    float scaleFactorLR = 3;
    float scaleFactorUD = 3.8;
    //std::cout << leftvsrightdist_normalised << "||"<< upvsdowndist << "\n";
    if (leftvsrightdist_normalised < g_calibHeadState.noseLeftX)
    {
        //std::cout << "<<<<<<<<<<<\n";
        headstate.headXleft_val = leftvsrightdist_normalised * AppConfigHead::SCALEFACTOR_LR;
        headstate.headXleft = true;
        headstate.headXright = false;
    }
    if (leftvsrightdist_normalised > g_calibHeadState.noseRightX)
    {
        //std::cout << "           >>>>>>>>>>>>\n";
        headstate.headXright_val = leftvsrightdist_normalised * AppConfigHead::SCALEFACTOR_LR;
        headstate.headXleft = false;
        headstate.headXright = true;
    }
    //std::cout << headstate.headXleft_val << "||" << headstate.headXright_val << "\n";
    //std::cout << 1 / pitch_normalised <<"||"<< "" << "\n";
    if (upvsdowndist > g_calibHeadState.noseUpY)//&& pitch_normalised < 2.6)
    {
        //std::cout << "\n^^^^^^^^UP\n";
        headstate.headYup_val = pitch_normalised * AppConfigHead::SCALEFACTOR_UD;
        headstate.headYup = true;
        headstate.headYdown = false;

    }
    if (upvsdowndist < g_calibHeadState.noseDownY)//&& pitch_normalised > -2.8)
    {
        //std::cout << "\n______DOWN\n";
        headstate.headYdown_val = pitch_normalised * AppConfigHead::SCALEFACTOR_UD;
        headstate.headYup = false;
        headstate.headYdown = true;

    }

    return headstate;
}

