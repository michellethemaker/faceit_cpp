#include "InputController.h"
#include <Windows.h>
#include <iostream>
#include "AppConfig.h"

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


InputController::InputController() //create onnx runtime env, set optimisation settings here
	: inputControllerenv(ORT_LOGGING_LEVEL_WARNING, "InputController")
{
	sessionOptions.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);
}
InputController::~InputController() // destructor
{
	stop(); // make sure worker thread joined! (stop() defined above)
}

void InputController::start() //start worker thread
{
	if (running) return;
	running = true;
	workerThread = std::thread(&InputController::workerLoop, this);
	std::cout << "starting inputcontroller thread\n";
}

void InputController::stop() //stop worker thread
{
	if (!running) return;
	running = false;

	// this is to wake up worker if it's sleeping/waiting for a frame
	{
		std::lock_guard<std::mutex> lock(stateMutex);
		hasPendingState = true;
	}

	if (workerThread.joinable())
	{
		workerThread.join();
	}
}
void InputController::pushState(const PSPoseState& state)// main thread writes to pendingFrame
{
	{
		std::lock_guard<std::mutex> lock(stateMutex);
		pendingState = state;
		hasPendingState = true;
	}
}
void InputController::workerLoop()
{
	while (running)
	{
		update(pendingState);
	}
}

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
	//if (state.ps_bodystate.grabLeftShoulder)
	//{
	//	std::cout << "LCLICK\n";
	//	//mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
	//	//MoveRelative(1, 0);
	//	leftClick = true;
	//	//Sleep(500);
	//}
	//else if (leftClick == true && !state.ps_bodystate.grabLeftShoulder)
	//{
	//	//mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
	//	leftClick = false;
	//}

	//if (state.ps_bodystate.grabRightShoulder)
	//{
	//	std::cout << "RCLICK\n";
	//	//mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, 0);
	//	rightClick = true;
	//}
	//else if (rightClick == true && !state.ps_bodystate.grabRightShoulder)
	//{
	//	//mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, 0);
	//}

	
	//===================HEAD MOVEMENT (POV CONTROL)=====================
	//scaled acc to how much u turn ur head. need to handle the scale, make sure up/down scale = left/right scale.
	//if (state.ps_headstate.headYup)
	//{
	//	//std::cout << state.ps_headstate.headYup_val<<"\n";
	//	MoveRelative(0, -1 * (state.ps_headstate.headYup_val));//TODO: fix these, normalise somehow.
	//}
	//else if (state.ps_headstate.headYdown)
	//{
	//	//std::cout << state.ps_headstate.headYdown_val << "\n";
	//	MoveRelative(0,  (-1) * (state.ps_headstate.headYdown_val));//TODO: fix these, normalise somehow.
	//}
	//if (state.ps_headstate.headXleft)
	//{
	//	//std::cout << state.ps_headstate.headXleft_val << "\n";
	//	MoveRelative(-1 * (state.ps_headstate.headXleft_val), 0);
	//}
	//else if (state.ps_headstate.headXright)
	//{
	//	//std::cout << "             " << state.ps_headstate.headXright_val << "\n";
	//	MoveRelative( (-1) * (state.ps_headstate.headXright_val), 0);
	//}
	//
	
	if (state.ps_bodystate.hasRightShoulder) // UP/DOWN
	{
		//std::cout << state.ps_headstate.headYup_val<<"\n";
		MoveRelative(0, 0.05f*(state.ps_bodystate.rightWristYcoord - state.ps_bodystate.rightShoulderYcoord));//TODO: fix these, normalise somehow.
	}

	if (state.ps_bodystate.hasRightShoulder) // LEFT/RIGHT
	{
		std::cout <<  state.ps_bodystate.rightWristXcoord - state.ps_bodystate.rightShoulderXcoord << "\n";
		MoveRelative(0.05f * (state.ps_bodystate.rightWristXcoord - state.ps_bodystate.rightShoulderXcoord ), 0);
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
		KeyDown(AppConfigKeybinds::KEYBIND_CROUCH);
		KeyUp(AppConfigKeybinds::KEYBIND_CROUCH);
		crouching = true;
		std::cout << "CROUCHING\n";
	}
	if (!state.ps_bodystate.crouching && crouching == true)
	{
		KeyDown(AppConfigKeybinds::KEYBIND_CROUCH);
		KeyUp(AppConfigKeybinds::KEYBIND_CROUCH);
		std::cout << "UNCROUCHING\n";
		crouching = false;
	}


	//===================HAND MOTIONS=====================
	if (state.ps_bodystate.leftOverRightHand && reloading == false)
	{
		KeyDown(AppConfigKeybinds::KEYBIND_RELOAD);
		KeyUp(AppConfigKeybinds::KEYBIND_RELOAD);
		reloading = true;
		std::cout << "RELOADING\n";
	}
	if (!state.ps_bodystate.leftOverRightHand && reloading == true)
	{
		KeyDown(AppConfigKeybinds::KEYBIND_RELOAD);
		KeyUp(AppConfigKeybinds::KEYBIND_RELOAD);
		reloading = false;
		std::cout << "END RELOAD\n";
	}
	
	if (state.ps_bodystate.rightOverLeftShoulder && etrigger == false)
	{
		KeyDown(AppConfigKeybinds::KEYBIND_ACTION1);
		KeyUp(AppConfigKeybinds::KEYBIND_ACTION1);
		etrigger = true;
		std::cout << "E\n";
	}
	if (!state.ps_bodystate.rightOverLeftShoulder && etrigger == true)
	{
		KeyDown(AppConfigKeybinds::KEYBIND_ACTION1);
		KeyUp(AppConfigKeybinds::KEYBIND_ACTION1);
		etrigger = false;
		std::cout << "END E\n";
	}

}
