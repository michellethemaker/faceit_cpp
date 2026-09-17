#include "InputController.h"
#include <Windows.h>
#include <iostream>
#include "AppConfig.h"
//#define DEBUG
#define DEBUG_NOMOUSE

float screenWidth = GetSystemMetrics(SM_CXSCREEN);
float screenHeight = GetSystemMetrics(SM_CYSCREEN);
//float prevX = 0;
//float prevY = 0;
//float currX = 0;
//float currY = 0;
int deadzone = 15;
bool leftClick = false;
bool rightClick = false;
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
bool scan = false;
bool leftCoverMouth = false;
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
	using clock = std::chrono::steady_clock;
	constexpr auto interval = std::chrono::microseconds(8333); // ~120 Hz
	auto nextTick = clock::now();
	while (running)
	{
		update(pendingState);
		nextTick += interval;
		std::this_thread::sleep_until(nextTick);
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
#ifndef DEBUG
	INPUT input{};
	input.type = INPUT_KEYBOARD;
	input.ki.wVk = keypress;
	input.ki.wScan = 0;
	input.ki.dwFlags = 0;

	SendInput(1, &input, sizeof(INPUT));
#endif // !DEBUG

}

void KeyUp(WORD keypress)
{
#ifndef DEBUG
	INPUT input{};
	input.type = INPUT_KEYBOARD;
	input.ki.wVk = keypress;
	input.ki.wScan = 0;
	input.ki.dwFlags = KEYEVENTF_KEYUP;

	SendInput(1, &input, sizeof(INPUT));
#endif // !DEBUG

}

void InputController::update(const PSPoseState& state)
{
	//===================LEFT CLICK =====================
	if (state.ps_bodystate.grabLeftShoulder)
	{
		std::cout << "LCLICK\n";
#ifndef DEBUG
		mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
#endif // !DEBUG
		//MoveRelative(1, 0);
		leftClick = true;
		//Sleep(500);
	}
	else if (leftClick == true && !state.ps_bodystate.grabLeftShoulder)
	{
#ifndef DEBUG
		mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
#endif // !DEBUG

		leftClick = false;
	}

	//===================RIGHT CLICK =====================
	if (state.ps_bodystate.grabRightShoulder)
	{
		std::cout << "RCLICK\n";
#ifndef DEBUG
		mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, 0);
#endif // !DEBUG
		rightClick = true;
	}
	else if (rightClick == true && !state.ps_bodystate.grabRightShoulder)
	{
#ifndef DEBUG
		mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, 0);
