#include "AnalyserHand.h"

PSHandState AnalyserHand::analyseHand(const AllKeypoints& keypoint)
{
    PSHandState handstate;

    //if (keypoint.keypoints.size() < 17)
    //{
    //    handstate.isVisible = false;
    //    return bodystate;
    //}

    //else
    //    handstate.isVisible = true;
    const auto& lw = keypoint.keypoints[LEFT_WRIST];
    const auto& rw = keypoint.keypoints[RIGHT_WRIST];

    /*handstate.leftArmUp = ls.confidence > 0.5f && lw.confidence > 0.5f && lw.y < ls.y;
    handstate.rightArmUp = rs.confidence > 0.5f && rw.confidence > 0.5f && rw.y < rs.y;

    float shoulderMidX = (ls.x + rs.x) * 0.5f;
    bodystate.headLeft = nose.confidence > 0.5f && nose.x > shoulderMidX + 20.0f;
    bodystate.headRight = nose.confidence > 0.5f && nose.x < shoulderMidX - 20.0f;

    bodystate.toot = rs.confidence > 0.5f && rw.confidence > 0.5f && std::abs(rs.x - rw.x) > 100.0f;*/
    //std::cout << "TOOT: " << bodystate.toot <<"\n";
    /*if (leftArmUp && rightArmUp) return "both arms up";
    if (leftArmUp) return "left arm up";
    if (rightArmUp) return "right arm up";
    if (headLeft) return "head left";
    if (headRight) return "head right";*/

    return handstate;
}