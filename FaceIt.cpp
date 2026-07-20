// RMB THIS IS THE MAIN FILE. CURSE THIS DUMB FILENAME.
//

#include <opencv2/opencv.hpp>
#include <onnxruntime_cxx_api.h>
#include <iostream>
#include "CameraHandle.h"
#include "KeypointDetector.h"
#include "BodyAnalyser.h"
#include "HandAnalyser.h"
#include "PoseState.h"
#include "InputController.h"

//TODO: more gestures,gesture smoothing, wrist detection (to pass to hand detection)
int main()
{
    std::cout << "INITIALISING!\n";

    Camera camera;
    KeypointDetector keypointdetector;
    HandAnalyser handanalyser;
    BodyAnalyser bodyanalyser;
    PoseState posestate;
    InputController inputcontroller;

    if (!keypointdetector.loadModel(L"models\\yolov8n-pose.onnx"))
    {
        std::cout << "Model failed to load!";
        return -1;
    }
        
    if (!camera.isOpened())
    {
        std::cout << "Couldn't open camera\n";
        return -1;
    }

    cv::Mat frame, flippedframe;

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
            posestate.bodystate = bodyanalyser.analyseBody(pose);
            inputcontroller.update(posestate);
            cv::flip(frame, flippedframe, 1); // FLIP FRAME BEFORE PRINTING STUFF. THIS WILL B REDUNDANT ONCE REPLACED 

            if (posestate.bodystate.leftArmUp)
            {
                cv::putText(flippedframe,
                    "Left Arm Up",
                    cv::Point(30, 30),
                    cv::FONT_HERSHEY_SIMPLEX,
                    1,
                    cv::Scalar(0, 255, 255),
                    2);
            }

            if (posestate.bodystate.rightArmUp)
            {
                cv::putText(flippedframe,
                    "Right Arm Up",
                    cv::Point(30, 70),
                    cv::FONT_HERSHEY_SIMPLEX,
                    1,
                    cv::Scalar(0, 255, 255),
                    2);
            }

            if (posestate.bodystate.headLeft)
            {
                cv::putText(flippedframe,
                    "Head Left",
                    cv::Point(30, 110),
                    cv::FONT_HERSHEY_SIMPLEX,
                    1,
                    cv::Scalar(0, 255, 255),
                    2);
            }

            if (posestate.bodystate.headRight)
            {
                cv::putText(flippedframe,
                    "Head Right",
                    cv::Point(30, 150),
                    cv::FONT_HERSHEY_SIMPLEX,
                    1,
                    cv::Scalar(0, 255, 255),
                    2);
            }
        }

        cv::imshow("Camera", flippedframe);
        

        if (cv::waitKey(1) == 27)
            break;
    }
    return 0;
}
