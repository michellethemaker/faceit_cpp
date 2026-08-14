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

    //bodystate.leftArmUp = ls.confidence > 0.5f && lw.confidence > 0.5f && lw.y < ls.y;
    //bodystate.rightArmUp = rs.confidence > 0.5f && rw.confidence > 0.5f && rw.y < rs.y;

    
    /*if (leye.x < nose.x)
    {
        std::cout << "nose left peak \n";
        headstate.headXcoord = 1.0;
    }
    else if (reye.x > nose.x)
    {
        std::cout << "nose right peak \n";
        headstate.headXcoord = 0.0;
    }
    else
    headstate.headXcoord = (nose.x - leye.x) /(reye.x - leye.x) ;*/
    //x coords
    float yawLeftMax = g_calibHeadState.noseLeftX;
    float yawRightMax = g_calibHeadState.noseRightX;
    std::cout <<                            yawLeftMax << "||" << yawRightMax << "\n";
    float yawRaw = nose.x;
    float x = (yawRaw - yawLeftMax) / (yawRightMax - yawLeftMax);
    headstate.headXcoord = std::clamp(x, 0.0f, 1.0f);


    //y coords
    float pitchUpMax = g_calibHeadState.noseUpY;
    float pitchDownMax = g_calibHeadState.noseDownY;
    std::cout << pitchUpMax<<"||"<<pitchDownMax << "\n";
    float pitchRaw = nose.y;
    float y = (pitchRaw - pitchUpMax) / (pitchDownMax - pitchUpMax);
    headstate.headYcoord = std::clamp(y, 0.0f, 1.0f);
    //if (reye.y > lear.y || leye.y > lear.y)
    //{
    //    std::cout << "BOTTOM MAXED\n";
    //    headstate.headYcoord = 1.0;
    //}
    //else if (nose.y < leye.y || nose.y < reye.y)
    //{
    //    std::cout << "TOP MAXED\n";
    //    headstate.headYcoord = 0.0;
    //}
    //else
    //{
    //    float eyeAvgY = (leye.y + reye.y) * 0.5f;
    //    float noseEyeY = nose.y - eyeAvgY;
    //    float eyeDist = commonmath.EuclDist(
    //        leye.x, reye.x,
    //        leye.y, reye.y
    //    );
    //    float pitch = noseEyeY / eyeDist; //norm by face scale
    //    float lr_ratio = commonmath.EuclDist(
    //        leye.x, nose.x, 
    //        leye.y, nose.y) / 
    //        (commonmath.EuclDist(
    //        reye.x, nose.x, 
    //        reye.y, nose.y));
    //    float yawCorrection = (lr_ratio + 1.0f / lr_ratio) * 0.5f; // turn lr_ratio symmetrical across origin
    //    pitch /= yawCorrection;

    //    
    //    headstate.headYcoord = pitch;
    //}
    //headstate.headYcoord = reye.y;
    headstate.tempX = nose.x;
    headstate.tempY = nose.y;
    return headstate;
}

