#include "AnalyserBody.h"
#include "CommonMath.h"
#include "AppConfig.h"
#include <opencv2/opencv.hpp> // for ROI of hands
#include <cmath>
#include <iostream>

PSCalibrateBodyState AnalyserBody::calibrateBody(const AllKeypoints& keypoint, char c)
{
    //PSCalibrateHeadState calibrateheadstate;
    CommonMath cmath;
    //if (keypoint.keypoints.size() < 17)
    //    return g_calibHeadState;

    const auto& ls = keypoint.keypoints[LEFT_SHOULDER];
    const auto& rs = keypoint.keypoints[RIGHT_SHOULDER];
    const auto& le = keypoint.keypoints[LEFT_ELBOW];
    const auto& re = keypoint.keypoints[RIGHT_ELBOW];
    const auto& lw = keypoint.keypoints[LEFT_WRIST];
    const auto& rw = keypoint.keypoints[RIGHT_WRIST];
    const auto& nose = keypoint.keypoints[NOSE];
    const auto& mouth = keypoint.keypoints[MOUTH];

    const auto& lh = keypoint.keypoints[LEFT_HIP];
    const auto& rh = keypoint.keypoints[RIGHT_HIP];

    const auto& lk = keypoint.keypoints[LEFT_KNEE];
    const auto& rk = keypoint.keypoints[RIGHT_KNEE];

    const auto& la = keypoint.keypoints[LEFT_ANKLE];
    const auto& ra = keypoint.keypoints[RIGHT_ANKLE];

    float calibHipWidth = 0.0f; 

    if (ls.confidence > 0.5f && rs.confidence > 0.5f)
        calibHipWidth = cmath.EuclDist(lh,rh);
    switch (c)
    {
    case 'q': //NEUTRAL POSITION
        if (le.confidence > 0.5f && re.confidence > 0.5f &&
            lw.confidence > 0.5f && rw.confidence > 0.5f &&
            lh.confidence > 0.5f && rh.confidence > 0.5f &&
            lk.confidence > 0.5f && rk.confidence > 0.5f &&
            la.confidence > 0.5f && ra.confidence > 0.5f
            )
        {
            g_calibBodyState.hipWidth = calibHipWidth;
            g_calibBodyState.shoulderElbowLength = cmath.EuclDist(ls, le) ;
            g_calibBodyState.elbowWristLength = cmath.EuclDist(le, lw) ;
            g_calibBodyState.hipKneeLength = cmath.EuclDist(lh, lk) ;
            g_calibBodyState.kneeAnkleLength = cmath.EuclDist(lk, la) ;
            g_calibBodyState.hipKneeAnkleRatio = g_calibBodyState.hipKneeLength / g_calibBodyState.kneeAnkleLength;

            std::cout << "NEUTRAL RATIOS " << g_calibBodyState.hipWidth << "\n" << 
                                              g_calibBodyState.shoulderElbowLength << "\n" <<
                                              g_calibBodyState.elbowWristLength << "\n" <<
                                              g_calibBodyState.hipKneeLength << "\n" <<
                                              g_calibBodyState.kneeAnkleLength << "\n" <<
                                              "^^^^^^^^^\n";
        }
        else
            std::cout << "NOT ALL KEYPOINTS IN FRAME\n";
        break;
    case 'p':
        std::cout << "CALIB DONE\n";
        return g_calibBodyState;
    };
    return g_calibBodyState;
}
PSBodyState AnalyserBody::analyseBody(const AllKeypoints& keypoint)
{
    PSBodyState bodystate;
    //PSHandState handstate; //TODO: maybe use handstates here now (w/o threading. just to separate the keypoints/calcs)
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
    const auto& mouth = keypoint.keypoints[MOUTH];
    
    const auto& lh = keypoint.keypoints[LEFT_HIP];
    const auto& rh = keypoint.keypoints[RIGHT_HIP];

    const auto& lk = keypoint.keypoints[LEFT_KNEE];
    const auto& rk = keypoint.keypoints[RIGHT_KNEE];

    const auto& la = keypoint.keypoints[LEFT_ANKLE];
    const auto& ra = keypoint.keypoints[RIGHT_ANKLE];

    const auto& lt = keypoint.keypoints[LEFT_THUMB];
    const auto& li = keypoint.keypoints[LEFT_INDEX];
    const auto& lp = keypoint.keypoints[LEFT_PINKY];

    const auto& lt_2 = keypoint.keypoints[LEFT_THUMB_2];
    const auto& li_2 = keypoint.keypoints[LEFT_INDEX_2];
    const auto& lp_2 = keypoint.keypoints[LEFT_PINKY_2];


    const auto& lt_b = keypoint.keypoints[LEFT_THUMB_BASE];
    const auto& li_b = keypoint.keypoints[LEFT_INDEX_BASE];
    const auto& lp_b = keypoint.keypoints[LEFT_PINKY_BASE];

    const auto& rt = keypoint.keypoints[RIGHT_THUMB];
    const auto& ri = keypoint.keypoints[RIGHT_INDEX];
    const auto& rp = keypoint.keypoints[RIGHT_PINKY];

    const auto& rt_2 = keypoint.keypoints[RIGHT_THUMB_2];
    const auto& ri_2 = keypoint.keypoints[RIGHT_INDEX_2];
    const auto& rp_2 = keypoint.keypoints[RIGHT_PINKY_2];

    
    const auto& rt_b = keypoint.keypoints[RIGHT_THUMB_BASE];
    const auto& ri_b = keypoint.keypoints[RIGHT_INDEX_BASE];
    const auto& rp_b = keypoint.keypoints[RIGHT_PINKY_BASE];

    g_lArmExtended = false; // hardset false every call to ensure it's not hardstuck true if limb isnt ard...
    //TO DO: REORGANISE CONFIDENCE SANITY CHECKS FOR OPTIMISATION

    if (ls.confidence > 0.5f && rs.confidence > 0.5f)
    {
        bodystate.currHipWidth = cmath.EuclDist(lh, rh);
        bodystate.currShoulderWidth = cmath.EuclDist(ls, rs);
        // TODO: shouldnt i use currShouldWidth too? for torso-up uses
    }
    
    bodystate.leftArmUp = nose.confidence > 0.5f && lw.confidence > 0.5f && ls.confidence > 0.5f && lw.y < nose.y && lw.x < ls.x;
    bodystate.rightArmUp = nose.confidence > 0.5f && rw.confidence > 0.5f && rs.confidence > 0.5f && rw.y < nose.y && rw.x > rs.x;

    if (ls.confidence > 0.5f && rs.confidence > 0.5f)
    {
        float shoulderMidX = (ls.x + rs.x) * 0.5f;
        bodystate.headLeft = nose.confidence > 0.5f && nose.x > shoulderMidX + 20.0f;
        bodystate.headRight = nose.confidence > 0.5f && nose.x < shoulderMidX - 20.0f;

        bodystate.hasLeftShoulder = true;
        bodystate.hasRightShoulder = true;
        // =====WRIST MOUSE MOVEMENT=====
        if (rw.confidence > 0.5f)
        {
            bodystate.rightWristUD_val = rw.y - rs.y;
            bodystate.rightWristLR_val = rw.x - rs.x;
            
            if (abs(bodystate.rightWristUD_val) < AppConfigBody::DEADZONE_WRIST_MIN  || 
                abs(bodystate.rightWristUD_val) > AppConfigBody::DEADZONE_WRIST_MAX)
            {
                bodystate.rightWristUD_val = 0.0f;
            }
            else
            {
                //std::cout << abs(bodystate.rightWristUD_val) << "\n";
                bodystate.rightWristUD_val = AppConfigBody::SCALEFACTOR_LR * bodystate.rightWristUD_val;
            }
            
            if (abs(bodystate.rightWristLR_val) < AppConfigBody::DEADZONE_WRIST_MIN || 
                abs(bodystate.rightWristLR_val) > AppConfigBody::DEADZONE_WRIST_MAX)
            {
                bodystate.rightWristLR_val = 0.0f;
            }
            else
            {
                bodystate.rightWristLR_val = AppConfigBody::SCALEFACTOR_UD * bodystate.rightWristLR_val;
            }
                
            // =====RIGHT FINGERS CALCULATION=====
            if (rt_b.y < rw.y  && rt_b.confidence > 0.5f) //UPRIGHT 
            { 
                //std::cout << cmath.Angle(rt, rt_b, ri_b) / (cmath.EuclDist(rt, rt_b)) << "\n";
                if (cmath.Angle(rt,rt_b,ri_b)/ (cmath.EuclDist(rt,rt_b)) < AppConfigBody::RTHUMB_CLOSE_MAX && rt.confidence > 0.5f && rt_b.confidence > 0.5f && ri_b.confidence > 0.5f) // THUMB IS STICKY, ANGLES WORK BETTER
                {
                    bodystate.rightThumbClosed = true;
                    //std::cout << "         RTHUMB\n";
                }
                if (ri_2.y < ri.y && ri_2.confidence > 0.5f && ri.confidence > 0.5f)
                {
                    bodystate.rightIndexClosed = true;
                }
                if (rp_2.y < rp.y && rp_2.confidence > 0.5f )
                {
                    bodystate.rightPinkyClosed = true;
                }
                //std::cout << bodystate.thumbClosed << "|" << bodystate.indexClosed << "|" << bodystate.pinkyClosed << "\n";
                bodystate.rightClosed = bodystate.rightThumbClosed && 
                                        bodystate.rightIndexClosed && 
                                        bodystate.rightPinkyClosed;
                
                if (bodystate.rightClosed)
                {
                    //std::cout << "***R HAND CLOSE***\n";
                }
            }
            // ==========R ELBOW LOGICS (INCL. R WRIST CONF)==========
            if (re.confidence > 0.5f)
            {
                // L CONSTANTINE
                if (re.x < rs.x && rw.y < rs.y)
                {
                    std::cout << "RCONSTANTINE\n";
                    bodystate.rConstantine = true;
                }
                bodystate.rElbowHang = (abs(rs.y - re.y) < 1.5f) && cmath.SignedAngle(rs, re, rw) < AppConfigBody::RELBOW_ANGLE_MAX && cmath.SignedAngle(rs, re, rw) > AppConfigBody::RELBOW_ANGLE_MIN;
                //std::cout << "}{" << cmath.SignedAngle(rs, re, rw) << "\n";
                if (bodystate.rElbowHang)
                    std::cout << "   RELB HANG\n";
            }
        }

        //==========L WRIST LOGICS==========
        if (lw.confidence > 0.5f)
        {
            // =====LEFT WRIST COVER MOUTH=====
            bodystate.leftCoverMouth = mouth.confidence > 0.5f && cmath.EuclDist(mouth, lw) / bodystate.currShoulderWidth < AppConfigBody::LEFTCOVERMOUTH_MAX;
            // =====LEFT FINGERS CALCULATION=====
            if (lt_b.y < lw.y && lt_b.confidence > 0.5f) //UPRIGHT 
            {
                //std::cout << cmath.Angle(lt, lt_b, li_b) << "\n";
                if ((cmath.Angle(lt, lt_b, li_b) / (cmath.EuclDist(lt, lt_b)))< AppConfigBody::LTHUMB_CLOSE_MAX && lt.confidence > 0.5f && lt_b.confidence > 0.5f && li_b.confidence > 0.5f) // THUMB IS STICKY, ANGLES WORK BETTER
                {
                    bodystate.leftThumbClosed = true;
                    //std::cout << "RCLICK\n";
                }
                if (li_2.y < li.y && li_2.confidence > 0.5f && li.confidence > 0.5f)
                {
                    bodystate.leftIndexClosed = true;
                    //std::cout << "LCLICK\n";
                }
                if (lp_2.y < lp.y && lp_2.confidence > 0.5f)
                {
                    bodystate.leftPinkyClosed = true;
                }
                //std::cout << bodystate.leftPinkyClosed << "|" << bodystate.leftIndexClosed << "|" << bodystate.leftThumbClosed << "\n";
                bodystate.leftClosed = bodystate.leftThumbClosed &&
                                       bodystate.leftIndexClosed &&
                                       bodystate.leftPinkyClosed;
                if (bodystate.leftIndexClosed)
                {
                    //std::cout << "**LINDEX CLOSE\n";
                }
                if (bodystate.leftThumbClosed)
                {
                    //std::cout << "     **LTHUMB CLOSE\n";
                }
                if (bodystate.leftClosed)
                {
                    //std::cout << "***L HAND CLOSE***\n";
                }
            }

            // ==========L ELBOW LOGICS (INCL. L WRIST CONF)==========
            if (le.confidence > 0.5f) 
            {
                // L CONSTANTINE
                if (le.x > ls.x && lw.y < ls.y)
                {
                    std::cout << "LCONSTANTINE\n";
                    bodystate.lConstantine = true;
                }

                bodystate.lElbowHang = (abs(ls.y - le.y) < 1.5f) && cmath.SignedAngle(ls, le, lw) > AppConfigBody::LELBOW_ANGLE_MIN && cmath.SignedAngle(ls, le, lw) < AppConfigBody::LELBOW_ANGLE_MAX;
                //std::cout << cmath.SignedAngle(ls, le, lw) << "\n";
                //if (bodystate.lElbowHang)
                //    std::cout << "LELB HANG\n";
                //bodystate.lElbowKick = (ls.y - le.y) / bodystate.currHipWidth > AppConfigBody::LELBOW_KICK_ELBOW && cmath.EuclDist(ls, lw) / bodystate.currHipWidth < AppConfigBody::LELBOW_KICK_WRIST;
                ////std::cout << cmath.EuclDist(ls, lw) / bodystate.currHipWidth << "\n";
                ////std::cout << bodystate.lElbowKick << "\n";
                //if (bodystate.lElbowKick)
                //    std::cout << "LELB KIIICK\n";
                bodystate.leftArmExtended = cmath.EuclDist(lw, ls) < bodystate.currShoulderWidth * 0.8;
                if (bodystate.leftArmExtended)
                {
                    g_lArmExtended = true;
                    bodystate.leftIndex = cmath.EuclDist(li, li_b);
                    //std::cout <<cmath.EuclDist(li, li_b)/ bodystate.currShoulderWidth << "EXTENDED\n";
                }
                else
                    g_lArmExtended = false;
                    
            }
        }
    }
    
    bodystate.toot = rs.confidence > 0.5f && rw.confidence > 0.5f && std::abs(rs.x - rw.x)>100.0f ;

    if (lh.confidence > 0.5f && rh.confidence > 0.5f && ls.confidence > 0.5f && rs.confidence > 0.5f)
    {
        
        //   ========== LEANING LOGIC ==========
        bodystate.leaningLeft = cmath.Angle(ls, lh, rh) > AppConfigBody::LEAN_MIN;
        bodystate.leaningRight = cmath.Angle(rs, rh, lh) > AppConfigBody::LEAN_MIN;
        //std::cout << cmath.Angle(ls, lh, rh) <<"|="<< cmath.Angle(rs, rh, lh) << "\n";
        //   ========== WALKING LOGIC ==========
        if (la.confidence > 0.5f && ra.confidence > 0.5f && lk.confidence > 0.5f && rk.confidence > 0.5f && lh.confidence > 0.5f && rh.confidence > 0.5f)
        {
            //std::cout << (ra.y - la.y) / (rh.x - lh.x) << "||" << (la.y - ra.y) / (rh.x - lh.x) << "\n";
            //std::cout << cmath.Angle(lh, lk, la) << " angle\n";
            //g_calibBodyState.hipKneeLength 
            //g_calibBodyState.kneeAnkleLength;
            float hipKneeLength = cmath.EuclDist(lh, lk);
            float kneeAnkleLength = cmath.EuclDist(lk, la);
            //std::cout << hipKneeLength / kneeAnkleLength << "[]" << g_calibBodyState.hipKneeLength / g_calibBodyState.kneeAnkleLength << "\n";
            
            bodystate.leftLegUp = cmath.EuclDist(lh, lk)/ cmath.EuclDist(lk, la) < g_calibBodyState.hipKneeAnkleRatio * 0.92 ;// add small deadzone //cmath.Angle(lh, lk, la) < AppConfigBody::LLEG_MAX;
            bodystate.rightLegUp = cmath.EuclDist(rh, rk) / cmath.EuclDist(rk, ra) < g_calibBodyState.hipKneeAnkleRatio * 0.92 ;// add small deadzone//cmath.Angle(rh, rk, ra) < AppConfigBody::RLEG_MAX;
            if (bodystate.leftLegUp)
            {
                //std::cout << "LEFTLEG \n";
            }
            if (bodystate.rightLegUp)
            {
                //std::cout << "    RIGxJTLEG \n";
            }
            //   ========== CROUCH LOGIC. shoulder-ankle / hip-ankle. TODO: calibrate that limit ==========
            if (ls.confidence > 0.5f && rs.confidence > 0.5f)
            {
                bodystate.crouching = (cmath.EuclDist(ls, lh) / cmath.EuclDist(ls, la) < AppConfigBody::CROUCH_MAX) && (cmath.EuclDist(rs, rh) / cmath.EuclDist(rs, ra) < AppConfigBody::CROUCH_MAX);
                if (bodystate.crouching)
                {
                    std::cout << (cmath.EuclDist(ls, lh) / cmath.EuclDist(ls, la)) << "==========\n";
                }
                //std::cout << (cmath.EuclDist(ls, lh) / cmath.EuclDist(ls, la)  ) << " \n";
                // 
                //===== JUMP LOGIC, USES PREV VALS =====
                bodystate.currAvgHips = ((lh.y + rh.y) / 2) / (lh.x - rh.x);
                bodystate.currAvgShoulders = ((ls.y + rs.y) / 2) / (ls.x - rs.x);
                //std::cout << bodystate.currAvgHips << "||" << bodystate.currAvgShoulders << "\n";
            }

            //std::cout << cmath.EuclDist(ls, la) / cmath.EuclDist(lh, la) << "|" << cmath.EuclDist(ls, la) / cmath.EuclDist(lk, la) << "\n"; 
        }
    }

//   ========== HANDS ROI LOGIC ==========

    if (lw.confidence > 0.5f)
    {
        bodystate.leftWristXcoord = lw.x;
        bodystate.leftWristYcoord = lw.y;

        // ===== GRAB LSHOULDER =====
        if (ls.confidence > 0.5f && le.confidence > 0.5f && lw.confidence > 0.5f)
        {
            //std::cout << cmath.Angle(ls,le,lw)<<"|"<< cmath.EuclDist(lw, ls) / n_factor<< "\n";
            bodystate.grabLeftShoulder = cmath.Angle(ls, le, lw) < AppConfigBody::GRABLEFTSHOULDER_MAXANGLE &&
                                         cmath.EuclDist(lw, ls)/cmath.EuclDist(ls,rs)  < AppConfigBody::GRABLEFTSHOULDER_MAXDIST;
        }

        // ===== RELOAD MOTION =====
        if (rw.confidence > 0.5f && ls.confidence > 0.5f && rs.confidence > 0.5f)
        {
            if (abs(lw.x - rw.x)/cmath.EuclDist(ls,rs) < 0.4 && abs(lw.y - rw.y) / bodystate.currShoulderWidth < 0.1)
            {
                bodystate.leftOverRightHand = true;
            }

            // ===== rightOverLeftShoulder =====
            if (abs(ls.x - rw.x) / bodystate.currShoulderWidth < 0.4 && abs(ls.y - rw.y) / bodystate.currShoulderWidth < 0.1)
            {
                bodystate.rightOverLeftShoulder = true;
            }
        }
    }
    
    if (rw.confidence > 0.5f) // has right hand
    {
        bodystate.rightWristXcoord = rw.x;
        bodystate.rightWristYcoord = rw.y;

        //=====GRAB RSHOULDER=====
        if (rs.confidence > 0.5f && re.confidence > 0.5f)
        {
            bodystate.grabRightShoulder = cmath.Angle(rs, re, rw) < AppConfigBody::GRABRIGHTSHOULDER_MAXANGLE &&
                cmath.EuclDist(rw, rs) / bodystate.currShoulderWidth < AppConfigBody::GRABRIGHTSHOULDER_MAXDIST;
        }

        // ===== F TRIGGER =====
        if (abs(rs.x - lw.x) / bodystate.currShoulderWidth < 0.4 && abs(rs.y - lw.y) / bodystate.currShoulderWidth < 0.1)
        {
            bodystate.leftOverRightShoulder = true;
        }
    }
    return bodystate;
}