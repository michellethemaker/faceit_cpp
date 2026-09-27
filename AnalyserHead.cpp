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

    const auto& ls = keypoint.keypoints[LEFT_SHOULDER];
    const auto& rs = keypoint.keypoints[RIGHT_SHOULDER];

    
    float leftvsrightdist = cmath.EuclDist(lear, nose) - cmath.EuclDist(rear, nose);
    float eyedist = cmath.EuclDist(leye, reye);
    float leftvsrightdist_normalised = leftvsrightdist / eyedist;


    //float upvsdowndist = cmath.EuclDist(meye, nose)/ cmath.EuclDist(nose, chin);
    //float vertdist = cmath.EuclDist(meye, chin);
    //float pitch_normalised = (upvsdowndist / vertdist) - g_calibHeadState.noseNeutralY;

    float shoulderAvg = (ls.y + rs.y) / 2;
    float upvsdowndist = shoulderAvg - nose.y;
    float pitch_normalised = (upvsdowndist / shoulderAvg) ;
    switch (c)
    {
    case ' ': //NEUTRAL POSITION
        //g_calibHeadState.noseNeutralY = upvsdowndist / vertdist;
        g_calibHeadState.headShoulderNeutralY = upvsdowndist / shoulderAvg;
        std::cout << "NEUTRAL XY " << upvsdowndist << "||" << g_calibHeadState.headShoulderNeutralY << "\n";
        break;

    case 'a':
        if (leftvsrightdist_normalised < -0.01) //keep it negative
        {
            g_calibHeadState.noseLeftX = leftvsrightdist_normalised;
            std::cout << "yawLeft saved " << g_calibHeadState.noseLeftX << leftvsrightdist_normalised << "\n";
        }
        else
            std::cout << "nope not good "<< leftvsrightdist_normalised <<"\n";
        break;

    case'd':
        if (leftvsrightdist_normalised > 0.1) //keep it pos. TODO: put these constants in appconfig
        {
            g_calibHeadState.noseRightX = leftvsrightdist_normalised;
            std::cout << "yawRight saved " << g_calibHeadState.noseRightX << leftvsrightdist_normalised << "\n";
        }
        else
            std::cout << "nope not good " << leftvsrightdist_normalised << "\n";
        break;

    case 'w':
        //std::cout << "pitchUp saved " << upvsdowndist << "||" << pitch_normalised << "\n";
        if (g_calibHeadState.headShoulderNeutralY -pitch_normalised <  0)
        {
            g_calibHeadState.noseUpY = g_calibHeadState.headShoulderNeutralY - pitch_normalised;
            std::cout << "pitchUp saved: " << g_calibHeadState.noseUpY << "\n";
        }
        else
            std::cout << "nope not good " << g_calibHeadState.headShoulderNeutralY - pitch_normalised << "\n";
        break;
    case 's':

        if (g_calibHeadState.headShoulderNeutralY - pitch_normalised > 0)
        {
            //std::cout << "pitchDown saved " << upvsdowndist << "||" << pitch_normalised << "\n";
            g_calibHeadState.noseDownY = g_calibHeadState.headShoulderNeutralY - pitch_normalised;
            std::cout << "pitchDown saved: " << g_calibHeadState.noseDownY << "\n";
        }
        else
            std::cout << "nope not good " << g_calibHeadState.headShoulderNeutralY - pitch_normalised << "\n";
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

    float shoulderAvg = (ls.y + rs.y) / 2;
    float upvsdowndist = shoulderAvg - nose.y;
    float pitch_normalised = g_calibHeadState.headShoulderNeutralY - (upvsdowndist / shoulderAvg);

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

