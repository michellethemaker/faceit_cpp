#include "AnalyserBody.h"
#include <opencv2/opencv.hpp> // for ROI of hands
#include <cmath>
#include "CommonMath.h"
#include <iostream>

PSBodyState AnalyserBody::analyseBody(const AllKeypoints& keypoint)
{
    PSBodyState bodystate;
    if (keypoint.keypoints.size() < 17)
    {
        bodystate.isVisible = false;
        return bodystate;
        return bodystate;
    }
        
    else
        bodystate.isVisible = true;

    const auto& ls = keypoint.keypoints[LEFT_SHOULDER];
    const auto& rs = keypoint.keypoints[RIGHT_SHOULDER];
    const auto& le = keypoint.keypoints[LEFT_ELBOW];
    const auto& re = keypoint.keypoints[RIGHT_ELBOW];
    const auto& lw = keypoint.keypoints[LEFT_WRIST];
    const auto& rw = keypoint.keypoints[RIGHT_WRIST];
    const auto& nose = keypoint.keypoints[NOSE];
    
    const auto& lh = keypoint.keypoints[LEFT_HIP];
    const auto& rh = keypoint.keypoints[RIGHT_HIP];

    const auto& lk = keypoint.keypoints[LEFT_KNEE];
    const auto& rk = keypoint.keypoints[RIGHT_KNEE];

    const auto& la = keypoint.keypoints[LEFT_ANKLE];
    const auto& ra = keypoint.keypoints[RIGHT_ANKLE];
    //TO DO: SET SHOULDER WIDTH AS NORMALISING DISTANCE FOR ALL RATIO-BASED CHECKS
    //TO DO: REORGANISE CONFIDENCE SANITY CHECKS FOR OPTIMISATION

    bodystate.leftArmUp = ls.confidence > 0.5f && lw.confidence > 0.5f && lw.y < ls.y;
    bodystate.rightArmUp = rs.confidence > 0.5f && rw.confidence > 0.5f && rw.y < rs.y;

    if (ls.confidence > 0.5f && rs.confidence > 0.5f)
    {
        float shoulderMidX = (ls.x + rs.x) * 0.5f;
        bodystate.headLeft = nose.confidence > 0.5f && nose.x > shoulderMidX + 20.0f;
        bodystate.headRight = nose.confidence > 0.5f && nose.x < shoulderMidX - 20.0f;
    }
    

    bodystate.toot = rs.confidence > 0.5f && rw.confidence > 0.5f && std::abs(rs.x - rw.x)>100.0f ;

    if (lh.confidence > 0.5f && rh.confidence > 0.5f && ls.confidence > 0.5f && rs.confidence > 0.5f)
    {
        
        //   ========== LEANING LOGIC ==========
        bodystate.left = cmath.Angle(ls, lh, rh) > 1.8;
        bodystate.right = cmath.Angle(rs, rh, lh) > 1.8;
        if (la.confidence > 0.5f && ra.confidence > 0.5f && lk.confidence > 0.5f && rk.confidence > 0.5f && lh.confidence > 0.5f && rh.confidence > 0.5f)
        {
            //std::cout << (la.y - lk.y) / (la.y - lh.y) << "\n";
            bodystate.leftLegUp = (ra.y - la.y) / (lh.x - rh.x) > 0.24;
            bodystate.rightLegUp = (la.y - ra.y) / (lh.x - rh.x) > 0.24;
            if (bodystate.leftLegUp)
            {
                //std::cout << "LEFT \n";
            }
            if (bodystate.rightLegUp)
            {
                //std::cout << "RIGJT \n";
            }
            //   ========== CROUCH LOGIC ==========
            if (ls.confidence > 0.5f && rs.confidence > 0.5f)
            {
                bodystate.crouching = (cmath.EuclDist(ls, la) / cmath.EuclDist(lh, la) > 2) && (cmath.EuclDist(rs, ra) / cmath.EuclDist(rh, ra) > 2);
                
                //===== JUMP LOGIC =====
                bodystate.currAvgHips = ((lh.y + rh.y) / 2) / (lh.x - rh.x);
                bodystate.currAvgShoulders = ((ls.y + rs.y) / 2) / (ls.x - rs.x);
                //std::cout << bodystate.currAvgHips << "\n";
            }

            //std::cout << cmath.EuclDist(ls, la) / cmath.EuclDist(lh, la) << "|" << cmath.EuclDist(ls, la) / cmath.EuclDist(lk, la) << "\n"; 
            
        }
    }


//   ========== HANDS ROI LOGIC ==========

    float radius = 300; // TODO: scale this to shoulder width(increases computation, leave as is if possible)

    if (lw.confidence > 0.5f)
    {
        bodystate.leftWristXcoord = lw.x;
        bodystate.leftWristYcoord = lw.y;

        float lx = static_cast<float>(lw.x);
        float ly = static_cast<float>(lw.y);

        cv::Rect leftROI(lx - radius, ly - radius,
            2 * radius, 2 * radius);

        bodystate.leftHandROI = leftROI;

        // ===== GRAB LSHOULDER =====
        if (ls.confidence > 0.5f && le.confidence > 0.5f)
        {
            //std::cout << cmath.Angle(ls,le,lw)<< "\n";
            bodystate.grabLeftShoulder = cmath.Angle(ls, le, lw) < 0.5;
        }

        // ===== RELOAD MOTION =====
        if (rw.confidence > 0.5f && ls.confidence > 0.5f && rs.confidence > 0.5f)
        {
            if (abs(lw.x - rw.x)/cmath.EuclDist(ls,rs) < 0.4 && abs(lw.y - rw.y) / cmath.EuclDist(ls, rs) < 0.1)
            {
                bodystate.leftOverRightHand = true;
            }

            // ===== E TRIGGER =====
            if (abs(ls.x - rw.x) / cmath.EuclDist(ls, rs) < 0.5 && abs(ls.y - rw.y) / cmath.EuclDist(ls, rs) < 0.1)
            {
                bodystate.rightOverLeftShoulder = true;
            }
        }
    }
    
    if (rw.confidence > 0.5f) // has right hand
    {
        bodystate.rightWristXcoord = rw.x;
        bodystate.rightWristYcoord = rw.y;

        float rx = static_cast<float>(rw.x);
        float ry = static_cast<float>(rw.y);

        cv::Rect rightROI(rx - radius, ry - radius,
            2 * radius, 2 * radius);

        bodystate.rightHandROI = rightROI;

        //=====GRAB RSHOULDER=====
        if (rs.confidence > 0.5f && re.confidence > 0.5f)
        {
            bodystate.grabRightShoulder = cmath.Angle(rs, re, rw) < 0.5;
        }
    }
    return bodystate;
}