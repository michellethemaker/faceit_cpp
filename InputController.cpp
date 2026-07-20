#include "InputController.h"
#include <iostream>

	
void InputController::update(const PoseState& state)
{
	if (state.bodystate.headLeft)
	{
		std::cout << "LEFT CLICK\n";  //sample output for now
	}
}