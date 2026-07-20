#pragma once
#include <onnxruntime_cxx_api.h>
#include <opencv2/opencv.hpp>
#include <memory>
#include <string>
#include <vector>
#include "Keypoint.h"
	
struct LetterboxInfo
{
	float gain;
	float padX;
	float padY;
};

class KeypointDetector
{
public:
	KeypointDetector();
	bool loadModel(const std::wstring& modelPath);
	std::vector<AllKeypoints> detect(const cv::Mat& frame); //const to be readonly; Mat& to reference frame (no duplicates!)

private:
	cv::Size inputSize{640, 640};

	Ort::Env env;
	Ort::SessionOptions sessionOptions; //session settings (e.g. optimisation level)
	std::unique_ptr<Ort::Session> session;

	std::vector<float> preprocess(const cv::Mat& frame); //preprocess frame
	std::vector<AllKeypoints> postprocess(const std::vector<float>& output,
								  const cv::Size& originalSize);
};