// RMB THIS IS THE MAIN FILE. CURSE THIS DUMB FILENAME.
//

#include <opencv2/opencv.hpp>
#include <onnxruntime_cxx_api.h>
#include <iostream>
#include "CameraHandle.h"
#include "KeypointDetector.h"
#include "KeypointHandDetector.h"
#include "AnalyserBody.h"
#include "AnalyserHand.h"
#include "AnalyserHead.h"
#include "PoseState.h"
#include "InputController.h"
#include <chrono>

//TODO: more gestures,gesture smoothing
int main()
{
    std::cout << "INITIALISING!\n";

    Camera camera;
    KeypointDetector keypointdetector;
    KeypointHandDetector lefthanddetector;
    //KeypointHandDetector righthanddetector;
    PSPoseState posestate;
    AnalyserHand analyserhand;
    AnalyserBody analyserbody;
    AnalyserHead analyserhead;
    InputController inputcontroller;
    char f;
    double fps = 0.0;
    double fpsSmoothed = 0.0;

    // Set level to WARNING n suppress INFO logs
    cv::utils::logging::setLogLevel(cv::utils::logging::LOG_LEVEL_WARNING);

    //if (!keypointdetector.loadPersonModel(L"models\\person_detection_mediapipe_2023mar.onnx")) //int8 model causes quantisation error!!
    //{
    //    std::cout << "Person Detection Model failed to load!";
    //    return -1;
    //}
    //else
    //    std::cout << "PERSON DETECTION MODEL LOADED\n";

    if (!keypointdetector.loadModel(L"models\\pose_estimation_mediapipe_2023mar.onnx"))
    {
        std::cout << "Body model failed to load!";
        return -1;
    }
    else
        std::cout << "BODY MODEL LOADED\n";
        
    //if (!lefthanddetector.loadModel(L"models\\yolo26_hand_pose_int8_2.onnx"))
    //{
    //    std::cout << "LHandModel failed to load!";
    //    return -1;
    //}
    //else
    //    std::cout << "LEFT HAND MODEL LOADED\n";
    //if (!righthanddetector.loadModel(L"models\\yolo26_hand_pose_int8.onnx"))
    //{
    //    std::cout << "RHandModel failed to load!";
    //    return -1;
    //}
    //else
    //    std::cout << "RIGHT HAND MODEL LOADED\n";
    keypointdetector.printModelInfo();
    keypointdetector.start(); //start worker trhread
    //lefthanddetector.start();
    //righthanddetector.start();

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
        //PersonROI roi;
        //if (keypointdetector.detectPerson(frame, roi))
        //{
        //    cv::rectangle( frame, cv::Rect(
        //            static_cast<int>(roi.center.x - roi.size.width * 0.5f),
        //            static_cast<int>(roi.center.y - roi.size.height * 0.5f),
        //            static_cast<int>(roi.size.width),
        //            static_cast<int>(roi.size.height)
        //        ),
        //        cv::Scalar(255, 0, 0),
        //        2
        //    );
       
        //}
        //keypointdetector.detectPerson(frame, roi);
        keypointdetector.pushFrame(frame); //send frame to worker
        //keypointhanddetector.pushFrame(frame); //send frame to worker
        //auto poses = keypointdetector.detect(frame);

        AllKeypoints pose;

        bool havePose = keypointdetector.getLatestPose(pose); //then get latest pose

        if (havePose)
        {
            int idx = 0;
            for (const auto& kp : pose.keypoints)
            {
                if (kp.confidence > 0.5f)
                {
                    cv::circle(frame, cv::Point((int)kp.x, (int)kp.y), 4, cv::Scalar(0, 255, 0), -1);
                    cv::putText(frame, std::to_string(idx), cv::Point((int)kp.x, (int)kp.y),  // slight offset
                                    cv::FONT_HERSHEY_SIMPLEX, 1, cv::Scalar(0, 12, 12), 2 );
                }
                ++idx;
            }
            posestate.ps_bodystate = analyserbody.analyseBody(pose);
            posestate.ps_headstate = analyserhead.analyseHead(pose);


            if (posestate.ps_bodystate.hasLeftHand)
            {
                cv::Rect leftROI = posestate.ps_bodystate.leftHandROI;
                
                //clamp
                leftROI &= cv::Rect(0, 0, frame.cols, frame.rows); 
                cv::Mat leftCrop;
                if (leftROI.width > 10 && leftROI.height > 10)
                    leftCrop = frame(leftROI).clone();
                

                if (!leftCrop.empty())
                    lefthanddetector.pushFrame(leftCrop);
                AllHandKeypoints posehand;
                
                if (lefthanddetector.getLatestPose(posehand))
                {
                    posestate.ps_handstate = analyserhand.analyseHand(posehand);
                    //int idx = 0;
                    for (const auto& kp : posehand.keypointshand)
                    {
                        if (kp.confidence > 0.5f)
                        {
                            cv::Point pt(
                                static_cast<int>(kp.x + leftROI.x),
                                static_cast<int>(kp.y + leftROI.y)
                            );
                            cv::circle(frame, pt, 4, cv::Scalar(115, 15, 0), -1);
                            //to check keypoints
                            //cv::putText(
                            //    frame,
                            //    std::to_string(idx),
                            //    pt + cv::Point(5, -5),  // slight offset
                            //    cv::FONT_HERSHEY_SIMPLEX,
                            //    0.4,
                            //    cv::Scalar(0, 255, 255),
                            //    1
                            //);
                        }
                        //idx++;
                    }
                }

            }

            //if (posestate.ps_bodystate.hasRightHand)
            //{
            //    cv::Rect rightROI = posestate.ps_bodystate.rightHandROI;

            //    //clamp
            //    rightROI &= cv::Rect(0, 0, frame.cols, frame.rows);
            //    cv::Mat rightCrop;
            //    if (rightROI.width > 10 && rightROI.height > 10)
            //        rightCrop = frame(rightROI).clone();


            //    if (!rightCrop.empty())
            //        righthanddetector.pushFrame(rightCrop);
            //    AllHandKeypoints posehand;
            //    if (righthanddetector.getLatestPose(posehand))
            //    {
            //        for (const auto& kp : posehand.keypointshand)
            //        {
            //            if (kp.confidence > 0.5f)
            //            {
            //                cv::Point pt(
            //                    static_cast<int>(kp.x + rightROI.x),
            //                    static_cast<int>(kp.y + rightROI.y)
            //                );
            //                //cv::circle(frame, pt, 4, cv::Scalar(55, 55, 0), -1);
            //            }
            //        }
            //    }
            //}
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

        // HANDS SECTION
        //if (posestate.ps_bodystate.leftWristXcoord)
        //{
        //    float lwx = posestate.ps_bodystate.leftWristXcoord;
        //    float lwy = posestate.ps_bodystate.leftWristYcoord;
        //    int radius = 200;
        //    int x0 = static_cast<int>(lwx) - radius;
        //    int y0 = static_cast<int>(lwy) - radius;
        //    int x1 = static_cast<int>(lwx) + radius;
        //    int y1 = static_cast<int>(lwy) + radius;
        //    int w = 2 * radius;
        //    int h = 2 * radius;

        //    // Clamp to image bounds
        //    x0 = std::clamp(x0, 0, frame.cols - 1);
        //    y0 = std::clamp(y0, 0, frame.rows - 1);
        //    x1 = std::clamp(x1, 0, frame.cols - 1);
        //    y1 = std::clamp(y1, 0, frame.rows - 1);
        //    w = std::clamp(w, 1, frame.cols - x0);
        //    h = std::clamp(h, 1, frame.rows - y0);

        //    cv::rectangle(frame, cv::Point(x0, y0), cv::Point(x1, y1), cv::Scalar(0, 255, 255), 1);
        //    cv::Rect roi(x0, y0, w, h);
        //}
        cv::flip(frame, flippedframe, 1); 
        cv::imshow("Camera", flippedframe);
        

        if (cv::waitKey(1) == 27)
            break;
    }
    keypointdetector.stop(); //stop worker thread
    //lefthanddetector.stop();
    //righthanddetector.stop();
    return 0;
}
