#pragma once
#include <Windows.h> //required for WORD keybinds

// RMB APPROACH: NAME KEYBINDS AFTER MOVEMENT, CUZ UR CODING MOVEMENTS, TO CORRESPOND TO THE KEY. KEY IS EASIER TO CHANGE HERE, RATHER THAN OVER THERE
struct AppConfigKeybinds 
{
	static constexpr char KEYBIND_CROUCH = 'C';
	//static constexpr int KEYBIND_CROUCH = 0x11; // int version of VK_CONTROL
	static constexpr char KEYBIND_RELOAD = 'R';
	static constexpr char KEYBIND_LGRABRSHOULDER = 'G';
	static constexpr WORD KEYBIND_RGRABLSHOULDER = VK_TAB;
	//static constexpr char KEYBIND_ACTION_F = 'F';
	//static constexpr char KEYBIND_LHANDCLOSED = ''; 
	static constexpr char KEYBIND_LARMUP = 'F';
	static constexpr char KEYBIND_RARMUP = '?';
	static constexpr char KEYBIND_LCONSTANTINE = 'V';
	static constexpr WORD KEYBIND_RCONSTANTINE = VK_SHIFT;
	static constexpr char KEYBIND_LRCONSTANTINE = 'Q';
	static constexpr char KEYBIND_LELBOW_HANG = 'E';
	static constexpr WORD KEYBIND_LEANLEFT = VK_CONTROL;
	static constexpr WORD KEYBIND_JUMP = VK_SPACE;
	static constexpr WORD KEYBIND_HOLDBREATH = VK_LMENU; // i.e. left alt
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
	static constexpr float SCALEFACTOR_LR = 2.0f;
	static constexpr float SCALEFACTOR_UD = 800.0f;
	static constexpr float SCALEFACTOR_LR_AIM = 1.0f;
	static constexpr float SCALEFACTOR_UD_AIM = 500.0f;

	static constexpr float LEFTTILT_MIN = 1.0f;
	static constexpr float RIGHTTILT_MIN = 1.0f;
};

struct AppConfigBody
{
	// HANDS TODO: MOVE TO HEAD!!! WHAT IS THIS DOING HERE?!!
	static constexpr float SCALEFACTOR_LR = 0.08f;
	static constexpr float SCALEFACTOR_UD = 0.08f;
	static constexpr float DEADZONE_WRIST_MIN = 4.0f;
	static constexpr float DEADZONE_WRIST_MAX = 350.0f;

	static constexpr float LINDEX_EXTEND_MIN = 1.8f;

	static constexpr float LTHUMB_CLOSE_MAX = 0.4f;
	static constexpr float RTHUMB_CLOSE_MAX = 0.4f;

	static constexpr float LEFTCOVERMOUTH_MAX = 0.3f;

	static constexpr float GRABLEFTSHOULDER_MAXANGLE = 0.4f;
	static constexpr float GRABRIGHTSHOULDER_MAXANGLE = 0.4f;
	static constexpr float GRABLEFTSHOULDER_MAXDIST = 0.35f;
	static constexpr float GRABRIGHTSHOULDER_MAXDIST = 0.35f;

	static constexpr float LELBOW_ANGLE_MIN = 1.1f;
	static constexpr float LELBOW_ANGLE_MAX = 1.7f;
	static constexpr float RELBOW_ANGLE_MIN = -2.0f;
	static constexpr float RELBOW_ANGLE_MAX = -1.1f;
	// BODY
	static constexpr float LEAN_MIN = 2.0f;
	static constexpr float LELBOW_KICK_WRIST = 0.4f;
	static constexpr float LELBOW_KICK_ELBOW = 0.34f;
	
	// LEGS
	static constexpr float LLEG_MAX = 2.0f;
	static constexpr float RLEG_MAX = 2.0f;
	static constexpr int WALKTIMER_MAX = 130;

	static constexpr float CROUCH_MAX = 0.3f;

	static constexpr float JUMPHIPS_MIN = 0.17f;
	static constexpr float JUMPHIPS_MAX = 0.9f;
	static constexpr float JUMPSHOULDER_MAX = 0.17f;
};