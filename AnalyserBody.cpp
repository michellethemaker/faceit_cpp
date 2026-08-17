#include "AnalyserBody.h"
#include <cmath>
#include <iostream>

PSBodyState AnalyserBody::analyseBody(const AllKeypoints& keypoint)
{
    PSBodyState bodystate;
    if (keypoint.keypoints.size() < 17)
    {
        bodystate.isVisible = false;
        return bodystate;
    }
        
    else
        bodystate.isVisible = true;
    const auto& ls = keypoint.keypoints[LEFT_SHOULDER];
    const auto& rs = keypoint.keypoints[RIGHT_SHOULDER];
    const auto& lw = keypoint.keypoints[LEFT_WRIST];
    const auto& rw = keypoint.keypoints[RIGHT_WRIST];
    const auto& nose = keypoint.keypoints[NOSE];
    
    const auto& lh = keypoint.keypoints[LEFT_HIP];
    const auto& rh = keypoint.keypoints[RIGHT_HIP];

    const auto& la = keypoint.keypoints[LEFT_ANKLE];
    const auto& ra = keypoint.keypoints[RIGHT_ANKLE];

    bodystate.leftArmUp = ls.confidence > 0.5f && lw.confidence > 0.5f && lw.y < ls.y;
    bodystate.rightArmUp = rs.confidence > 0.5f && rw.confidence > 0.5f && rw.y < rs.y;

    float shoulderMidX = (ls.x + rs.x) * 0.5f;
    bodystate.headLeft = nose.confidence > 0.5f && nose.x > shoulderMidX + 20.0f;
    bodystate.headRight = nose.confidence > 0.5f && nose.x < shoulderMidX - 20.0f;

    bodystate.toot = rs.confidence > 0.5f && rw.confidence > 0.5f && std::abs(rs.x - rw.x)>100.0f ;

    bodystate.leftLegUp = (ra.y - la.y)/(lh.x-rh.x) > 0.3;
    bodystate.rightLegUp = (la.y - ra.y) / (lh.x - rh.x) > 0.3;
    //if (bodystate.leftLegUp) std::cout << "<<<<<<<LEFT\n";
    //if (bodystate.rightLegUp) std::cout << "RIGHT>>>>>>>>\n";

    /*if (leftArmUp && rightArmUp) return "both arms up";
    if (leftArmUp) return "left arm up";
    */

    return bodystate;
}