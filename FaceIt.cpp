// RMB THIS IS THE MAIN FILE. CURSE THIS DUMB FILENAME.
//

#include <opencv2/opencv.hpp>
#include <onnxruntime_cxx_api.h>
#include <iostream>
#include "CameraHandle.h"
#include "KeypointDetector.h"
//#include "Calibration.h"
#include "AnalyserBody.h"
#include "AnalyserHand.h"
#include "AnalyserHead.h"
#include "PoseState.h"
#include "InputController.h"

//TODO: more gestures,gesture smoothing, wrist detection (to pass to hand detection)
int main()
{
    std::cout << "INITIALISING!\n";

    Camera camera;
    KeypointDetector keypointdetector;
    PSPoseState posestate;
    //Calibration calibration;
    AnalyserHand analyserhand;
    AnalyserBody analyserbody;
    AnalyserHead analyserhead;
    InputController inputcontroller;
    char f;

    if (!keypointdetector.loadModel(L"models\\yolov8n-pose.onnx"))
    {
        std::cout << "Model failed to load!";
        return -1;
    }
    else
        std::cout << "MODEL LOADED\n";
        
    if (!camera.isOpened())
    {
        std::cout << "Couldn't open camera\n";
        return -1;
    }
    else
        std::cout << "CAMERA LOADED\n";

    cv::Mat frame, flippedframe;

    //calibration loop
    while (true)
    {
        if (!camera.getFrame(frame))
        {
            std::cout << "CAN'T GET FRAME\n";
            break;
        }
        auto poses = keypointdetector.detect(frame);

        if (!poses.empty())
        {
            auto best = std::max_element(
                poses.begin(), poses.end(),
                [](const AllKeypoints& a, const AllKeypoints& b)
                {
                    return a.score < b.score;
                });

            const AllKeypoints& pose = *best;

            for (const auto& kp : pose.keypoints)
            {
                if (kp.confidence > 0.5f)
                    cv::circle(frame, cv::Point((int)kp.x, (int)kp.y), 4, cv::Scalar(0, 255, 0), -1);
            }
            posestate.ps_bodystate = analyserbody.analyseBody(pose);
            posestate.ps_headstate = analyserhead.analyseHead(pose);

            f = cv::waitKey(1);
            if (f != -1)
            {
                std::cout << f << "\n";
                posestate.ps_calibrateheadstate = analyserhead.calibrateHead(pose, f);
            }
         
        }
        cv::flip(frame, flippedframe, 1); // FLIP FRAME BEFORE PRINTING WORDS. THIS WILL B REDUNDANT ONCE REPLACED 

        cv::imshow("Camera", flippedframe);
        if (cv::waitKey(1) == 27)
        {
            std::cout << "CALIBRATION DONE, PLEASE WAIT, RUNNING LOOP STARTING UP\n";
            break;
        }
            
        
    }
    


    std::cout << "RUNNING LOOP STARTED\n";

    //actual running loop
    //mainprogramloop:
    while (true)
    {
        if (!camera.getFrame(frame))
            break;
        
        auto poses = keypointdetector.detect(frame);

        if (!poses.empty())
        {
            auto best = std::max_element(
                poses.begin(), poses.end(),
                [](const AllKeypoints& a, const AllKeypoints& b)
                {
                    return a.score < b.score;
                });

            const AllKeypoints& pose = *best;

            for (const auto& kp : pose.keypoints)
            {
                if (kp.confidence > 0.5f)
                    cv::circle(frame, cv::Point((int)kp.x, (int)kp.y), 4, cv::Scalar(0, 255, 0), -1);
            }


            // BODY GESTURE SECTION
            posestate.ps_bodystate = analyserbody.analyseBody(pose);
            posestate.ps_headstate = analyserhead.analyseHead(pose);
            
            inputcontroller.update(posestate);
            if (posestate.ps_headstate.headXcoord) // mouse control
            {
                /*std::cout << "xcoord: " << posestate.ps_headstate.headXcoord << "\n"
                          << "x: " << (int)(posestate.ps_headstate.headXcoord * frame.size().width) << "\n"
                          << "y: " << (int)posestate.ps_headstate.headYcoord << "\n";*/
                
                cv::circle(frame, cv::Point((int)(posestate.ps_headstate.headXcoord * frame.size().width), (int)(posestate.ps_headstate.headYcoord * frame.size().height)) , 6, cv::Scalar(255, 0, 0), -1);
            }

            

            if (posestate.ps_bodystate.leftArmUp)
            {
                cv::putText(frame,
                    "Left Arm Up",
                    cv::Point(30, 30),
                    cv::FONT_HERSHEY_SIMPLEX,
                    1,
                    cv::Scalar(0, 255, 255),
                    2);
            }

            if (posestate.ps_bodystate.rightArmUp)
            {
                cv::putText(frame,
                    "Right Arm Up",
                    cv::Point(30, 70),
                    cv::FONT_HERSHEY_SIMPLEX,
                    1,
                    cv::Scalar(0, 255, 255),
                    2);
            }

            if (posestate.ps_bodystate.headLeft)
            {
                cv::putText(frame,
                    "Head Left",
                    cv::Point(30, 110),
                    cv::FONT_HERSHEY_SIMPLEX,
                    1,
                    cv::Scalar(0, 255, 255),
                    2);
            }

            if (posestate.ps_bodystate.headRight)
            {
                cv::putText(frame,
                    "Head Right",
                    cv::Point(30, 150),
                    cv::FONT_HERSHEY_SIMPLEX,
                    1,
                    cv::Scalar(0, 255, 255),
                    2);
            }
        }
        cv::flip(frame, flippedframe, 1); 
        cv::imshow("Camera", flippedframe);
        

        if (cv::waitKey(1) == 27)
            break;
    }
    return 0;
}
