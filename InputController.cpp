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
int walkTimerMax = 5;
int walkTimerCurr = 1;
bool leftPrev = false;
bool rightPrev = false;
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

void InputController::update(const PSPoseState& state)
{

		//std::cout << "LEFT ARM: " << state.ps_bodystate.leftArmUp << "\n";
		if (state.ps_bodystate.toot)
		{
			//std::cout << "LCLICK\n";
			//mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
			//MoveRelative(1, 0);
			leftClick = true;
			//Sleep(500);
		}
		else if (leftClick == true && !state.ps_bodystate.toot )
		{
			//mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
			leftClick = false;
		}



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


		if (state.ps_bodystate.leftLegUp ^ state.ps_bodystate.rightLegUp) // only ONE up! start walking check
		{
			if (state.ps_bodystate.leftLegUp ) // left up first
			{
				if (state.ps_bodystate.leftLegUp ^ leftPrev) //oscillation occured
				{
					walkTimerCurr++;
				}
				else // no osc
				{
					walkTimerCurr--; 
				}
				leftPrev = true; // record left as up 
				rightPrev = false; // record right as down
				std::cout << walkTimerCurr<<" left\n";
			}
			else // right up first
			{
				if (state.ps_bodystate.rightLegUp ^ rightPrev)
				{
					walkTimerCurr++;
				}
				else
				{
					walkTimerCurr--;
				}
				rightPrev = true;
				leftPrev = false;
				std::cout << "right " <<walkTimerCurr << "\n";
			}
		}
		if (walkTimerCurr == walkTimerMax)
		{
			std::cout << "WALK THE PLANK\n";
			walkTimerCurr--; //to keep going
		}
		else if (walkTimerCurr <= 0) walkTimerCurr = 1;// make sure it doesnt go negative
		
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