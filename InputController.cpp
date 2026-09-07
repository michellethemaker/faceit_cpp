#include "InputController.h"
#include <Windows.h>
#include <iostream>
#include "KeyboardConstants.h"

float screenWidth = GetSystemMetrics(SM_CXSCREEN);
float screenHeight = GetSystemMetrics(SM_CYSCREEN);
//float prevX = 0;
//float prevY = 0;
//float currX = 0;
//float currY = 0;
int deadzone = 15;
bool leftClick = false;
bool rightClick = false;
int walkTimerMax = 15;
int walkTimerCurr = 0;
bool leftPrev = false;
bool rightPrev = false;
bool leftStep = false;
bool rightStep = false;
bool leaningLeft = false;
bool leaningRight = false;
bool crouching = false;
bool reloading = false;
bool etrigger = false;
float avgHips;
float avgShoulders;
//int deadzone_L = 15;

using namespace keyboardconstants;

void MoveRelative(LONG dx, LONG dy)
{
	INPUT input{};
	input.type = INPUT_MOUSE;
	input.mi.dwFlags = MOUSEEVENTF_MOVE;
	input.mi.dx = dx;
	input.mi.dy = dy;

	UINT result = SendInput(1, &input, sizeof(INPUT));

	if (result != 1)
		std::cerr << "SendInput failed\n";
}

void KeyDown(WORD keypress) // W, A, VK_SPACE etc
{
	//INPUT input{};
	//input.type = INPUT_KEYBOARD;
	//input.ki.wVk = keypress;
	//input.ki.wScan = MapVirtualKey(keypress, MAPVK_VK_TO_VSC); //VITAL!! w/o scancodes the keys cant be recognised.
	//input.ki.dwFlags = KEYEVENTF_SCANCODE; // default was 0, but scancode required in fps games
	//SendInput(1, &input, sizeof(INPUT));
}

void KeyUp(WORD keypress)
{
	//INPUT input{};
	//input.type = INPUT_KEYBOARD;
	//input.ki.wVk = keypress;
	//input.ki.wScan = MapVirtualKey(keypress, MAPVK_VK_TO_VSC);
	//input.ki.dwFlags = KEYEVENTF_KEYUP;
	//SendInput(1, &input, sizeof(INPUT));
}

