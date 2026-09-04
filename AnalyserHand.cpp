#include "AnalyserHand.h"
#include "CommonMath.h"
#include <iostream>

PSHandState AnalyserHand::analyseHand(const AllHandKeypoints& keypoint)
{
    PSHandState handstate;

    if (keypoint.keypointshand.size() < 17)
    { 
        handstate.isVisible = false;
        return handstate;
    }

    else
        handstate.isVisible = true;
    const auto& wrist = keypoint.keypointshand[WRIST];
    const auto& thumbtip = keypoint.keypointshand[THUMB_TIP];
    const auto& thumbbase = keypoint.keypointshand[THUMB_BASE];

    const auto& indextip = keypoint.keypointshand[INDEX_TIP];
    const auto& indexbase = keypoint.keypointshand[INDEX_BASE];
    
    const auto& middletip = keypoint.keypointshand[MIDDLE_TIP];
    const auto& middlebase = keypoint.keypointshand[MIDDLE_BASE];
    
    const auto& ringtip = keypoint.keypointshand[RING_TIP];
    const auto& ringbase = keypoint.keypointshand[RING_BASE];
    const auto& pinkytip = keypoint.keypointshand[PINKY_TIP];
    const auto& pinkybase = keypoint.keypointshand[PINKY_BASE];

    if (thumbtip.confidence > 0.25f && thumbbase.confidence > 0.25f && indextip.confidence > 0.25f)
    {
        float angleGun = cmath.SignedAngle(thumbtip, thumbbase, indextip);
        //std::cout << angleGun << "\n";
    }
    
    //TODO: add check that palm is UP
    handstate.indexDown = indextip.confidence > 0.25f && indexbase.confidence > 0.25f && 
        cmath.EuclDist(indexbase, wrist) / cmath.EuclDist(indextip, wrist) > 0.8; //open>0.5, closed <1
    handstate.middleDown = middletip.confidence > 0.25f && middlebase.confidence > 0.25f &&
        cmath.EuclDist(middlebase, wrist) / cmath.EuclDist(middletip, wrist) > 0.8;
    handstate.ringDown = ringtip.confidence > 0.25f && ringbase.confidence > 0.25f &&
        cmath.EuclDist(ringbase, wrist) / cmath.EuclDist(ringtip, wrist) > 0.8;
    handstate.pinkyDown = pinkytip.confidence > 0.25f && pinkybase.confidence > 0.25f &&
        cmath.EuclDist(pinkybase, wrist) / cmath.EuclDist(pinkytip, wrist) > 0.8;
    //std::cout << cmath.EuclDist(indexbase, wrist) / cmath.EuclDist(indextip, wrist) << "\n";
    //std::cout << cmath.EuclDist(ringbase, wrist) / cmath.EuclDist(ringtip, wrist) << "\n===============\n";
    if (handstate.middleDown && handstate.ringDown && handstate.pinkyDown)
    {
        handstate.closedFist = handstate.middleDown && handstate.ringDown && handstate.pinkyDown;
        std::cout << "CLOSED FIST\n";
    }

    return handstate;
}