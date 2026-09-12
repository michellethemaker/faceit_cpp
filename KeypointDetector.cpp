#include "KeypointDetector.h"
#include <iostream>
#include <cstring>
#define NDEBUG

// resize, pad inpt img so it matches model's expected ip size (KEEPS ASPECT RATIO SAME)
static cv::Mat letterbox(const cv::Mat& src, cv::Size newShape, LetterboxInfo& info) 
{
    float r = std::min((float)newShape.width / src.cols, (float)newShape.height / src.rows);

    int newUnpadW = (int)std::round(src.cols * r);
    int newUnpadH = (int)std::round(src.rows * r);

    int dw = newShape.width - newUnpadW;
    int dh = newShape.height - newUnpadH;

    dw /= 2;
    dh /= 2;

    info.gain = r;
    info.padX = (float)dw;
    info.padY = (float)dh;

    cv::Mat resized, out;
    cv::resize(src, resized, cv::Size(newUnpadW, newUnpadH));
    cv::copyMakeBorder(resized, out, dh, newShape.height - newUnpadH - dh, dw, newShape.width - newUnpadW - dw,
        cv::BORDER_CONSTANT, cv::Scalar(114, 114, 114));

    return out;
}

void KeypointDetector::start() //start worker thread
{
    if (running) return;
    running = true;
    workerThread = std::thread(&KeypointDetector::workerLoop, this);
    std::cout << "starting body worker thread\n";
}

void KeypointDetector::stop() //stop worker thread
{
    if (!running) return;
    running = false;

    // this is to wake up worker if it's sleeping/waiting for a frame
    {
        std::lock_guard<std::mutex> lock(frameMutex);
        hasPendingFrame = true;
    }

    if (workerThread.joinable())
    {
        workerThread.join();
    }
}

KeypointDetector::KeypointDetector() //create onnx runtime env, set optimisation settings here
	: env(ORT_LOGGING_LEVEL_WARNING, "KeypointDetector") 
{
	sessionOptions.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);
}
KeypointDetector::~KeypointDetector() // destructor
{
    stop(); // make sure worker thread joined! (stop() defined above)
}

void KeypointDetector::pushFrame(const cv::Mat& frame)// main thread writes to pendingFrame
{
    {
        std::lock_guard<std::mutex> lock(frameMutex);
        frame.copyTo(pendingFrame);
        hasPendingFrame = true;
    }
}

bool KeypointDetector::getLatestPose(AllKeypoints& out)// main thread reads latestPose
{
    std::lock_guard<std::mutex> lock(poseMutex);
    if (!hasLatestPose)
        return false;
    out = latestPose;
    return true;
}

// helper function, replaces person detector
cv::Rect KeypointDetector::fixedPersonCrop(const cv::Mat& frame)
{
    int W = frame.cols;
    int H = frame.rows;

    // NOTE: this is total agaration
    float cropW = W * 0.75f;  // 75% width
    float cropH = H * 1.0f;  // 100% height (person prolly hits max height in webcam)

    int x = static_cast<int>((W - cropW) * 0.5f); // centered horizontally
    int y = static_cast<int>((H - cropH) * 0.15f); // slightly top-biased? idk anymore

    return cv::Rect(x, y, static_cast<int>(cropW), static_cast<int>(cropH));
}

void KeypointDetector::workerLoop()
{
    while (running)
    {
        cv::Mat workFrame;
        bool haveFrame = false;

        // grab pending frame (ifany)
        std::lock_guard<std::mutex> lock(frameMutex);
        if (hasPendingFrame)
        {
            workFrame = pendingFrame.clone();
            hasPendingFrame = false;
            haveFrame = true;
        }

        if (!haveFrame)//smol sleep, avoid busyspinninh
        {
            //std::this_thread::sleep_for(std::chrono::milliseconds(1)); //tried the sleep, too jittery. keep in case.
            continue;
        }

        // try fixed roi crop instead of person detector model
        cv::Rect personRoi = fixedPersonCrop(workFrame);
        cv::Mat personCrop = workFrame(personRoi).clone();

        auto poses = detect(personCrop); // BOOM just run this in this here worker thread
        AllKeypoints bestPose;
        bool hasPose = false;

        if (!poses.empty())
        {
            auto best = std::max_element(poses.begin(), poses.end(),
                [](const AllKeypoints& a, const AllKeypoints& b)
                {
                    return a.score < b.score;
                });
            bestPose = *best;
            // need to shift keypoints from cropped ROI back to full-frame coordinates
            for (auto& kp : bestPose.keypoints) {
                kp.x += static_cast<float>(personRoi.x);
                kp.y += static_cast<float>(personRoi.y);
            }
            hasPose = true;
        }

        {
            std::lock_guard<std::mutex> lock(poseMutex);//publish result for main thread
            latestPose = bestPose;
            hasLatestPose = hasPose;
        }
    }
}

