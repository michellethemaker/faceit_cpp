#pragma once
#include <opencv2/core.hpp>
// Plain Old Data (POD) object! just collates all the info the app might glean from my frame. NEAT.

struct PSHeadState
{
	bool isVisible = false;
	float headXcoord = 0.0f; //need to figure out how to translate to mouse movement
	float headYcoord = 0.0f;
	float noseXcoord = 0.0f;
	float noseYcoord = 0.0f;
	bool headXleft = false;
	bool headXright = false;
	bool headYup = false;
	bool headYdown = false;
	float headXleft_val = 0.0f;
	float headXright_val = 0.0f;
	float headYup_val = 0.0f;
	float headYdown_val = 0.0f;
	float tempX = 0.0f;
	float tempY = 0.0f;
	float yaw = 0.0f;
	float pitch = 0.0f;
	float roll = 0.0f;

	bool tiltLeft = false;
	bool tiltRight = false;
};

struct PSCalibrateHeadState
{
	float noseXcoord = 0.0f;
	float noseYcoord = 0.0f;

	float noseLeftX = 0.0f;
	float noseRightX = 0.0f;
	float noseUpY = 0.0f;
	float noseDownY = 0.0f;
};

struct PSBodyState
{
	bool isVisible = false;
	bool isStanding = false;
	bool isCrouching = false;
	bool leftArmUp = false;
	bool rightArmUp = false;
	bool grabLeftShoulder = false;
	bool grabRightShoulder = false;
	bool headLeft = false;
	bool headRight = false;
	bool toot = false;
	bool leftLegUp = false;
	bool rightLegUp = false;
	bool left = false;
	bool right = false;
	bool crouching = false;
	bool hasLeftShoulder = false;
	bool hasRightShoulder = false;

	bool leftThumbClosed = false;
	bool leftIndexClosed = false;
	bool leftPinkyClosed = false;
	bool leftClosed = false;
	bool rightThumbClosed = false;
	bool rightIndexClosed = false;
	bool rightPinkyClosed = false;
	bool rightClosed = false;

	float leftWristXcoord;
	float leftWristYcoord;
	float rightWristXcoord = 0.0f;
	float rightWristYcoord = 0.0f;
	float rightShoulderXcoord = 0.0f;
	float rightShoulderYcoord = 0.0f;
	float rightWristLR_val = 0.0f;
	float rightWristUD_val = 0.0f;

	float currAvgHips;
	float currAvgShoulders;

	cv::Rect leftHandROI;
	cv::Rect rightHandROI;

	bool hasLeftHand = false;
	bool hasRightHand = false;
	bool leftOverRightHand = false; //reload movement
	bool rightOverLeftShoulder = false; // E trigger
};

struct PSHandState
{
	bool isVisible = false;
	bool isVisibleLeft = false; // top or these 2 lines?
	bool isVisibleRight = false;
	bool indexDown= false;
	bool middleDown = false;
	bool ringDown = false;
	bool pinkyDown = false;
	bool closedFist = false;
};

struct PSPoseState
{
	PSHeadState ps_headstate;
	PSCalibrateHeadState ps_calibrateheadstate;
	PSBodyState ps_bodystate;
	PSHandState ps_handstate;

};

extern PSCalibrateHeadState g_calibHeadState; //expose calibheadstate struct so ppl can get from it. everything else has been setting vals.
