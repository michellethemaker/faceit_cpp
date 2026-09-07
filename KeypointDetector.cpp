#include "KeypointDetector.h"
#include <iostream>
#include <cstring>
#define NDEBUG
#define MEDIAPIPE
//#define YOLO


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

//bool KeypointDetector::loadPersonModel(const std::wstring& modelPath)
//{
//    try
//    {
//        personSession = std::make_unique<Ort::Session>(env, modelPath.c_str(), sessionOptions);
//        std::cout << "Person detector loaded\n";
//        return true;
//    }
//    catch (const Ort::Exception& e)
//    {
//        std::cerr << "Person detector load failed: " << e.what() << "\n";
//        return false;
//    }
//}

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

#ifdef YOLO
//convert opencv's Mat to vector type for onnx model to read
std::vector<float> KeypointDetector::preprocess(const cv::Mat& frame)
{
    LetterboxInfo info{};
    cv::Mat boxed = letterbox(frame, inputSize, info);
    cv::Mat rgb, floatImg;
    cv::cvtColor(boxed, rgb, cv::COLOR_BGR2RGB);
    rgb.convertTo(floatImg, CV_32F, 1.0f / 255.0f);

// yolo expects NCHW, [1,3,H,W]
    std::vector<float> inputTensorValues(3.0f * inputSize.width * inputSize.height);

    std::vector<cv::Mat> channels(3);
    for (int i = 0; i < 3; ++i)
    {
        channels[i] = cv::Mat(inputSize.height, inputSize.width, CV_32F,
            inputTensorValues.data() + i * inputSize.width * inputSize.height);
    }

    cv::split(floatImg, channels);
    return inputTensorValues;
}
#endif

#ifdef MEDIAPIPE
//std::vector<PersonAnchor> KeypointDetector::createPersonAnchors()
//{
//    std::vector<PersonAnchor> anchors;
//    anchors.reserve(2254);
//
//    // Feature map 1:
//    // 28 x 28 locations
//    // 2 anchors per location
//    // 28 * 28 * 2 = 1568
//
//    for (int y = 0; y < 28; ++y)
//    {
//        for (int x = 0; x < 28; ++x)
//        {
//            float anchorX = (x + 0.5f) / 28.0f;
//            float anchorY = (y + 0.5f) / 28.0f;
//            anchors.push_back( { anchorX, anchorY });
//            anchors.push_back( { anchorX, anchorY });
//        }
//    }
//
//    // Feature map 2:
//    // 14 x 14 locations
//    // 2 anchors per location
//    // 14 * 14 * 2 = 392
//
//    for (int y = 0; y < 14; ++y)
//    {
//        for (int x = 0; x < 14; ++x)
//        {
//            float anchorX = (x + 0.5f) / 14.0f;
//            float anchorY = (y + 0.5f) / 14.0f;
//            anchors.push_back({ anchorX, anchorY });
//            anchors.push_back({ anchorX, anchorY });
//        }
//    }
//
//    // Feature map 3:
//    // 7 x 7 locations
//    // 6 anchors per location
//    // 7 * 7 * 6 = 294
//
//    for (int y = 0; y < 7; ++y)
//    {
//        for (int x = 0; x < 7; ++x)
//        {
//            float anchorX = (x + 0.5f) / 7.0f;
//            float anchorY = (y + 0.5f) / 7.0f;
//            for (int i = 0; i < 6; ++i)
//            {
//                anchors.push_back({ anchorX, anchorY });
//            }
//        }
//    }
//
//    //std::cout << "Number of anchors = " << anchors.size() << "\n";
//
//    return anchors;
//}
//
//std::vector<float> KeypointDetector::preprocessPerson(
//    const cv::Mat& frame,
//    PersonPreprocessInfo& info)
//{
//    constexpr int INPUT_SIZE = 224;
//
//    const int origW = frame.cols;
//    const int origH = frame.rows;
//
//    // Compute square canvas size
//    const float scale = static_cast<float>(std::max(origW, origH));
//    const int   squareSize = static_cast<int>(scale);
//
//    // Compute padding to center the original image on the square
//    const int padX = (squareSize - origW) / 2;
//    const int padY = (squareSize - origH) / 2;
//
//    // Save for decoding
//    info.scale = scale;
//    info.padX = static_cast<float>(padX);
//    info.padY = static_cast<float>(padY);
//
//    // Create square canvas (BGR for now)
//    cv::Mat square( squareSize, squareSize, frame.type(), cv::Scalar(0, 0, 0));
//
//    // Paste original image into center
//    frame.copyTo( square(cv::Rect(padX, padY, origW, origH)) );
//
//    // Convert to RGB and normalize to [-1, 1]
//    cv::Mat rgb;
//    cv::cvtColor(square, rgb, cv::COLOR_BGR2RGB);
//
//    cv::Mat norm;
//    rgb.convertTo(norm, CV_32FC3, 1.0f / 255.0f); // [0,1]
//    norm = (norm - 0.5f) * 2.0f; // [-1,1]
//
//    // Resize to 224x224
//    cv::Mat resized;
//    cv::resize( norm, resized, cv::Size(INPUT_SIZE, INPUT_SIZE), 0, 0, cv::INTER_AREA );
//
//    // HWC -> CHW, fill output buffer [3 * 224 * 224]
//    std::vector<float> inputData(3 * INPUT_SIZE * INPUT_SIZE);
//    std::vector<cv::Mat> channels(3);
//    for (int c = 0; c < 3; ++c)
//    {
//        channels[c] = cv::Mat( INPUT_SIZE, INPUT_SIZE, CV_32F, inputData.data() + c * INPUT_SIZE * INPUT_SIZE );
//    }
//    cv::split(resized, channels);
//    return inputData;
//}


