#include "BodyAnalyser.h"
#include <cmath>

BodyState BodyAnalyser::analyseBody(const AllKeypoints& keypoint)
{
    BodyState bodystate;
    if (keypoint.keypoints.size() < 17)
        return bodystate;

    const auto& ls = keypoint.keypoints[LEFT_SHOULDER];
    const auto& rs = keypoint.keypoints[RIGHT_SHOULDER];
    const auto& lw = keypoint.keypoints[LEFT_WRIST];
    const auto& rw = keypoint.keypoints[RIGHT_WRIST];
    const auto& nose = keypoint.keypoints[NOSE];

    bodystate.leftArmUp = ls.confidence > 0.5f && lw.confidence > 0.5f && lw.y < ls.y;
    bodystate.rightArmUp = rs.confidence > 0.5f && rw.confidence > 0.5f && rw.y < rs.y;

    float shoulderMidX = (ls.x + rs.x) * 0.5f;
    bodystate.headLeft = nose.confidence > 0.5f && nose.x > shoulderMidX + 20.0f;
    bodystate.headRight = nose.confidence > 0.5f && nose.x < shoulderMidX - 20.0f;

    /*if (leftArmUp && rightArmUp) return "both arms up";
    if (leftArmUp) return "left arm up";
    if (rightArmUp) return "right arm up";
    if (headLeft) return "head left";
    if (headRight) return "head right";*/

    return bodystate;
}