#endif // !DEBUG
	}
	
	//===================MOUSE MOVEMENT (POV CONTROL)=====================

	//if (state.ps_bodystate.hasRightShoulder && state.ps_bodystate.rightPinkyClosed) // ENABLE IF MOUSE CONTROL ENABLED BY CLOSED FIST
	{
		//std::cout << state.ps_bodystate.rightWristLR_val << "||"<<state.ps_bodystate.rightWristUD_val<< "\n";
		//MoveRelative((state.ps_bodystate.rightWristLR_val), (state.ps_bodystate.rightWristUD_val));  // LEFT/RIGHT--UP/DOWN
		if (state.ps_headstate.headXleft) 
		{
#ifndef DEBUG_NOMOUSE:
		MoveRelative( (state.ps_headstate.headXleft_val), 0);
#endif
			//std::cout << "<<<<<<<<<<<\n";
		}
		if (state.ps_headstate.headXright)
		{
#ifndef DEBUG_NOMOUSE:
		MoveRelative((state.ps_headstate.headXright_val), 0);
#endif
			//std::cout << "           >>>>>>>>>>>>\n";
		}
		if (state.ps_headstate.headYup)
		{
#ifndef DEBUG_NOMOUSE:
		MoveRelative(0, (state.ps_headstate.headYup_val));
#endif
			//std::cout << "\n^^^^^^^^UP\n";
		}
		if (state.ps_headstate.headYdown)
		{
#ifndef DEBUG_NOMOUSE:
			MoveRelative(0, (state.ps_headstate.headYdown_val));
#endif
			//std::cout << "\n______DOWN\n";
		}
	}

	//===================SCAN =====================
	if (state.ps_bodystate.leftArmUp && scan == false)
	{
#ifndef DEBUG
		KeyDown(AppConfigKeybinds::KEYBIND_SCAN);
		KeyDown(AppConfigKeybinds::KEYBIND_SCAN);
		KeyDown(AppConfigKeybinds::KEYBIND_SCAN);
		Sleep(100);
		KeyUp(AppConfigKeybinds::KEYBIND_SCAN);
		std::cout << "SCAN \n";
#endif // !DEBUG
		scan = true;
	}
	else if (scan == true && !state.ps_bodystate.leftArmUp)
	{
//#ifndef DEBUG
		std::cout << "UNSCAN \n";
//#endif // !DEBUG
		scan = false;
	}
	//===================WALKING=====================

	if (state.ps_bodystate.leftLegUp ^ leftPrev && state.ps_bodystate.leftLegUp == false) // falling edge
	{
			leftStep = true; //we did a left steppy
			if (rightStep && walkTimerCurr >= 0)
			{
				leftStep = false;
				rightStep = false;
				walkTimerCurr = AppConfigBody::WALKTIMER_MAX;
			}
	}
	if (state.ps_bodystate.rightLegUp ^ rightPrev && state.ps_bodystate.rightLegUp == false) // falling edge
	{
			rightStep = true; //we did a right steppy
			if (leftStep && walkTimerCurr >= 0)
			{
				leftStep = false;
				rightStep = false;
				walkTimerCurr = AppConfigBody::WALKTIMER_MAX;
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
		//std::cout << "NOTWALKIN\n";
	}
	//std::cout << leftStep << "|" << rightStep << "||" << walkTimerCurr << "\n";
	leftPrev = state.ps_bodystate.leftLegUp; //update prev bools
	rightPrev = state.ps_bodystate.rightLegUp;
	
	
	if (avgHips -state.ps_bodystate.currAvgHips > 0.13f && avgHips - state.ps_bodystate.currAvgHips < 0.6f &&
		avgShoulders - state.ps_bodystate.currAvgShoulders > 0.19f) 
	{
		KeyDown(AppConfigKeybinds::KEYBIND_JUMP);
		KeyUp(AppConfigKeybinds::KEYBIND_JUMP);
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
		KeyDown(AppConfigKeybinds::KEYBIND_CROUCH);
		KeyDown(AppConfigKeybinds::KEYBIND_CROUCH);
		Sleep(100);
		KeyUp(AppConfigKeybinds::KEYBIND_CROUCH);
		crouching = true;
		std::cout << "CROUCHING\n";
	}
	if (!state.ps_bodystate.crouching && crouching == true)
	{
		KeyDown(AppConfigKeybinds::KEYBIND_CROUCH);
		KeyDown(AppConfigKeybinds::KEYBIND_CROUCH);
		KeyDown(AppConfigKeybinds::KEYBIND_CROUCH);
		Sleep(100);
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
		KeyDown(AppConfigKeybinds::KEYBIND_ACTION_E);
		KeyUp(AppConfigKeybinds::KEYBIND_ACTION_E);
		etrigger = true;
		std::cout << "E\n";
	}
	if (!state.ps_bodystate.rightOverLeftShoulder && etrigger == true)
	{
		KeyDown(AppConfigKeybinds::KEYBIND_ACTION_E);
		KeyUp(AppConfigKeybinds::KEYBIND_ACTION_E);
		etrigger = false;
		std::cout << "END E\n";
	}

	// =====LEFT COVER MOUTH=====
	if (state.ps_bodystate.leftCoverMouth && leftCoverMouth == false)
	{
		KeyDown(AppConfigKeybinds::KEYBIND_HOLDBREATH);
		KeyDown(AppConfigKeybinds::KEYBIND_HOLDBREATH);
		KeyDown(AppConfigKeybinds::KEYBIND_HOLDBREATH);
		Sleep(100);
		KeyUp(AppConfigKeybinds::KEYBIND_HOLDBREATH);
		leftCoverMouth = true;
		std::cout << "HOLD BREATH\n";
	}
	if (!state.ps_bodystate.leftCoverMouth && leftCoverMouth == true)
	{
		KeyDown(AppConfigKeybinds::KEYBIND_HOLDBREATH);
		KeyDown(AppConfigKeybinds::KEYBIND_HOLDBREATH);
		KeyDown(AppConfigKeybinds::KEYBIND_HOLDBREATH);
		Sleep(100);
		KeyUp(AppConfigKeybinds::KEYBIND_HOLDBREATH);
		leftCoverMouth = false;
		std::cout << "END HOLD BREATH\n";
	}

}
