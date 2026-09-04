#include "CommonMath.h"
#include <cmath>

CommonMath cmath; // for the extern

double CommonMath::EuclDist(const Keypoint& pt1, const Keypoint& pt2) //pass by reference! no copies, cheaper!! 
{   
	return (sqrt(pow((pt2.x - pt1.x), 2) + pow((pt2.y - pt1.y), 2)));
}

double CommonMath::EuclDist(const KeypointHand& pt1, const KeypointHand& pt2) //pass by reference! no copies, cheaper!! 
{
    return (sqrt(pow((pt2.x - pt1.x), 2) + pow((pt2.y - pt1.y), 2)));
}

double CommonMath::Angle(const Keypoint& A, const Keypoint& B, const Keypoint& C)
{
    double BAx = A.x - B.x;
    double BAy = A.y - B.y;

    double BCx = C.x - B.x;
    double BCy = C.y - B.y;

    double dot = BAx * BCx + BAy * BCy;

    double lenBA = std::hypot(BAx, BAy);
    double lenBC = std::hypot(BCx, BCy);

    return std::acos(dot / (lenBA * lenBC));
}

double CommonMath::Angle(const KeypointHand& A, const KeypointHand& B, const KeypointHand& C)
{
    double BAx = A.x - B.x;
    double BAy = A.y - B.y;

    double BCx = C.x - B.x;
    double BCy = C.y - B.y;

    double dot = BAx * BCx + BAy * BCy;

    double lenBA = std::hypot(BAx, BAy);
    double lenBC = std::hypot(BCx, BCy);

    return std::acos(dot / (lenBA * lenBC));
}

double CommonMath::SignedAngle(const Keypoint& A, const Keypoint& B, const Keypoint& C)
{
    double BAx = A.x - B.x;
    double BAy = A.y - B.y;

    double BCx = C.x - B.x;
    double BCy = C.y - B.y;

    double cross = BAx * BCy - BAy * BCx;
    double dot = BAx * BCx + BAy * BCy;

    return std::atan2(cross, dot) ; //* 180.0 / 3.14159265358979323846
}

double CommonMath::SignedAngle(const KeypointHand& A, const KeypointHand& B, const KeypointHand& C)
{
    double BAx = A.x - B.x;
    double BAy = A.y - B.y;

    double BCx = C.x - B.x;
    double BCy = C.y - B.y;

    double cross = BAx * BCy - BAy * BCx;
    double dot = BAx * BCx + BAy * BCy;

    return std::atan2(cross, dot); //* 180.0 / 3.14159265358979323846
}