std::vector<float> KeypointDetector::preprocess(const cv::Mat& frame, MediaPipePreprocessInfo& info)
{
    int width = frame.cols;
    int height = frame.rows;

    int squareSize = std::max(width, height);

    int padX = (squareSize - width) / 2;
    int padY = (squareSize - height) / 2;

    cv::Mat square(squareSize, squareSize, frame.type(), cv::Scalar(0, 0, 0));

    frame.copyTo(square(cv::Rect(padX, padY, width, height)));
    cv::Mat resized;

    //resize square to 256x256
    cv::resize( square, resized, inputSize, 0, 0, cv::INTER_AREA);

    // bgr to rgb, float32, normalise to [0,1]
    cv::Mat rgb;
    cv::cvtColor( resized, rgb, cv::COLOR_BGR2RGB);
    cv::Mat rgbFloat;
    rgb.convertTo(rgbFloat, CV_32FC3, 1.0 / 255.0);
    
    info.scale =static_cast<float>(squareSize) / static_cast<float>(inputSize.width);
    info.padX = static_cast<float>(padX);
    info.padY = static_cast<float>(padY);

//mpipe expects NHWC, [1, 3, H,W]
    std::vector<float> inputData( 256 * 256 * 3 );
    std::memcpy(inputData.data(), rgbFloat.ptr<float>(), inputData.size() * sizeof(float) );
    return inputData;


}
#endif