bool KeypointDetector::loadPersonModel(const std::wstring& modelPath)
{
    try
    {
        personSession = std::make_unique<Ort::Session>(env, modelPath.c_str(), sessionOptions);
        std::cout << "Person detector loaded\n";
        return true;
    }
    catch (const Ort::Exception& e)
    {
        std::cerr << "Person detector load failed: " << e.what() << "\n";
        return false;
    }
}

bool KeypointDetector::loadModel(const std::wstring& modelPath)
{
    try
    {
        sessionOptions.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);
        session = std::make_unique<Ort::Session>(env, modelPath.c_str(), sessionOptions);
        return true;
    }
    catch (const Ort::Exception& e)
    {
        std::cout << "ONNX load failed: " << e.what() << "\n";
        return false;
    }

}


std::vector<float> KeypointDetector::preprocess(const cv::Mat& frame, ModelPreprocessInfo& info)
{
    cv::Size targetSize(288, 384);
    LetterboxInfo lbInfo{};
    cv::Mat lb = letterbox(frame, targetSize, lbInfo); // existing letterbox

    // bgr to rgb, float32
    cv::Mat norm;
    lb.convertTo(norm, CV_32FC3, 1.0f / 255.0f);

    // HWC -> CHW
    std::vector<float> inputData(3 * targetSize.height * targetSize.width);
    std::vector<cv::Mat> channels(3);
    for (int c = 0; c < 3; ++c) {
        channels[c] = cv::Mat(
            targetSize.height,
            targetSize.width,
            CV_32F,
            inputData.data() + c * targetSize.height * targetSize.width
        );
    }
    cv::split(norm, channels);

    // Store letterbox info in your existing struct (reuse fields)
    info.scale = lbInfo.gain;
    info.padX = lbInfo.padX;
    info.padY = lbInfo.padY;

    return inputData;
}

std::vector<AllKeypoints> KeypointDetector::detect(const cv::Mat& frame)
{
    if (!session)
        return {};

    ModelPreprocessInfo preprocessInfo;
    std::vector<float> inputData = preprocess(frame, preprocessInfo);

    std::array<int64_t, 4> inputShape = { 1, 3, 384, 288 };

    Ort::MemoryInfo memoryInfo = Ort::MemoryInfo::CreateCpu( OrtArenaAllocator, 
        OrtMemTypeDefault );

    Ort::Value inputTensor = Ort::Value::CreateTensor<float>(
                                    memoryInfo,
                                    inputData.data(),
                                    inputData.size(),
                                    inputShape.data(),
                                    inputShape.size()
                                    );

    Ort::AllocatorWithDefaultOptions allocator;

    // input
    auto inputName = session->GetInputNameAllocated( 0, allocator );
    const char* inputNames[] = { inputName.get() };

    // Get all model outputs
    const size_t numOutputs = session->GetOutputCount();
    std::vector< Ort::AllocatedStringPtr > outputNameStorage;
    std::vector<const char*> outputNames;
    outputNameStorage.reserve(numOutputs);
    outputNames.reserve(numOutputs);

    for (size_t i = 0; i < numOutputs; ++i)
    {
        outputNameStorage.push_back(
            session->GetOutputNameAllocated(i, allocator));

        outputNames.push_back( outputNameStorage.back().get());
    }

    auto outputTensors = session->Run( Ort::RunOptions{ nullptr }, inputNames, &inputTensor, 
                        1, outputNames.data(), outputNames.size() );

    if (outputTensors.size() != 2) {
        std::cerr << "Expected 2 outputs (simcc_x, simcc_y), got "
            << outputTensors.size() << "\n";
        return {};
    }

    //get simcc_xand simcc_y
    const Ort::Value& simccXTensor = outputTensors[0];
    const Ort::Value& simccYTensor = outputTensors[1];

    const float* simccX = simccXTensor.GetTensorData<float>();
    const float* simccY = simccYTensor.GetTensorData<float>();

    // DWPose dw-ll_ucoco_384: [1, 133, 576] and [1, 133, 768]
    const int K = 133;
    const int W_simcc = 576; // 288 * 2
    const int H_simcc = 768; // 384 * 2
    const float splitRatio = 2.0f;
    
    //decode 133 kps
    AllKeypoints pose;
    pose.keypoints.reserve(K);
    pose.score = 1.0f;

    for (int k = 0; k < K; ++k) {
        const float* rowX = simccX + k * W_simcc;
        const float* rowY = simccY + k * H_simcc;

        // argmax x
        int x_idx = 0;
        float x_max = rowX[0];
        for (int i = 1; i < W_simcc; ++i) {
            if (rowX[i] > x_max) {
                x_max = rowX[i];
                x_idx = i;
            }
        }

        // argmax y
        int y_idx = 0;
        float y_max = rowY[0];
        for (int i = 1; i < H_simcc; ++i) {
            if (rowY[i] > y_max) {
                y_max = rowY[i];
                y_idx = i;
            }
        }

        float x_model = static_cast<float>(x_idx) / splitRatio; // 0..288
        float y_model = static_cast<float>(y_idx) / splitRatio; // 0..384

        // Undo letterbox
        float x_orig = (x_model - preprocessInfo.padX) / preprocessInfo.scale;
        float y_orig = (y_model - preprocessInfo.padY) / preprocessInfo.scale;

        x_orig = std::clamp(x_orig, 0.0f, static_cast<float>(frame.cols - 1));
        y_orig = std::clamp(y_orig, 0.0f, static_cast<float>(frame.rows - 1));

        Keypoint kp;
        kp.x = x_orig;
        kp.y = y_orig;
        kp.confidence = std::min(x_max, y_max);

        pose.keypoints.push_back(kp);
    }

    return { pose };
}

