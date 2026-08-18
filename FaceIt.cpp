// RMB THIS IS THE MAIN FILE. CURSE THIS DUMB FILENAME.
//

#include <opencv2/opencv.hpp>
#include <onnxruntime_cxx_api.h>
#include <iostream>
#include "CameraHandle.h"
#include "KeypointDetector.h"
#include "AnalyserBody.h"
#include "AnalyserHand.h"
#include "AnalyserHead.h"
#include "PoseState.h"
#include "InputController.h"
#include <chrono>

//TODO: more gestures,gesture smoothing, wrist detection (to pass to hand detection)
int main()
{
    std::cout << "INITIALISING!\n";

    Camera camera;
    KeypointDetector keypointdetector;
    PSPoseState posestate;
    AnalyserHand analyserhand;
    AnalyserBody analyserbody;
    AnalyserHead analyserhead;
    InputController inputcontroller;
    char f;
    double fps = 0.0;
    double fpsSmoothed = 0.0;

    if (!keypointdetector.loadModel(L"models\\yolov8n-pose.onnx"))
    {
        std::cout << "Model failed to load!";
        return -1;
    }
    else
        std::cout << "MODEL LOADED\n";
        
    keypointdetector.start(); //start worker trhread

    if (!camera.isOpened())
    {
        std::cout << "Couldn't open camera\n";
        return -1;
    }
    else
        std::cout << "CAMERA LOADED\n";

    cv::Mat frame, smoothedframe, flippedframe;
    double alpha = 0.4;

    //calibration loop
    while (true)
    {
        auto timeStart = std::chrono::high_resolution_clock::now(); //timer for fps reading!
        if (!camera.getFrame(frame))
        {
            std::cout << "CAN'T GET FRAME\n";
            break;
        }
        keypointdetector.pushFrame(frame); //send frame to worker
        //auto poses = keypointdetector.detect(frame);

        AllKeypoints pose;
        bool havePose = keypointdetector.getLatestPose(pose); //then get latest pose
  

        if (havePose)
        {
            //auto best = std::max_element(
            //    poses.begin(), poses.end(),
            //    [](const AllKeypoints& a, const AllKeypoints& b)
            //    {
            //        return a.score < b.score;
            //    });

            //const AllKeypoints& pose = *best;
            // ^^ commented out this part; alrdy settled in keypointdetector while adding worker thread stuff
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
        
            
        auto timeEnd = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed = timeEnd - timeStart;
        double timeFrame = elapsed.count();

        if (timeFrame > 0.0) {
            fps = 1.0 / timeFrame;

            if (fpsSmoothed == 0.0) //first frame only
            {
                fpsSmoothed = fps;
            }
            else //TODO: this is a quick LPF. might strain system though, also consider moving to commonMath.cpp
            {
                fpsSmoothed = (alpha * fps) + ((1.0 - alpha) * fpsSmoothed);
            }
        }
        
        cv::flip(frame, flippedframe, 1); // FLIP FRAME BEFORE PRINTING WORDS. THIS WILL B REDUNDANT ONCE REPLACED 
        //std::cout << fpsSmoothed << "\n";

        cv::putText(flippedframe,
            std::to_string(fpsSmoothed),
            cv::Point(10, 70),
            cv::FONT_HERSHEY_SIMPLEX,
            1,
            cv::Scalar(0, 255, 255),
            2);

        cv::imshow("Camera", flippedframe);
        if (cv::waitKey(1) == 'p')
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
        
        keypointdetector.pushFrame(frame);
        /*auto poses = keypointdetector.detect(frame);*/
        AllKeypoints pose;
        bool havePose = keypointdetector.getLatestPose(pose);

        if (havePose)
        {
            /*auto best = std::max_element(
                poses.begin(), poses.end(),
                [](const AllKeypoints& a, const AllKeypoints& b)
                {
                    return a.score < b.score;
                });

            const AllKeypoints& pose = *best;*/
            // ^^ commented out this part; alrdy settled in keypointdetector while adding worker thread stuff
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
    keypointdetector.stop(); //stop worker thread
    return 0;
}
