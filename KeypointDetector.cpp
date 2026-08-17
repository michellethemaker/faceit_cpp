#include "KeypointDetector.h"
#include <iostream>
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
	sessionOptions.SetGraphOptimizationLevel(
		GraphOptimizationLevel::ORT_ENABLE_ALL);
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

//convert opencv's Mat to vector type for onnx model to read
std::vector<float> KeypointDetector::preprocess(const cv::Mat& frame)
{
    LetterboxInfo info{};
    cv::Mat boxed = letterbox(frame, inputSize, info);

    cv::Mat rgb, floatImg;
    cv::cvtColor(boxed, rgb, cv::COLOR_BGR2RGB);
    rgb.convertTo(floatImg, CV_32F, 1.0f / 255.0f);

    std::vector<float> inputTensorValues(3 * inputSize.width * inputSize.height);

    std::vector<cv::Mat> channels(3);
    for (int i = 0; i < 3; ++i)
    {
        channels[i] = cv::Mat(inputSize.height, inputSize.width, CV_32F,
            inputTensorValues.data() + i * inputSize.width * inputSize.height);
    }

    cv::split(floatImg, channels);
    return inputTensorValues;
}

//convert model output to actual keypoints (in original image coords!)
std::vector<AllKeypoints> KeypointDetector::postprocess(const std::vector<float>& output, const cv::Size& originalSize)
{
    std::vector<AllKeypoints> keypoints;

    const int numKeypoints = 17;
    const int numAttributes = 56; // 4 box + 1 score + 51 kps
    const int numCandidates = 8400;
    const float confThresh = 0.65f; //only keep valid pts

    if (output.size() < static_cast<size_t>(numAttributes * numCandidates)) //if o/p too smol.
        return keypoints;

    auto at = [&](int attr, int idx) -> float
    {
        return output[attr * numCandidates + idx];
    };
    
    float gain = std::min((float)inputSize.width / originalSize.width, (float)inputSize.height / originalSize.height);
    float padX = (inputSize.width - originalSize.width * gain) * 0.5f;
    float padY = (inputSize.height - originalSize.height * gain) * 0.5f;


    for (int i = 0; i < numCandidates; ++i)
    {
        float score = at(4, i);
        if (score < confThresh)
            continue;

        AllKeypoints keypoint;
        keypoint.keypoints.reserve(numKeypoints);

        for (int k = 0; k < numKeypoints; ++k)
        {
            int attrBase = 5 + k * 3;

            float x = at(attrBase + 0, i);
            float y = at(attrBase + 1, i);
            float kpConf = at(attrBase + 2, i);

            x = (x - padX) / gain;
            y = (y - padY) / gain;

            if (x < 0.0f) x = 0.0f;
            if (y < 0.0f) y = 0.0f;
            if (x > originalSize.width - 1) x = (float)(originalSize.width - 1);
            if (y > originalSize.height - 1) y = (float)(originalSize.height - 1);

            Keypoint kp;
            kp.x = x;
            kp.y = y;
            kp.confidence = kpConf;

            keypoint.keypoints.push_back(kp);
        }

        keypoints.push_back(std::move(keypoint));
    }
        return keypoints;
}

std::vector<AllKeypoints> KeypointDetector::detect(const cv::Mat& frame)
{
    if (!session) //make sure model loaded!
        return { };

    std::vector<float> inputData = preprocess(frame); //conv frame->tensor
    std::array<int64_t, 4> inputShape = { 1,3,inputSize.height, inputSize.width };

    Ort::MemoryInfo memoryInfo = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
    //allocate from CPU arena, and use default CPU memory type.

    Ort::Value inputTensor = Ort::Value::CreateTensor<float>(memoryInfo, 
        inputData.data(), 
        inputData.size(), 
        inputShape.data(), 
        inputShape.size());

    Ort::AllocatorWithDefaultOptions allocator; //to get ip/op names safely

    auto inputName = session->GetInputNameAllocated(0, allocator); //first ip name
    auto outputName = session->GetOutputNameAllocated(0, allocator); //first op name

    const char* inputNames[] = { inputName.get() }; //convrt to array of c strings (onnxrt reqmt)
    const char* outputNames[] = { outputName.get() };

    auto outputTensors = session->Run(Ort::RunOptions{ nullptr },
        inputNames,
        &inputTensor,
        1,
        outputNames,
        1);

    const Ort::Value& outTensor = outputTensors[0]; //first op tensor

    auto outInfo = outTensor.GetTensorTypeAndShapeInfo();

#ifdef DEBUG
    auto outShape = outInfo.GetShape(); // Get actual shape.

    std::cout << "Output shape = [";
    for (size_t i = 0; i < outShape.size(); ++i)
    {
        if (i) std::cout << ", ";
        std::cout << outShape[i];
    }
    std::cout << "]\n";
    
#endif
    size_t outCount = outInfo.GetElementCount(); //total float vals in op

    const float* outData = outTensor.GetTensorData<float>(); //raw ptr to tensor contents

#ifdef DEBUG
    size_t n = std::min<size_t>(20, outInfo.GetElementCount()); // Only print a few.

    std::cout << "First output values: ";
    for (size_t i = 0; i < n; ++i)
    {
        std::cout << outData[i] << " ";
    }
    std::cout << "\n";
#endif
    std::vector<float> output(outData, outData + outCount); //copy op to normal vector
    return postprocess(output, frame.size()); //convt raw op to poses

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

        auto poses = detect(workFrame); // BOOM just run this in this here worker thread
       
        AllKeypoints bestPose;
        bool hasPose = false;

        if (!poses.empty())
        {
            auto best = std::max_element(
                poses.begin(), poses.end(),
                [](const AllKeypoints& a, const AllKeypoints& b)
                {
                    return a.score < b.score;
                });
            bestPose = *best;
            hasPose = true;
        }
        
        {
            std::lock_guard<std::mutex> lock(poseMutex);//publish result for main thread
            latestPose = bestPose;
            hasLatestPose = hasPose;
        }
    }
}