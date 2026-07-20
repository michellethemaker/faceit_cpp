// RMB THIS IS THE MAIN FILE. CURSE THIS DUMB FILENAME.
//

#include <opencv2/opencv.hpp>
#include <onnxruntime_cxx_api.h>
#include <iostream>
#include "CameraHandle.h"
#include "KeypointDetector.h"
#include "GestureDetector.h"
#include "HandDetector.h"

//TODO: more gestures,gesture smoothing, wrist detection (to pass to hand detection)
int main()
{
    std::cout << "INITIALISING!\n";

    Camera camera;
    KeypointDetector keypointdetector;
    HandDetector handdetector;
    GestureDetector gesturedetector;
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

    cv::Mat frame;

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
            std::string gesture = gesturedetector.detectGesture(pose);
            cv::putText(frame, gesture, cv::Point(30, 30),
                cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(0, 255, 255), 2);
        }


        cv::imshow("Camera", frame);
        

        if (cv::waitKey(1) == 27)
            break;
    }
    return 0;
}