#ifdef YOLO
//convert model output to actual keypoints (in original image coords!)
std::vector<AllKeypoints> KeypointDetector::postprocess(const std::vector<float>& output, const cv::Size& originalSize)
{
    std::vector<AllKeypoints> keypoints;

//#ifdef YOLO
    const int numKeypoints = 17;
    const int numAttributes = 56; // 4 box + 1 score + 51 kps. uncomment outInfo.GetShape() to check.
    const int numCandidates = 8400;
//#endif
#ifdef MEDIAPIPE
#endif

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
#endif

#ifdef MEDIAPIPE
std::vector<AllKeypoints> KeypointDetector::postprocess( const float* landmarkData,  size_t landmarkCount, float posePresence, const cv::Size& originalSize, const MediaPipePreprocessInfo& info)
{
    constexpr int numLandmarks = 39;
    constexpr int valuesPerLandmark = 5;

    if (landmarkCount <
        numLandmarks * valuesPerLandmark)
    {
        return {};
    }

    AllKeypoints pose;

    pose.keypoints.reserve(33); //39 includes 6 auxiliary pts, not compulsory

    for (int i = 0; i < 33; ++i)
    {
        const float* landmark = landmarkData + i * valuesPerLandmark;

        /*
         * MediaPipe landmark output is:
         *
         * [x, y, z, visibility, presence]
         * x/y are in the 256x256 model coordinate system. NOT NORMALISED
         */

        float x = landmark[0];
        float y = landmark[1];
        // visibility/presence are logits.
        // Convert to probabilities.
        float visibility =
            1.0f / (1.0f + std::exp(-landmark[3]));

        float presence =
            1.0f / (1.0f + std::exp(-landmark[4]));


        float originalX =
            (x * info.scale) - info.padX;

        float originalY =
            (y * info.scale) - info.padY;

        Keypoint kp;

        kp.x = originalX;
        kp.y = originalY;

        kp.confidence = 0.6f;//std::min(visibility, presence);

        // Clamp to camera frame.
        kp.x = std::clamp(
            kp.x,
            0.0f,
            static_cast<float>(originalSize.width - 1)
        );

        kp.y = std::clamp(
            kp.y,
            0.0f,
            static_cast<float>(originalSize.height - 1)
        );

        pose.keypoints.push_back(kp);
    }

    pose.score = posePresence;

    return { pose };
}

#endif

#ifdef YOLO
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

    size_t outCount = outInfo.GetElementCount(); //total float vals in op

    const float* outData = outTensor.GetTensorData<float>(); //raw ptr to tensor contents

    #ifdef DEBUG
        auto outShape = outInfo.GetShape(); // Get actual shape.

        std::cout << "Output shape = [";
        for (size_t i = 0; i < outShape.size(); ++i)
        {
            if (i) std::cout << ", ";
            std::cout << outShape[i];
        }
        std::cout << "]\n";

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
#endif

#ifdef MEDIAPIPE
//bool KeypointDetector::detectPerson(const cv::Mat& frame, PersonROI& roi)
//{
//    if (!personSession)
//        return false;
//    // Preprocess
//    PersonPreprocessInfo preprocessInfo;
//    std::vector<float> inputData = preprocessPerson(frame, preprocessInfo);
//
//    // Build input tensor [1, 3, 224, 224]
//    std::array<int64_t, 4> inputShape = { 1, 3, 224, 224 };
//
//    Ort::MemoryInfo memoryInfo =
//        Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
//
//    Ort::Value inputTensor = Ort::Value::CreateTensor<float>(
//        memoryInfo,
//        inputData.data(),
//        inputData.size(),
//        inputShape.data(),
//        inputShape.size()
//        );
//
//    // Input/output names
//    Ort::AllocatorWithDefaultOptions allocator;
//
//    auto inputName = personSession->GetInputNameAllocated(0, allocator);
//    const char* inputNames[] = { inputName.get() };
//
//    const size_t numOutputs = personSession->GetOutputCount();
//    std::vector<Ort::AllocatedStringPtr> outputNameStorage;
//    std::vector<const char*> outputNames;
//    outputNameStorage.reserve(numOutputs);
//    outputNames.reserve(numOutputs);
//
//    for (size_t i = 0; i < numOutputs; ++i) {
//        outputNameStorage.push_back(
//            personSession->GetOutputNameAllocated(i, allocator)
//        );
//        outputNames.push_back(outputNameStorage.back().get());
//    }
//
//    // Run model
//    auto outputTensors = personSession->Run(
//        Ort::RunOptions{ nullptr },
//        inputNames,
//        &inputTensor,
//        1,
//        outputNames.data(),
//        static_cast<int>(outputNames.size())
//    );
//
//    if (outputTensors.size() != 2) {
//        std::cerr << "Expected 2 outputs from person detector, got "
//            << outputTensors.size() << "\n";
//        return false;
//    }
//
//    // Output 0: scores [1, 2254, 1]
//    // Output 1: box_land_delta [1, 2254, 12]
//
//    const float* scoreData = outputTensors[0].GetTensorData<float>();
//    const float* boxData = outputTensors[1].GetTensorData<float>();
//
//    // Find best detection
//    int bestIndex = -1;
//    float bestScore = 0.0f;
//    
//    for (int i = 0; i < 2254; ++i) {
//        float logit = scoreData[i];
//        float score = 1.0f / (1.0f + std::exp(-logit)); // sigmoid
//
//        if (score > bestScore) {
//            bestScore = score;
//            bestIndex = i;
//        }
//    }
//
//    constexpr float scoreThreshold = 0.5f;
//    if (bestIndex < 0 || bestScore < scoreThreshold) {
//        return false;
//    }
//
//    // Decode box for bestIndex
//    const auto anchors = createPersonAnchors();
//    if (anchors.size() != 2254) {
//        std::cerr << "Anchor count mismatch\n";
//        return false;
//    }
//
//    const PersonAnchor& anchor = anchors[bestIndex];
//    const float* det = boxData + bestIndex * 12; // 12 values per detection
//
//    // det[0..3] = [dx, dy, dw, dh]
//    constexpr float modelSize = 224.0f;
//
//    float dx = det[0] / modelSize;
//    float dy = det[1] / modelSize;
//    float dw = det[2] / modelSize;
//    float dh = det[3] / modelSize;
//
//    float cx = dx + anchor.x;
//    float cy = dy + anchor.y;
//    float w = dw;
//    float h = dh;
//
//    float x1_norm = cx - w * 0.5f;
//    float y1_norm = cy - h * 0.5f;
//    float x2_norm = cx + w * 0.5f;
//    float y2_norm = cy + h * 0.5f;
//
//    // Convert to original-image coordinates
//    float x1 = (x1_norm * preprocessInfo.scale) - preprocessInfo.padX;
//    float y1 = (y1_norm * preprocessInfo.scale) - preprocessInfo.padY;
//    float x2 = (x2_norm * preprocessInfo.scale) - preprocessInfo.padX;
//    float y2 = (y2_norm * preprocessInfo.scale) - preprocessInfo.padY;
//
//    // Clamp
//    x1 = std::clamp(x1, 0.0f, static_cast<float>(frame.cols - 1));
//    y1 = std::clamp(y1, 0.0f, static_cast<float>(frame.rows - 1));
//    x2 = std::clamp(x2, 0.0f, static_cast<float>(frame.cols - 1));
//    y2 = std::clamp(y2, 0.0f, static_cast<float>(frame.rows - 1));
//
//    std::cout << "Best index: " << bestIndex << "\n";
//    std::cout << "Best score: " << bestScore << "\n";
//    std::cout << "Raw 12 values for best detection:\n";
//    for (int i = 0; i < 12; ++i) {
//       if(i == 2 || i ==3)
//        std::cout << "det[" << i << "] = " << det[i] << "\n";
//    }
//    std::cout << "Anchor: (" << anchor.x << ", " << anchor.y << ")\n";
//    std::cout << "preprocessInfo.scale = " << preprocessInfo.scale << "\n";
//    std::cout << "preprocessInfo.padX = " << preprocessInfo.padX
//        << ", padY = " << preprocessInfo.padY << "\n";
//
//
//    // Build ROI struct
//    float boxWidth = x2 - x1;
//    float boxHeight = y2 - y1;
//
//    roi.center = cv::Point2f((x1 + x2) * 0.5f, (y1 + y2) * 0.5f);
//    roi.size = cv::Size2f(boxWidth, boxHeight);
//    roi.rotation = 0.0f;
//    roi.confidence = bestScore;
//
//    return true;
//}


std::vector<AllKeypoints> KeypointDetector::detect(const cv::Mat& frame)
{
    if (!session)
        return {};
    //PersonROI roi;

    //if (!detectPerson(frame, roi))
    //    return {};
    MediaPipePreprocessInfo preprocessInfo;
    std::vector<float> inputData = preprocess(frame, preprocessInfo);

    std::array<int64_t, 4> inputShape = { 1, 256, 256, 3 };

    Ort::MemoryInfo memoryInfo = Ort::MemoryInfo::CreateCpu( OrtArenaAllocator, 
        OrtMemTypeDefault );

    Ort::Value inputTensor =
        Ort::Value::CreateTensor<float>(
            memoryInfo,
            inputData.data(),
            inputData.size(),
            inputShape.data(),
            inputShape.size()
            );

    Ort::AllocatorWithDefaultOptions allocator;

    // Input
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

    if (outputTensors.size() < 5)
    {
        std::cerr << "Expected 5 outputs, got " << outputTensors.size() << "\n";
        return {};
    }

    // Output 0: [1,195]
    // 33 landmarks × 5 values

    const Ort::Value& landmarkTensor = outputTensors[0];
    auto landmarkInfo = landmarkTensor.GetTensorTypeAndShapeInfo();
    const float* landmarkData = landmarkTensor.GetTensorData<float>();
    size_t landmarkCount = landmarkInfo.GetElementCount();

    if (landmarkCount != 195)
    {
        std::cerr << "Unexpected landmark output size: " << landmarkCount << "\n";
        return {};
    }

    // Output 1: [1,1]
    // Pose presence/confidence

    const Ort::Value& presenceTensor = outputTensors[1];
    const float* presenceData = presenceTensor.GetTensorData<float>();
    float posePresence = presenceData[0];

    // Reject if the model doesn't think a pose is present.
    if (posePresence < 0.5f) return {};

    return postprocess(landmarkData,landmarkCount,posePresence,frame.size(),preprocessInfo);
}

#endif

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
}