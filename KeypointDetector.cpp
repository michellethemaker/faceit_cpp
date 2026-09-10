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

//std::vector<AllKeypoints> KeypointDetector::postprocess( const float* landmarkData,  size_t landmarkCount, float posePresence, const cv::Size& originalSize, const ModelPreprocessInfo& info)
//{
//    constexpr int numLandmarks = 39;
//    constexpr int valuesPerLandmark = 5;
//
//    if (landmarkCount <
//        numLandmarks * valuesPerLandmark)
//    {
//        return {};
//    }
//
//    AllKeypoints pose;
//
//    pose.keypoints.reserve(33); //39 includes 6 auxiliary pts, not compulsory
//
//    for (int i = 0; i < 33; ++i)
//    {
//        const float* landmark = landmarkData + i * valuesPerLandmark;
//
//        /*
//         * MediaPipe landmark output is:
//         *
//         * [x, y, z, visibility, presence]
//         * x/y are in the 256x256 model coordinate system. NOT NORMALISED
//         */
//
//        float x = landmark[0];
//        float y = landmark[1];
//        // visibility/presence are logits.
//        // Convert to probabilities.
//        float visibility =
//            1.0f / (1.0f + std::exp(-landmark[3]));
//
//        float presence =
//            1.0f / (1.0f + std::exp(-landmark[4]));
//
//
//        float originalX =
//            (x * info.scale) - info.padX;
//
//        float originalY =
//            (y * info.scale) - info.padY;
//
//        Keypoint kp;
//
//        kp.x = originalX;
//        kp.y = originalY;
//
//        kp.confidence = 0.6f;//std::min(visibility, presence);
//
//        // Clamp to camera frame.
//        kp.x = std::clamp(
//            kp.x,
//            0.0f,
//            static_cast<float>(originalSize.width - 1)
//        );
//
//        kp.y = std::clamp(
//            kp.y,
//            0.0f,
//            static_cast<float>(originalSize.height - 1)
//        );
//
//        pose.keypoints.push_back(kp);
//    }
//
//    pose.score = posePresence;
//
//    return { pose };
//}

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