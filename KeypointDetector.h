#pragma once
#include <onnxruntime_cxx_api.h>
#include <opencv2/opencv.hpp>
#include <memory>
#include <string>
#include <vector>
#include "Keypoint.h"
//threading headers
#include <thread>
#include <mutex>
#include <atomic>
//#define YOLO
#define MEDIAPIPE

struct LetterboxInfo
{
	float gain;
	float padX;
	float padY;
};


struct ModelPreprocessInfo
{
	float scale;
	float padX;
	float padY;
};

class KeypointDetector
{
public:
	KeypointDetector();
	~KeypointDetector(); //destructor

	// worker thread stuff
	void start();
	void stop();
	void pushFrame(const cv::Mat& frame); // call from main thread per frame
	bool getLatestPose(AllKeypoints& out); //call from main thread per frame too, false if no pose ready
	
	bool loadPersonModel(const std::wstring& modelPath); // for person detection in mediapipe
	bool loadModel(const std::wstring& modelPath);

//	bool detectPerson(const cv::Mat& frame, PersonROI& roi); // just detect person
	std::vector<AllKeypoints> detect(const cv::Mat& frame); //const to be readonly; Mat& to reference frame (no duplicates!)



	void printModelInfo();
	void printPoseModelInfo();

private:
	void workerLoop(); //worker thread
	cv::Size personInputSize{ 224,224 }; // for person detection in mediapipe
	cv::Size inputSize{256,256}; //640, 640 for yolo26

	Ort::Env env;
	Ort::SessionOptions sessionOptions; //session settings (e.g. optimisation level)
	std::unique_ptr<Ort::Session> personSession;  // for person detection in mediapipe
	std::unique_ptr<Ort::Session> session;

#ifdef MEDIAPIPE
//	std::vector<PersonAnchor> createPersonAnchors();
//	std::vector<float> preprocessPerson(const cv::Mat& frame, PersonPreprocessInfo& info);
	std::vector<float> preprocess(const cv::Mat& frame, ModelPreprocessInfo& info); //preprocess frame
	//std::vector<AllKeypoints> postprocess(const float* landmarkData, size_t landmarkCount,float posePresence, 
	//								const cv::Size& originalSize, const ModelPreprocessInfo& info);
#endif
	// the threading members
	std::thread workerThread;
	std::atomic<bool> running{ false };

	// these go from main -> worker (i.e. frames to process)
	cv::Mat pendingFrame;
	bool hasPendingFrame = false;
	std::mutex frameMutex;

	// these go from worker -> main (all the latest pose results)
	AllKeypoints latestPose;
	bool hasLatestPose = false;
	std::mutex poseMutex;
};