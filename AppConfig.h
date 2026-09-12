#pragma once

struct AppConfigKeybinds 
{
	static constexpr char KEYBIND_CROUCH = 'C';
	//static constexpr int KEYBIND_CROUCH = 0x11; // int version of VK_CONTROL
	static constexpr char KEYBIND_RELOAD = 'R';
	static constexpr char KEYBIND_ACTION1 = 'E';
	static constexpr char KEYBIND_ACTION2 = 'F';
};

struct AppConfigCamera
{
	static constexpr int kFrameWidth = 640;
	static constexpr int kFrameHeight = 480;
};

struct AppConfigHead
{
	static constexpr float SCALEFACTOR_LR = 0.8f;
	static constexpr float SCALEFACTOR_UD = 0.8f;

	static constexpr float LEFTTILT_MIN = 1.4f;
	static constexpr float RIGHTTILT_MIN = 1.4f;
};

struct AppConfigBody
{
	// HANDS TODO: MOVE TO HEAD!!! WHAT IS THIS DOING HERE?!!
	static constexpr float SCALEFACTOR_LR = 0.08f;
	static constexpr float SCALEFACTOR_UD = 0.08f;
	static constexpr float DEADZONE_WRIST_MIN = 4.0f;
	static constexpr float DEADZONE_WRIST_MAX = 350.0f;

	static constexpr float LTHUMB_CLOSE_MAX = 0.4;
	static constexpr float RTHUMB_CLOSE_MAX = 0.4;
	// LEGS
	static constexpr float LLEG_MAX = 1.9f;
	static constexpr float RLEG_MAX = 1.9f;
	static constexpr int WALKTIMER_MAX = 135;

	static constexpr float CROUCH_MIN = 0.39f;
};