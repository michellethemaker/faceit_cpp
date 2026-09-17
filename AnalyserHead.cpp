#include "AnalyserHead.h"
#include "CommonMath.h"
#include "AppConfig.h"
#include <algorithm>
#include <cmath>
#include <iostream>

PSCalibrateHeadState AnalyserHead::calibrateHead(const AllKeypoints& keypoint, char c)
{
    //PSCalibrateHeadState calibrateheadstate;
    CommonMath cmath;
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
    const auto& mouth = keypoint.keypoints[MOUTH];
    const auto& chin = keypoint.keypoints[CHIN];

    float leftvsrightdist = cmath.EuclDist(lear, nose) - cmath.EuclDist(rear, nose);
    float eyedist = cmath.EuclDist(leye, reye);
    float leftvsrightdist_normalised = leftvsrightdist / eyedist;


    float upvsdowndist = cmath.EuclDist(meye, nose)/ cmath.EuclDist(nose, chin);
    float vertdist = cmath.EuclDist(meye, chin);
    float pitch_normalised = (upvsdowndist / vertdist) - g_calibHeadState.noseNeutralY;

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
        if (leftvsrightdist_normalised > 3) //keep it pos. TODO: put these constants in appconfig
        {
            g_calibHeadState.noseRightX = leftvsrightdist_normalised;
        }
        else
            g_calibHeadState.noseRightX = 0.15;
        break;
    case 'w':
        std::cout << "pitchUp saved " << upvsdowndist << "||" << pitch_normalised << "\n";

        if (pitch_normalised < 1 ) //keep it neg
        {
            
            g_calibHeadState.noseUpY = pitch_normalised;
        }
        else
        {
            g_calibHeadState.noseUpY = 0;
            std::cout << "nope not good " << pitch_normalised << "\n";
        }

        //std::cout << "pitchUp saved: " << g_calibHeadState.noseUpY << "/n";

        break;
    case 's':

        if (pitch_normalised > 0.001) //keep it pos
        {
            std::cout << "pitchDown saved " << upvsdowndist << "||" << pitch_normalised << "\n";
            g_calibHeadState.noseDownY = pitch_normalised;
        }
        else
        {
            std::cout << "nope not good " << pitch_normalised << "\n";
            g_calibHeadState.noseDownY = 0.001;
        }
        
        break;

    case ' ': //NEUTRAL POSITION
            std::cout << "NEUTRAL XY " << leftvsrightdist_normalised << "||" << pitch_normalised << "\n";
            g_calibHeadState.noseNeutralY = upvsdowndist / vertdist;

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
    CommonMath cmath;
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

    const auto& ls = keypoint.keypoints[LEFT_SHOULDER];
    const auto& rs = keypoint.keypoints[RIGHT_SHOULDER];
    
    float leftvsrightdist = cmath.EuclDist(lear, nose) - cmath.EuclDist(rear, nose);
    float eyedist = cmath.EuclDist(leye, reye);
    float leftvsrightdist_normalised = leftvsrightdist / eyedist;

    float upvsdowndist = cmath.EuclDist(meye, nose) / cmath.EuclDist(nose, chin);
    float vertdist = cmath.EuclDist(meye, chin);
    float pitch_normalised = (upvsdowndist / vertdist) - g_calibHeadState.noseNeutralY;

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
    if (pitch_normalised < g_calibHeadState.noseUpY)//&& pitch_normalised < 2.6)
    {
        //std::cout << "\n^^^^^^^^UP\n";
        headstate.headYup_val = pitch_normalised * AppConfigHead::SCALEFACTOR_UD;
        headstate.headYup = true;
        headstate.headYdown = false;
    }

    if (pitch_normalised > g_calibHeadState.noseDownY)//&& pitch_normalised > -2.8)
    {
        //std::cout << "\n______DOWN\n";
        headstate.headYdown_val = pitch_normalised * AppConfigHead::SCALEFACTOR_UD;
        headstate.headYup = false;
        headstate.headYdown = true;

    }

    // ===== HEAD TILT =====
    headstate.tiltLeft = cmath.Angle(ls, chin, leye) < AppConfigHead::LEFTTILT_MIN && ls.confidence > 0.5f;
    headstate.tiltRight = cmath.Angle(rs, chin, reye) < AppConfigHead::RIGHTTILT_MIN && rs.confidence > 0.5f;
    //std::cout << headstate.tiltLeft << "|" << headstate.tiltRight << "\n";
    return headstate;
}

