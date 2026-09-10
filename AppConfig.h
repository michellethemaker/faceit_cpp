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

};