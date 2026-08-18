#include "InputController.h"
#include <Windows.h>
#include <iostream>

float screenWidth = GetSystemMetrics(SM_CXSCREEN);
float screenHeight = GetSystemMetrics(SM_CYSCREEN);
//float prevX = 0;
//float prevY = 0;
//float currX = 0;
//float currY = 0;
int deadzone = 15;
bool leftClick = false;
int walkTimerMax = 25;
int walkTimerCurr = 0;
bool leftPrev = false;
bool rightPrev = false;
bool leftStep = false;
bool rightStep = false;
bool leaningLeft = false;
bool leaningRight = false;
//int deadzone_L = 15;

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
	INPUT input{};
	input.type = INPUT_KEYBOARD;
	input.ki.wVk = keypress;
	input.ki.wScan = MapVirtualKey(keypress, MAPVK_VK_TO_VSC); //VITAL!! w/o scancodes the keys cant be recognised.
	input.ki.dwFlags = KEYEVENTF_SCANCODE; // TODO: should help w keeping keypress on? og docs say leave it as 0. check.
	SendInput(1, &input, sizeof(INPUT));
}

void KeyUp(WORD keypress)
{
	INPUT input{};
	input.type = INPUT_KEYBOARD;
	input.ki.wVk = keypress;
	input.ki.wScan = MapVirtualKey(keypress, MAPVK_VK_TO_VSC);
	input.ki.dwFlags = KEYEVENTF_KEYUP;
	SendInput(1, &input, sizeof(INPUT));
}

void InputController::update(const PSPoseState& state)
{

	////===================LEFT CLICK VIA LEFT ARM EXTENSION=====================
	//if (state.ps_bodystate.toot)
	//{
	//	//std::cout << "LCLICK\n";
	//	//mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
	//	//MoveRelative(1, 0);
	//	leftClick = true;
	//	//Sleep(500);
	//}
	//else if (leftClick == true && !state.ps_bodystate.toot)
	//{
	//	//mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
	//	leftClick = false;
	//}


	//===================HEAD MOVEMENT (POV CONTROL)=====================
	if (state.ps_headstate.headYup)
	{
		MoveRelative(0, -10);
	}
	else if (state.ps_headstate.headYdown)
	{
		MoveRelative(0, 10);
	}
	if (state.ps_headstate.headXleft)
	{
		MoveRelative(-10, 0);
	}
	else if (state.ps_headstate.headXright)
	{
		MoveRelative(10, 0);
	}

	//===================LEFT/RIGHT TRIGGER(A, D CONTROL)=====================
	if (state.ps_bodystate.left ) //&& !leaningLeft
	{
		//std::cout << "LEFT\n";
		KeyDown('A');
		leaningLeft = true;
	}
	else
	{
		leaningLeft = false;
		KeyUp('A');
	}
		
	if (state.ps_bodystate.right )//&& !leaningRight
	{
		//std::cout << "         RIGHT\n";
		KeyDown('D');
		leaningRight = true;
	}
	else
	{
		leaningRight = false;
		KeyUp('D');
	}
	//===================WALKING=====================
	if (state.ps_bodystate.leftLegUp ^ leftPrev && state.ps_bodystate.leftLegUp == false) // falling edge
	{
		leftStep = true; //we did a left steppy
		if (rightStep && walkTimerCurr >= 0)
		{
			std::cout << "STEPPY\n";
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
			std::cout << "STEPPY\n";
			leftStep = false;
			rightStep = false;
			walkTimerCurr = walkTimerMax;
		}
	}
	
	if (walkTimerCurr > 0)
	{
		std::cout << walkTimerCurr<<"STEPPY\n";
		if (walkTimerCurr == 15)
		{
			KeyDown('W');
		}
		
		walkTimerCurr--;
	}
	else
	{
		KeyUp('W');
		walkTimerCurr = 0;
	}
	leftPrev = state.ps_bodystate.leftLegUp; //update prev bools
	rightPrev = state.ps_bodystate.rightLegUp;
	//std::cout << walkTimerCurr << "||" << leftPrev << "||" << rightPrev << ")(" << leftStep << "||" << rightStep << "\n";
	//if (state.ps_headstate.headXcoord) // TODO: check if this actually checks presence of head. what returns when no head present?
	//{

	//	currX = state.ps_headstate.headXcoord * screenWidth;
	//	currY = state.ps_headstate.headYcoord * screenHeight;
	//	if (std::abs(currX - prevX) < deadzone_S)
	//	{
	//		currX = prevX;
	//	}
	//	else
	//		prevX = currX;
	//	
	//	if (std::abs(currY - prevY) < deadzone_S)
	//	{
	//		currY = prevY;
	//	}
	//	else
	//		prevY = currY;


	//	SetCursorPos(currX, currY);
	//	//std::cout << "====\nmousepos: \n" << currX << "||" << currY << "\n" << prevX << "||" << prevY << "\n=====\n";
	//}

}