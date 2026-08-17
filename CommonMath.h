#pragma once
#include "Keypoint.h" // to include keypoint struct



class CommonMath
{
public:
	double EuclDist(const Keypoint& pt1, const Keypoint& pt2);
    double Angle(const Keypoint& A, const Keypoint& B, const Keypoint& C);
	double SignedAngle(const Keypoint& A, const Keypoint& B, const Keypoint& C);
};