void KeypointDetector::printModelInfo()
{
    if (!session)
        return;

    Ort::AllocatorWithDefaultOptions allocator;

    size_t numInputs = session->GetInputCount();

    std::cout << "\n=== INPUTS ===\n";

    for (size_t i = 0; i < numInputs; ++i)
    {
        auto name = session->GetInputNameAllocated(i, allocator);

        auto typeInfo = session->GetInputTypeInfo(i);
        auto tensorInfo = typeInfo.GetTensorTypeAndShapeInfo();

        auto shape = tensorInfo.GetShape();

        std::cout << "Input " << i
            << ": " << name.get() << "\n";

        std::cout << "  shape: [";

        for (size_t j = 0; j < shape.size(); ++j)
        {
            if (j) std::cout << ", ";
            std::cout << shape[j];
        }

        std::cout << "]\n";

        std::cout << "  type: "
            << tensorInfo.GetElementType()
            << "\n";
    }

    size_t numOutputs = session->GetOutputCount();

    std::cout << "\n=== OUTPUTS ===\n";

    for (size_t i = 0; i < numOutputs; ++i)
    {
        auto name = session->GetOutputNameAllocated(i, allocator);

        auto typeInfo = session->GetOutputTypeInfo(i);

        auto tensorInfo = typeInfo.GetTensorTypeAndShapeInfo();

        auto shape = tensorInfo.GetShape();

        std::cout << "Output " << i
            << ": " << name.get() << "\n";
        std::cout << "  shape: [";

        for (size_t j = 0; j < shape.size(); ++j)
        {
            if (j) std::cout << ", ";
            std::cout << shape[j];
        }

        std::cout << "]\n";
        std::cout << "  type: " << tensorInfo.GetElementType() << "\n";
    }
    for (size_t i = 0; i < session->GetOutputCount(); ++i)
    {
        auto typeInfo = session->GetOutputTypeInfo(i);
        auto tensorInfo = typeInfo.GetTensorTypeAndShapeInfo();
        auto shape = tensorInfo.GetShape();
        std::cout << "Output " << i << " shape: [";
        for (size_t j = 0; j < shape.size(); ++j) {
            if (j) std::cout << ", ";
            std::cout << shape[j];
        }
        std::cout << "]\n";
    }
}

void KeypointDetector::printPoseModelInfo()
{
    if (!session) {
        std::cout << "Pose session is null\n";
        return;
    }

    Ort::AllocatorWithDefaultOptions allocator;

    size_t numInputs = session->GetInputCount();
    std::cout << "\n=== POSE MODEL INPUTS ===\n";
    for (size_t i = 0; i < numInputs; ++i) {
        auto name = session->GetInputNameAllocated(i, allocator);
        auto typeInfo = session->GetInputTypeInfo(i);
        auto tensorInfo = typeInfo.GetTensorTypeAndShapeInfo();
        auto shape = tensorInfo.GetShape();

        std::cout << "Input " << i << ": " << name.get() << "\n";
        std::cout << "  shape: [";
        for (size_t j = 0; j < shape.size(); ++j) {
            if (j) std::cout << ", ";
            std::cout << shape[j];
        }
        std::cout << "]\n";
        std::cout << "  type: " << tensorInfo.GetElementType() << "\n";
    }

    size_t numOutputs = session->GetOutputCount();
    std::cout << "\n=== POSE MODEL OUTPUTS ===\n";
    for (size_t i = 0; i < numOutputs; ++i) {
        auto name = session->GetOutputNameAllocated(i, allocator);
        auto typeInfo = session->GetOutputTypeInfo(i);
        auto tensorInfo = typeInfo.GetTensorTypeAndShapeInfo();
        auto shape = tensorInfo.GetShape();

        std::cout << "Output " << i << ": " << name.get() << "\n";
        std::cout << "  shape: [";
        for (size_t j = 0; j < shape.size(); ++j) {
            if (j) std::cout << ", ";
            std::cout << shape[j];
        }
        std::cout << "]\n";
        std::cout << "  type: " << tensorInfo.GetElementType() << "\n";
    }
}