void InputController::update(const PSPoseState& state)
{

	////===================LEFT CLICK VIA LEFT ARM EXTENSION=====================
	if (state.ps_bodystate.grabLeftShoulder)
	{
		std::cout << "LCLICK\n";
		//mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
		//MoveRelative(1, 0);
		leftClick = true;
		//Sleep(500);
	}
	else if (leftClick == true && !state.ps_bodystate.grabLeftShoulder)
	{
		//mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
		leftClick = false;
	}

	if (state.ps_bodystate.grabRightShoulder)
	{
		std::cout << "RCLICK\n";
		//mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, 0);
		rightClick = true;
	}
	else if (rightClick == true && !state.ps_bodystate.grabRightShoulder)
	{
		//mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, 0);
	}

	
	//===================HEAD MOVEMENT (POV CONTROL)=====================
	//scaled acc to how much u turn ur head. need to handle the scale, make sure up/down scale = left/right scale.
	if (state.ps_bodystate.hasRightShoulder) // UP/DOWN
	{
		//std::cout << state.ps_headstate.headYup_val<<"\n";
		MoveRelative(0, 0.2f*(state.ps_bodystate.rightWristYcoord - state.ps_bodystate.rightShoulderYcoord));//TODO: fix these, normalise somehow.
	}

	if (state.ps_bodystate.hasRightShoulder)
	{
		//std::cout << state.ps_headstate.headXleft_val << "\n";
		MoveRelative(0.2f*(state.ps_bodystate.rightShoulderXcoord - state.ps_bodystate.rightWristXcoord), 0);
	}


	//===================LEFT/RIGHT TRIGGER(A, D CONTROL)=====================
	if (state.ps_bodystate.left ) //&& !leaningLeft
	{
		//std::cout << "LEFT\n";
		//KeyDown('A');
		leaningLeft = true;
	}
	else if (!state.ps_bodystate.left && leaningLeft == true)
	{
		//KeyUp('A');
		leaningLeft = false;
	}
		
	if (state.ps_bodystate.right )//&& !leaningRight
	{
		//std::cout << "         RIGHT\n";
		//KeyDown('D');
		leaningRight = true;
	}
	else if (!state.ps_bodystate.right && leaningRight == true)
	{
		//KeyUp('D');
		leaningRight = false;
	}

	//===================WALKING=====================

	if (state.ps_bodystate.leftLegUp ^ leftPrev && state.ps_bodystate.leftLegUp == false) // falling edge
	{
			leftStep = true; //we did a left steppy
			if (rightStep && walkTimerCurr >= 0)
			{
				leftStep = false;
				rightStep = false;
				walkTimerCurr = walkTimerMax;
			}
	}
	if (state.ps_bodystate.rightLegUp ^ rightPrev && state.ps_bodystate.rightLegUp == false) // falling edge
	{

			rightStep = true; //we did a right steppy

			if (leftStep && walkTimerCurr >= 0)
			{
				leftStep = false;
				rightStep = false;
				walkTimerCurr = walkTimerMax;
			}
	}
	
	if (walkTimerCurr > 0)
	{
		//std::cout << walkTimerCurr<<"STEPPY\n";
		if (walkTimerCurr > 5)
		{
			KeyDown('W');
			std::cout << "WALKIN\n";
		}
		
		walkTimerCurr--;
	}
	else
	{
		KeyUp('W');
		walkTimerCurr = 0;
	}
	//std::cout << leftStep << "|" << rightStep << "||" << walkTimerCurr << "\n";
	leftPrev = state.ps_bodystate.leftLegUp; //update prev bools
	rightPrev = state.ps_bodystate.rightLegUp;
	
	
	if (avgHips -state.ps_bodystate.currAvgHips > 0.13f && avgHips - state.ps_bodystate.currAvgHips < 0.6f &&
		avgShoulders - state.ps_bodystate.currAvgShoulders > 0.19f) 
	{
		KeyDown(VK_SPACE);
		KeyUp(VK_SPACE);
		std::cout << "JUMP\n";
		std::cout << avgHips - state.ps_bodystate.currAvgHips << "{}"<< avgShoulders - state.ps_bodystate.currAvgShoulders << "\n";
	}
	avgHips = state.ps_bodystate.currAvgHips;
	avgShoulders = state.ps_bodystate.currAvgShoulders;
	// 
	//===================CROUCHING=====================
	//std::cout<< state.ps_bodystate.crouching <<"|"<< crouching << "\n";
	if (state.ps_bodystate.crouching && crouching == false) 
	{
		KeyDown(KEYBIND_CROUCH);
		KeyUp(KEYBIND_CROUCH);
		crouching = true;
		std::cout << "CROUCHING\n";
	}
	if (!state.ps_bodystate.crouching && crouching == true)
	{
		KeyDown(KEYBIND_CROUCH);
		KeyUp(KEYBIND_CROUCH);
		std::cout << "UNCROUCHING\n";
		crouching = false;
	}


	//===================HAND MOTIONS=====================
	if (state.ps_bodystate.leftOverRightHand && reloading == false)
	{
		KeyDown(KEYBIND_RELOAD);
		KeyUp(KEYBIND_RELOAD);
		reloading = true;
		std::cout << "RELOADING\n";
	}
	if (!state.ps_bodystate.leftOverRightHand && reloading == true)
	{
		KeyDown(KEYBIND_RELOAD);
		KeyUp(KEYBIND_RELOAD);
		reloading = false;
		std::cout << "END RELOAD\n";
	}
	
	if (state.ps_bodystate.rightOverLeftShoulder && etrigger == false)
	{
		KeyDown(KEYBIND_ACTION1);
		KeyUp(KEYBIND_ACTION1);
		etrigger = true;
		std::cout << "E\n";
	}
	if (!state.ps_bodystate.rightOverLeftShoulder && etrigger == true)
	{
		KeyDown(KEYBIND_ACTION1);
		KeyUp(KEYBIND_ACTION1);
		etrigger = false;
		std::cout << "END E\n";
	}

}
