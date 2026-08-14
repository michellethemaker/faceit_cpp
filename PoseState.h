#pragma once

// Plain Old Data (POD) object! just collates all the info the app might glean from my frame. NEAT.

struct PSHeadState
{
	bool isVisible = false;
	float headXcoord = 0.0f; //need to figure out how to translate to mouse movement
	float headYcoord = 0.0f;
	float noseXcoord = 0.0f;
	float noseYcoord = 0.0f;
	float tempX = 0.0f;
	float tempY = 0.0f;
	float yaw = 0.0f;
	float pitch = 0.0f;
	float roll = 0.0f;
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
	bool headLeft = false;
	bool headRight = false;

};

struct PSHandState
{
	bool isVisibleLeft = false;
	bool isVisibleRight = false;
};

struct PSPoseState
{
	PSHeadState ps_headstate;
	PSCalibrateHeadState ps_calibrateheadstate;
	PSBodyState ps_bodystate;
	PSHandState ps_handstate;

};

extern PSCalibrateHeadState g_calibHeadState; //expose calibheadstate struct so ppl can get from it. everything else has been setting vals.
