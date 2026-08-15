#include "InputController.h"
#include <Windows.h>
#include <iostream>

float screenWidth = GetSystemMetrics(SM_CXSCREEN);
float screenHeight = GetSystemMetrics(SM_CYSCREEN);
float prevX = 0;
float prevY = 0;
float currX = 0;
float currY = 0;
int deadzone_S = 15;
bool leftClick = false;
//int deadzone_L = 15;
void InputController::update(const PSPoseState& state)
{

		//std::cout << "LEFT ARM: " << state.ps_bodystate.leftArmUp << "\n";
		if (state.ps_bodystate.toot)
		{
			//std::cout << "LCLICK\n";
			mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
			leftClick = true;
			//Sleep(500);
		}
		else if (leftClick == true && !state.ps_bodystate.toot )
		{
			mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
			leftClick = false;
		}

		if (state.ps_headstate.headXcoord) // TODO: check if this actually checks presence of head. what returns when no head present?
		{

			currX = state.ps_headstate.headXcoord * screenWidth;
			currY = state.ps_headstate.headYcoord * screenHeight;
			if (std::abs(currX - prevX) < deadzone_S)
			{
				currX = prevX;
			}
			else
				prevX = currX;
			
			if (std::abs(currY - prevY) < deadzone_S)
			{
				currY = prevY;
			}
			else
				prevY = currY;


			SetCursorPos(currX, currY);
			//std::cout << "====\nmousepos: \n" << currX << "||" << currY << "\n" << prevX << "||" << prevY << "\n=====\n";
		}
	
}