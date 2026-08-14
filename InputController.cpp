#include "InputController.h"
#include <Windows.h>
#include <iostream>

	
void InputController::update(const PSPoseState& state)
{
	//if (state.bodystate.headLeft)
	//{
	//	std::cout << "LEFT CLICK\n";  //sample output for now
	//}
	//SetCursorPos(200,200);
	//if (g_bodyState.isVisible)
	//{
		std::cout << "LEFT ARM: " << state.ps_bodystate.leftArmUp << "\n";
		if (state.ps_bodystate.leftArmUp)
		{
			std::cout << "LCLICK\n";
			mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
			mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
			//Sleep(500);
		}
	//}
	
}