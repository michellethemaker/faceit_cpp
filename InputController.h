#pragma once
#include <onnxruntime_cxx_api.h>
#include <Windows.h> // for WORD
#include "Keypoint.h"
#include "PoseState.h"
//threading headers
#include <thread>
#include <mutex>
#include <atomic>
#include <chrono> //for keydown/keyup
#include <array>//for keydown/keyup
class InputController
{
public:
    InputController();
    ~InputController();
    
    void start();
    void stop();
    void pushState(const PSPoseState& state); // call from main thread per frame

    void setHeld(WORD keyCode, bool shouldHold);
    void tapKey(WORD keyCode, int holdMs);
    void updateInput(); //for keydown/keyup
    void releaseAllKeys(); //for keydown/keyup

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

    enum class InputCode : size_t // to add to key_count
    {
        MouseLeft = 256,
        MouseRight = 257,
        MouseMiddle = 258,

        Count = 259
    };

    void setHeld(InputCode inputCode, bool shouldHold);//generic private vsn
    void tapKey(InputCode inputCode, int holdMs = 25);
    void tapInputInternal(size_t inputCode, int holdMs);
    void sendDown(size_t inputCode);
    void sendUp(size_t inputCode);
    void setHeldInternal(size_t inputCode, bool shouldHold);
    static constexpr size_t KEY_COUNT = 256; //for keydown/keyup
    static constexpr size_t INPUT_COUNT = static_cast<size_t>(InputCode::Count); // 259

    // all heldkey requests
    std::array<bool, INPUT_COUNT> heldWanted{}; //for keydown/keyup

    // Actual state already sent to Windows.
    std::array<bool, INPUT_COUNT> sentDown{}; //for keydown/keyup

    // Time at which a temporary tap expires.
    // A default-constructed time_point means "no active tap".
    std::array<std::chrono::steady_clock::time_point, INPUT_COUNT> tapUntil{}; //for keydown/keyup
};