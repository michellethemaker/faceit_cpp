#include "KeypointHandDetector.h"
#include <iostream>

// resize, pad inpt img so it matches model's expected ip size (KEEPS ASPECT RATIO SAME)
static cv::Mat letterbox(const cv::Mat& src, cv::Size newShape, LetterboxInfoHand& info)
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

void KeypointHandDetector::start() //start worker thread
{
    if (running) return;
    running = true;
    workerThread = std::thread(&KeypointHandDetector::workerLoop, this);
    std::cout << "starting hand worker thread\n";
}

void KeypointHandDetector::stop() //stop worker thread
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

KeypointHandDetector::KeypointHandDetector() //create onnx runtime env, set optimisation settings here
    : env(ORT_LOGGING_LEVEL_WARNING, "KeypointDetector")
{
    sessionOptions.SetGraphOptimizationLevel(
        GraphOptimizationLevel::ORT_ENABLE_ALL);
}
KeypointHandDetector::~KeypointHandDetector() // destructor
{
    stop(); // make sure worker thread joined! (stop() defined above)
}

void KeypointHandDetector::pushFrame(const cv::Mat& frame)// main thread writes to pendingFrame
{
    {
        std::lock_guard<std::mutex> lock(frameMutex);
        frame.copyTo(pendingFrame);
        hasPendingFrame = true;
    }
}

bool KeypointHandDetector::getLatestPose(AllHandKeypoints& out)// main thread reads latestPose
{
    std::lock_guard<std::mutex> lock(poseMutex);
    if (!hasLatestPose)
        return false;
    out = latestPose;
    return true;
}

void KeypointHandDetector::workerLoop()
{
    while (running)
    {
        cv::Mat workFrame;
        bool haveFrame = false;

        // grab pending frame (if any)
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
        AllHandKeypoints bestPose;
        bool hasPose = false;

        if (!poses.empty())
        {
            auto best = std::max_element(
                poses.begin(), poses.end(),
                [](const AllHandKeypoints& a, const AllHandKeypoints& b)
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

bool KeypointHandDetector::loadModel(const std::wstring& modelPath)
{
    try
    {
        sessionOptions.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);
        session = std::make_unique<Ort::Session>(env, modelPath.c_str(), sessionOptions);
        return true;
    }
    catch (const Ort::Exception& e)
    {
        std::cout << "Hand ONNX load failed: " << e.what() << "\n";
        return false;
    }
}

//convert opencv's Mat to vector type for onnx model to read
std::vector<float> KeypointHandDetector::preprocess(const cv::Mat& frame)
{
    LetterboxInfoHand info{};
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
std::vector<AllHandKeypoints> KeypointHandDetector::postprocess(const std::vector<float>& output, const cv::Size& originalSize)
{
    std::vector<AllHandKeypoints> keypoints;

    const int numKeypoints = 21;
    // uncomment outInfo.GetShape() to check what values for the following 2 vals
    const int numAttributes = 69; // 4 box + 1 score + 64 values per candidate. 
    const int numCandidates = 300; // characteristics/attributes
    const float confThresh = 0.15f; //only keep valid pts

    if (output.size() < static_cast<size_t>(numAttributes * numCandidates)) //if o/p too smol.
        return keypoints;

    auto at = [&](int attr, int idx) -> float
    {
        return output[attr * numAttributes + idx];
    };

#ifdef DEBUG
    // Debug: find max score
    float maxScore = 0.0f;
    int maxIdx = -1;
    for (int i = 0; i < numCandidates; ++i)
    {
        float score = at(i, 4);
        if (score > maxScore)
        {
            maxScore = score;
            maxIdx = i;
        }
    }

    std::cout << "Hand model: maxScore = " << maxScore
        << " at candidate " << maxIdx << "\n";


#endif

    float gain = std::min((float)inputSize.width / originalSize.width, (float)inputSize.height / originalSize.height);
    float padX = (inputSize.width - originalSize.width * gain) * 0.5f;
    float padY = (inputSize.height - originalSize.height * gain) * 0.5f;


    for (int i = 0; i < numCandidates; ++i)
    {
        float score = at(i, 4);
        if (score < confThresh)
            continue;

        AllHandKeypoints keypoint;
        keypoint.keypointshand.reserve(numKeypoints);
        keypoint.score = score;

        for (int k = 0; k < numKeypoints; ++k)
        {
            int attrBase = 6 + k * 3;

            float x = at(i, attrBase + 0);
            float y = at(i, attrBase + 1);
            float kpConf = at(i, attrBase + 2);

            x = (x - padX) / gain;
            y = (y - padY) / gain;

            x = std::clamp(x, 0.0f, static_cast<float>(originalSize.width - 1));
            y = std::clamp(y, 0.0f, static_cast<float>(originalSize.height - 1));

            KeypointHand kp;
            kp.x = x;
            kp.y = y;
            kp.confidence = kpConf;

            keypoint.keypointshand.push_back(kp);
        }

        keypoints.push_back(std::move(keypoint));
    }
    return keypoints;
}

std::vector<AllHandKeypoints> KeypointHandDetector::detect(const cv::Mat& frame)
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

    std::cout << "Hand Output shape = [";
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
