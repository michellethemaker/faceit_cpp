#include "GestureDetector.h"
#include <cmath>

std::string GestureDetector::detectGesture(const AllKeypoints& keypoint)
{
    if (keypoint.keypoints.size() < 17)
        return "unknown";

    const auto& ls = keypoint.keypoints[LEFT_SHOULDER];
    const auto& rs = keypoint.keypoints[RIGHT_SHOULDER];
    const auto& lw = keypoint.keypoints[LEFT_WRIST];
    const auto& rw = keypoint.keypoints[RIGHT_WRIST];
    const auto& nose = keypoint.keypoints[NOSE];
    const auto& leye = keypoint.keypoints[LEFT_EYE];
    const auto& reye = keypoint.keypoints[RIGHT_EYE];

    bool leftArmUp = ls.confidence > 0.5f && lw.confidence > 0.5f && lw.y < ls.y;
    bool rightArmUp = rs.confidence > 0.5f && rw.confidence > 0.5f && rw.y < rs.y;

    float shoulderMidX = (ls.x + rs.x) * 0.5f;
    bool headLeft = nose.confidence > 0.5f && nose.x < shoulderMidX - 20.0f;
    bool headRight = nose.confidence > 0.5f && nose.x > shoulderMidX + 20.0f;

    if (leftArmUp && rightArmUp) return "both arms up";
    if (leftArmUp) return "left arm up";
    if (rightArmUp) return "right arm up";
    if (headLeft) return "head left";
    if (headRight) return "head right";

    return "neutral";
}