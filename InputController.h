#pragma once
#include <onnxruntime_cxx_api.h>
#include "Keypoint.h"
#include "PoseState.h"
//threading headers
#include <thread>
#include <mutex>
#include <atomic>
class InputController
{
public:
    InputController();
    ~InputController();
    
    void start();
    void stop();
    void pushState(const PSPoseState& state); // call from main thread per frame

    void update(const PSPoseState & state);

private:
    void workerLoop(); //worker thread
    cv::Size personInputSize{ 224,224 }; // for person detection in mediapipe
    cv::Size inputSize{ 256,256 }; //640, 640 for yolo26

    Ort::Env inputControllerenv;
    Ort::SessionOptions sessionOptions; //session settings (e.g. optimisation level)
    std::unique_ptr<Ort::Session> inputControllerSession;

    // the threading members
    std::thread workerThread;
    std::atomic<bool> running{ false };

    PSPoseState pendingState;
    bool hasPendingState = false;
    std::mutex stateMutex;
};