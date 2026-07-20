#pragma once

// Plain Old Data (POD) object! just collates all the info the app might glean from my frame. NEAT.

struct HeadState
{
	bool isVisible = false;
	float tilt = 0.0f; //need to figure out how to translate to mouse movement
};

struct BodyState
{
	bool isVisible = false;
	bool isStanding = false;
	bool isCrouching = false;
	bool leftArmUp = false;
	bool rightArmUp = false;
	bool headLeft = false;
	bool headRight = false;

};

struct HandState
{
	bool isVisibleLeft = false;
	bool isVisibleRight = false;
};

struct PoseState
{
	HeadState headstate;
	BodyState bodystate;
	HandState handstate;
};