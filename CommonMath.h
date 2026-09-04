#pragma once
#include "Keypoint.h" // to include keypoint struct
#include "KeypointHand.h"



class CommonMath
{
public:
	double EuclDist(const Keypoint& pt1, const Keypoint& pt2);
	double EuclDist(const KeypointHand& pt1, const KeypointHand& pt2);
    double Angle(const Keypoint& A, const Keypoint& B, const Keypoint& C);
	double Angle(const KeypointHand& A, const KeypointHand& B, const KeypointHand& C);
	double SignedAngle(const Keypoint& A, const Keypoint& B, const Keypoint& C);
	double SignedAngle(const KeypointHand& A, const KeypointHand& B, const KeypointHand& C);
};

extern CommonMath cmath; //the proper way to allow it to be used by other files