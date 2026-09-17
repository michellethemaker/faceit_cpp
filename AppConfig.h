#pragma once
#include <Windows.h> //required for WORD keybinds

struct AppConfigKeybinds 
{
	static constexpr char KEYBIND_CROUCH = 'C';
	//static constexpr int KEYBIND_CROUCH = 0x11; // int version of VK_CONTROL
	static constexpr char KEYBIND_RELOAD = 'R';
	static constexpr char KEYBIND_ACTION_E = 'E';
	static constexpr char KEYBIND_ACTION_F = 'F';
	static constexpr char KEYBIND_ATTACK = 'V';
	static constexpr char KEYBIND_SCAN = 'Q';
	static constexpr WORD KEYBIND_JUMP = VK_SPACE;
	static constexpr WORD KEYBIND_HOLDBREATH = VK_LMENU; // i.e. left alt
	static constexpr WORD KEYBIND_CUFFLINKS = VK_TAB;
};

struct AppConfigCamera
{
	static constexpr int kFrameWidth = 640;
	static constexpr int kFrameHeight = 480;
};

struct AppConfigKeypoints
{
	static constexpr float CONF_THRESHOLD = 0.5f; // lower if holding at obv wrong places
	static constexpr int INVALID_FRAMES_MAX = 10; // lower if points are stale for too long

};
struct AppConfigHead
{
	static constexpr float SCALEFACTOR_LR = 5.0f;
	static constexpr float SCALEFACTOR_UD = 1800.0f;

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

	static constexpr float LTHUMB_CLOSE_MAX = 0.4f;
	static constexpr float RTHUMB_CLOSE_MAX = 0.4f;

	static constexpr float LEFTCOVERMOUTH_MAX = 0.3f;

	static constexpr float GRABLEFTSHOULDER_MAXANGLE = 0.4f;
	static constexpr float GRABRIGHTSHOULDER_MAXANGLE = 0.4f;
	static constexpr float GRABLEFTSHOULDER_MAXDIST = 0.35f;
	static constexpr float GRABRIGHTSHOULDER_MAXDIST = 0.35f;
	// LEGS
	static constexpr float LLEG_MAX = 1.9f;
	static constexpr float RLEG_MAX = 1.9f;
	static constexpr int WALKTIMER_MAX = 135;

	static constexpr float CROUCH_MAX = 0.4f;
};