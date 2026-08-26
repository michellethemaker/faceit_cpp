#pragma once
#include <onnxruntime_cxx_api.h>
#include <opencv2/opencv.hpp>
#include <memory>
#include <string>
#include <vector>
#include "KeypointHand.h"

//threading headers
#include <thread>
#include <mutex>
#include <atomic>

struct LetterboxInfoHand
{
	float gain;
	float padX;
	float padY;
};

class KeypointHandDetector
{
public:
	KeypointHandDetector();
	~KeypointHandDetector(); //destructor

	// worker thread stuff
	void start();
	void stop();
	void pushFrame(const cv::Mat& frame); // call from main thread per frame
	bool getLatestPose(AllHandKeypoints& out); //call from main thread per frame too, false if no pose ready

	bool loadModel(const std::wstring& modelPath);
	std::vector<AllHandKeypoints> detect(const cv::Mat& frame); //const to be readonly; Mat& to reference frame (no duplicates!)

private:
	void workerLoop(); //worker thread
	cv::Size inputSize{ 640, 640 };

	Ort::Env env;
	Ort::SessionOptions sessionOptions; //session settings (e.g. optimisation level)
	std::unique_ptr<Ort::Session> session;

	std::vector<float> preprocess(const cv::Mat& frame); //preprocess frame
	std::vector<AllHandKeypoints> postprocess(const std::vector<float>& output,
		const cv::Size& originalSize);

	// the threading members
	std::thread workerThread;
	std::atomic<bool> running{ false };

	// these go from main -> worker (i.e. frames to process)
	cv::Mat pendingFrame;
	bool hasPendingFrame = false;
	std::mutex frameMutex;

	// these go from worker -> main (all the latest pose results)
	AllHandKeypoints latestPose;
	bool hasLatestPose = false;
	std::mutex poseMutex;
};