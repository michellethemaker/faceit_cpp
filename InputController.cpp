#include "InputController.h"
#include <Windows.h> // for WORD
#include <iostream>
#include <chrono> //for keydown/keyup
#include <unordered_map> //for keydown/keyup
#include "AppConfig.h"
#define DEBUG
//#define DEBUG_NOMOUSE
#define DSTRAND //enablec sleep
#define WALKINGMODE

float screenWidth = GetSystemMetrics(SM_CXSCREEN);
float screenHeight = GetSystemMetrics(SM_CYSCREEN);

int deadzone = 15;
bool leftClick = false;
bool rightClick = false;
int walkTimerCurr = 0;
bool leftPrev = false;
bool rightPrev = false;
bool leftStep = false;
bool rightStep = false;
bool walking = false;
bool leaningLeft = false;
bool leaningRight = false;
bool crouching = false;
bool reloading = false;
bool rGrabLShoulder = false;
bool lGrabRShoulder = false;
bool lArmUp = false;
bool rArmUp = false;
bool leanLeft = false;
bool leftCoverMouth = false;
bool lElbowHang = false;
bool lElbowKick = false;
bool lConstantine = false;
bool rConstantine = false;
float avgHips;
float avgShoulders;
float currLeftIndex;
int ff = 3000;
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

void MouseDown(WORD keypress) 
{
	INPUT input = { 0 };
	input.type = INPUT_MOUSE;
	input.mi.dwFlags = keypress;

	SendInput(1, &input, sizeof(INPUT));
}


void MouseUp(WORD keypress)
{
	INPUT input = { 0 };
	input.type = INPUT_MOUSE;
	input.mi.dwFlags = keypress;

	SendInput(1, &input, sizeof(INPUT));
}

void InputController::sendDown(size_t inputCode)
{
	if (inputCode < KEY_COUNT)
	{
		KeyDown(static_cast<WORD>(inputCode));
		return;
	}

	if (inputCode == static_cast<size_t>(InputCode::MouseLeft))
	{
		MouseDown(MOUSEEVENTF_LEFTDOWN);
	}
	else if (inputCode == static_cast<size_t>(InputCode::MouseRight))
	{
		MouseDown(MOUSEEVENTF_RIGHTDOWN);
	}
	else if (inputCode == static_cast<size_t>(InputCode::MouseMiddle))
	{
		MouseDown(MOUSEEVENTF_MIDDLEDOWN);
	}
}

void InputController::sendUp(size_t inputCode)
{
	if (inputCode < KEY_COUNT)
	{
		KeyUp(static_cast<WORD>(inputCode));
		return;
	}

	if (inputCode == static_cast<size_t>(InputCode::MouseLeft))
	{
		MouseUp(MOUSEEVENTF_LEFTUP);
	}
	else if (inputCode == static_cast<size_t>(InputCode::MouseRight))
	{
		MouseUp(MOUSEEVENTF_RIGHTUP);
	}
	else if (inputCode == static_cast<size_t>(InputCode::MouseMiddle))
	{
		MouseUp(MOUSEEVENTF_MIDDLEUP);
	}
}

void InputController::setHeld(WORD keyCode, bool shouldHold)
{
	setHeldInternal( static_cast<size_t>(keyCode), shouldHold);
}
void InputController::setHeld(InputCode inputCode, bool shouldHold)
{
	setHeldInternal( static_cast<size_t>(inputCode), shouldHold );
}
void InputController::setHeldInternal( size_t inputCode, bool shouldHold)
{
	if (inputCode >= INPUT_COUNT)
	{
		return;
	}

	heldWanted[inputCode] = shouldHold;
}

void InputController::tapKey( WORD keyCode, int holdMs)
{
	tapInputInternal( static_cast<size_t>(keyCode), holdMs );
}
void InputController::tapKey( InputCode inputCode, int holdMs)
{
	tapInputInternal( static_cast<size_t>(inputCode), holdMs );
}
void InputController::tapInputInternal( size_t inputCode, int holdMs)
{
	if (inputCode >= INPUT_COUNT)
	{
		return;
	}

	const auto requestedReleaseTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(holdMs);

	if (requestedReleaseTime > tapUntil[inputCode])
	{
		tapUntil[inputCode] = requestedReleaseTime;
	}
}
void InputController::updateInput()
{
	const auto now = std::chrono::steady_clock::now();

	for (size_t key = 0; key < INPUT_COUNT; ++key)
	{
		const bool tapActive = (now < tapUntil[key]);

		// A key stays down if either a sustained action wants it,
		// or a temporary tap is still active.
		const bool shouldBeDown =
			heldWanted[key] || tapActive;

		// Send events only when the actual state needs to change.
		if (shouldBeDown && !sentDown[key])
		{
			sendDown(key);
			sentDown[key] = true;
		}
		else if (!shouldBeDown && sentDown[key])
		{
			sendUp(key);
			sentDown[key] = false;
		}
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



void InputController::update(const PSPoseState& state)
{
	//if (ff <= 800 && ff >0)
	//{
	//	if (ff == 800)
	//	{
	//		setHeld('S', true);
	//		mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, 0);
	//		std::cout << "BOOM\n";
	//	}
	//}
	//else
	//{
	//	setHeld('S', false);
	//	mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, 0);
	//}
	//ff--;
		//if(state.ps_bodystate.leftIndex - currLeftIndex != 0)
		//std::cout << state.ps_bodystate.leftIndex - currLeftIndex << "\n";
	//	if (state.ps_bodystate.leftArmExtended && (state.ps_bodystate.leftIndex - currLeftIndex > AppConfigBody::LINDEX_EXTEND_MIN))
	//	{
	//		std::cout << "LCLICK\n";
	////#ifndef DEBUG
	//		tapKey(InputCode::MouseLeft, 30);
	////#endif // !DEBUG
	//		//MoveRelative(1, 0);
	//		leftClick = true;
	//		//Sleep(10);
	//	}
	//	else if (leftClick == true)
	//	{
	//		std::cout << "UNCLICK\n";
	////#ifndef DEBUG
	//		//mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
	////#endif // !DEBUG
	//
	//		leftClick = false;
	//	}
	currLeftIndex = state.ps_bodystate.leftIndex;
	//======== LELBOW KICK =========
//	if (state.ps_bodystate.lElbowKick)
//	{
//		std::cout << "LCLICK\n";
//#ifndef DEBUG
//		mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
//#endif // !DEBUG
//		//MoveRelative(1, 0);
//		lElbowKick = true;
//		//Sleep(10);
//	}
//	else if (lElbowKick == true && !state.ps_bodystate.lElbowKick)
//	{
//#ifndef DEBUG
//		mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
//#endif // !DEBUG
//
//		lElbowKick = false;
//	}

	// =============TILT HEAD RIGHT=============
//	if (state.ps_headstate.tiltRight)
//	{
//		std::cout << "RCLICK\n";
//#ifndef DEBUG
//		mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, 0);
//#endif // !DEBUG
//		rightClick = true;
//	}
//	else if (rightClick == true)
//	{
//#ifndef DEBUG
//		mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, 0);
//#endif // !DEBUG
//	}

	//======== GRAB LSHOULDER =========
	if (state.ps_bodystate.grabLeftShoulder)
	{
//		std::cout << "LCLICK\n";
//#ifndef DEBUG
//		mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
//#endif // !DEBUG
//		//MoveRelative(1, 0);
//		leftClick = true;
//		//Sleep(10);
	}
//	else if (leftClick == true && !state.ps_bodystate.grabLeftShoulder)
//	{
//#ifndef DEBUG
//		mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
//#endif // !DEBUG
//
//		leftClick = false;
//	}

	//===================RIGHT CLICK =====================
	if (state.ps_bodystate.grabRightShoulder)
	{
//		std::cout << "RCLICK\n";
//#ifndef DEBUG
//		mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, 0);
//#endif // !DEBUG
//		rightClick = true;
	}
//	else if (rightClick == true && !state.ps_bodystate.grabRightShoulder)
//	{
//#ifndef DEBUG
//		mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, 0);
//#endif // !DEBUG
//	}
	//===================LHAND CLOSED ======================
	if (state.ps_bodystate.leftPinkyClosed) //no check to hold down, at least for this control... not very configurable if this action requires a hold function in the future...
	{
//		KeyDown(AppConfigKeybinds::KEYBIND_LHANDCLOSED);
//		KeyDown(AppConfigKeybinds::KEYBIND_LHANDCLOSED);
//		KeyDown(AppConfigKeybinds::KEYBIND_LHANDCLOSED);
//#ifdef DSTRAND
//		Sleep(10);
//#endif
		//KeyUp(AppConfigKeybinds::KEYBIND_LHANDCLOSED);
	}

	if (state.ps_bodystate.lConstantine) //no check to hold down, at least for this control... not very configurable if this action requires a hold function in the future...
	{
		//===================L & RCONSTANTINE======================
		if (state.ps_bodystate.rConstantine)
		{
			KeyDown(AppConfigKeybinds::KEYBIND_LRCONSTANTINE);
			KeyDown(AppConfigKeybinds::KEYBIND_LRCONSTANTINE);
			Sleep(5);
			KeyUp(AppConfigKeybinds::KEYBIND_LRCONSTANTINE);
		}
		else
		{
			//===================LCONSTANTINE======================
			KeyDown(AppConfigKeybinds::KEYBIND_LCONSTANTINE);
			KeyDown(AppConfigKeybinds::KEYBIND_LCONSTANTINE);
			Sleep(10);
			KeyUp(AppConfigKeybinds::KEYBIND_LCONSTANTINE);
		}
	}
	if (!state.ps_bodystate.lConstantine && state.ps_bodystate.rConstantine)
	{
		tapKey(AppConfigKeybinds::KEYBIND_RCONSTANTINE, 10);
	}

	//{
	//	KeyDown(AppConfigKeybinds::KEYBIND_RCONSTANTINE);
	//	KeyDown(AppConfigKeybinds::KEYBIND_RCONSTANTINE);
	//	Sleep(5);//nosleep! will interrupt run
	//	KeyUp(AppConfigKeybinds::KEYBIND_RCONSTANTINE);
	//}
	//===================LELBOWHANG======================
	if (state.ps_bodystate.lElbowHang) //no check to hold down, at least for this control... not very configurable if this action requires a hold function in the future...
	{
		KeyDown(AppConfigKeybinds::KEYBIND_LELBOW_HANG);
		KeyDown(AppConfigKeybinds::KEYBIND_LELBOW_HANG);
		KeyDown(AppConfigKeybinds::KEYBIND_LELBOW_HANG);
		//Sleep(10); //cannot sleep; will interrupt walk/run 
		lElbowHang = true;
	}
	if (lElbowHang == true && !state.ps_bodystate.lElbowHang) //no check to hold down, at least for this control... not very configurable if this action requires a hold function in the future...
	{
		KeyUp(AppConfigKeybinds::KEYBIND_LELBOW_HANG);
		lElbowHang = false;
	}
	//===================MOUSE MOVEMENT (POV CONTROL)=====================

	if (state.ps_bodystate.hasRightShoulder && !state.ps_bodystate.grabRightShoulder) // ENABLE IF MOUSE CONTROL ENABLED BY CLOSED FIST
	{
		//std::cout << state.ps_bodystate.rightWristLR_val << "||"<<state.ps_bodystate.rightWristUD_val<< "\n";
		//MoveRelative((state.ps_bodystate.rightWristLR_val), (state.ps_bodystate.rightWristUD_val));  // LEFT/RIGHT--UP/DOWN
		
#ifndef DEBUG_NOMOUSE:

		MoveRelative((state.ps_headstate.headXleft_val + state.ps_headstate.headXright_val), 
					 (state.ps_headstate.headYup_val + state.ps_headstate.headYdown_val));

#endif
//		
//		if (state.ps_headstate.headXleft) 
//		{
//#ifndef DEBUG_NOMOUSE:
//		MoveRelative( (state.ps_headstate.headXleft_val), 0);
//#endif
//			//std::cout << "<<<<<<<<<<<\n";
//		}
//		if (state.ps_headstate.headXright)
//		{
//#ifndef DEBUG_NOMOUSE:
//		MoveRelative((state.ps_headstate.headXright_val), 0);
//#endif
//			//std::cout << "           >>>>>>>>>>>>\n";
//		}
//		if (state.ps_headstate.headYup)
//		{
//#ifndef DEBUG_NOMOUSE:
//		MoveRelative(0, (state.ps_headstate.headYup_val));
//#endif
//			//std::cout << "\n^^^^^^^^UP\n";
//		}
//		if (state.ps_headstate.headYdown)
//		{
//#ifndef DEBUG_NOMOUSE:
//			MoveRelative(0, (state.ps_headstate.headYdown_val));
//#endif
//			//std::cout << "\n______DOWN\n";
//		}
	}

	//===================LARM UP =====================
	if (state.ps_bodystate.leftArmUp)
	{
#ifndef DEBUG
		KeyDown(AppConfigKeybinds::KEYBIND_LARMUP);
		KeyDown(AppConfigKeybinds::KEYBIND_LARMUP);
#ifdef DSTRAND
		Sleep(5);
#endif
		KeyUp(AppConfigKeybinds::KEYBIND_LARMUP);
#endif // !DEBUG
		std::cout << "LARMUP \n";
		lArmUp = true;
	}
	else if (lArmUp == true && !state.ps_bodystate.leftArmUp)
	{
//#ifndef DEBUG
		std::cout << "UNLARMUP \n";
//#endif // !DEBUG
		lArmUp = false;
	}
	//===================RARM UP =====================
	if (state.ps_bodystate.rightArmUp && rArmUp == false)
	{
#ifndef DEBUG
		KeyDown(AppConfigKeybinds::KEYBIND_RARMUP);
		KeyDown(AppConfigKeybinds::KEYBIND_RARMUP);
		KeyDown(AppConfigKeybinds::KEYBIND_RARMUP);
#ifdef DSTRAND
		Sleep(10);
#endif
		KeyUp(AppConfigKeybinds::KEYBIND_RARMUP);
#endif // !DEBUG
		std::cout << "RARMUP \n";
		rArmUp = true;
	}
	else if (rArmUp == true && !state.ps_bodystate.rightArmUp)
	{
		//#ifndef DEBUG
		std::cout << "UNRARMUP \n";
		//#endif // !DEBUG
		rArmUp = false;
	}

	//===================LEAN LEFT=====================
//	if (state.ps_bodystate.leaningLeft && leanLeft == false)
//	{
//#ifndef DEBUG
//		KeyDown(AppConfigKeybinds::KEYBIND_LEANLEFT);
//		KeyDown(AppConfigKeybinds::KEYBIND_LEANLEFT);
//		KeyDown(AppConfigKeybinds::KEYBIND_LEANLEFT);
//#ifdef DSTRAND
//		Sleep(10);
//#endif
//		KeyUp(AppConfigKeybinds::KEYBIND_LEANLEFT);
//#endif // !DEBUG
//		std::cout << "LEANLEFT \n";
//		leanLeft = true;
//	}
//	else if (leanLeft == true && !state.ps_bodystate.leaningLeft)
//	{
//		//#ifndef DEBUG
//		std::cout << "UNLEANLEFT \n";
//		//#endif // !DEBUG
//		leanLeft = false;
//	}
	//===================WALKING=====================

	//if (state.ps_bodystate.leftLegUp || state.ps_bodystate.rightLegUp)
	//{
	//	walkTimerCurr = AppConfigBody::WALKTIMER_MAX;
	//	std::cout << state.ps_bodystate.leftLegUp << " || " << state.ps_bodystate.rightLegUp << "\n";
	//}

	if (state.ps_bodystate.rightLegUp)
	{
		rightStep = true;
		leftStep = false;
	}
	if (rightStep)
	{
		if (state.ps_bodystate.leftLegUp)
		{
			leftStep = true;
			walkTimerCurr = AppConfigBody::WALKTIMER_MAX;
			rightStep = false;
			
		}
	}

	//if (state.ps_bodystate.leftLegUp ^ leftPrev && state.ps_bodystate.leftLegUp == false) // falling edge
	//{
	//		leftStep = true; //we did a left steppy
	//		if (rightStep && walkTimerCurr >= 0)
	//		{
	//			leftStep = false;
	//			rightStep = false;
	//			std::cout << "LEFT\n";
	//			walkTimerCurr = AppConfigBody::WALKTIMER_MAX;
	//		}
	//}
	//if (state.ps_bodystate.rightLegUp ^ rightPrev && state.ps_bodystate.rightLegUp == false) // falling edge
	//{
	//		rightStep = true; //we did a right steppy
	//		if (leftStep && walkTimerCurr >= 0)
	//		{
	//			leftStep = false;
	//			rightStep = false;
	//			std::cout << "RIGHT\n";
	//			walkTimerCurr = AppConfigBody::WALKTIMER_MAX;
	//		}
	//}

	if (walkTimerCurr > 0)
	{
		//std::cout << walkTimerCurr<<"STEPPY\n";
		if (walkTimerCurr > 5 && walking == false)
		{
			walking = true;
#ifdef WALKINGMODE
			
			setHeld('W', walking);
			tapKey(AppConfigKeybinds::KEYBIND_RCONSTANTINE, 30);
			//KeyDown('W'); //trust, it's walking. doesnt look that way in notepad but it's walking. needed to not overload windows.
#endif
			std::cout << "WALKIN\n";
		}
		walkTimerCurr--;
	}
	else
	{
		walking = false;
#ifdef WALKINGMODE
		setHeld('W', walking);
		//KeyUp('W');
#endif
		walkTimerCurr = 0; //reset walktimer to 0, stop walking
		//std::cout << "NOTWALKIN\n";
	}
	//std::cout << leftStep << "|" << leftPrev << "\n" << rightStep << "|" << rightPrev << "||" << walkTimerCurr << "\n====\n";
	leftPrev = state.ps_bodystate.leftLegUp; //update prev bools
	rightPrev = state.ps_bodystate.rightLegUp;
	

	//if (state.ps_bodystate.currAvgHips - avgHips != 0 && avgShoulders - state.ps_bodystate.currAvgShoulders != 0)
	//{
	//std::cout << state.ps_bodystate.currAvgHips - avgHips << "||" << avgShoulders - state.ps_bodystate.currAvgShoulders << "\n";
	//}
	
	if (state.ps_bodystate.currAvgHips - avgHips > AppConfigBody::JUMPHIPS_MIN &&
		state.ps_bodystate.currAvgHips - avgHips < AppConfigBody::JUMPHIPS_MAX &&
		state.ps_bodystate.currAvgShoulders - avgShoulders > AppConfigBody::JUMPSHOULDER_MAX
		)
	{
		tapKey(AppConfigKeybinds::KEYBIND_JUMP, 30);
		if (walkTimerCurr > 0)
		{
			walkTimerCurr += 20; //just to keep the momentum u know
			std::cout << "KEEP UR ASS GOING GIRL\n";
		}
			
//		KeyDown(AppConfigKeybinds::KEYBIND_JUMP);
//		KeyDown(AppConfigKeybinds::KEYBIND_JUMP);
//		KeyDown(AppConfigKeybinds::KEYBIND_JUMP);
//#ifdef DSTRAND
//		Sleep(5);
//#endif
//		KeyUp(AppConfigKeybinds::KEYBIND_JUMP);
		std::cout << "JUMP\n";
		//std::cout << avgHips - state.ps_bodystate.currAvgHips << "{}"<< avgShoulders - state.ps_bodystate.currAvgShoulders << "\n";
	}
		avgHips = state.ps_bodystate.currAvgHips;
		avgShoulders = state.ps_bodystate.currAvgShoulders;
		
	//===================CROUCHING=====================
	//std::cout<< state.ps_bodystate.crouching <<"|"<< crouching << "\n";
	if (state.ps_bodystate.crouching && crouching == false) 
	{
		KeyDown(AppConfigKeybinds::KEYBIND_CROUCH);
		KeyDown(AppConfigKeybinds::KEYBIND_CROUCH);
		KeyDown(AppConfigKeybinds::KEYBIND_CROUCH);
#ifdef DSTRAND
		Sleep(10);
#endif
		KeyUp(AppConfigKeybinds::KEYBIND_CROUCH);
		crouching = true;
		std::cout << "CROUCHING\n";
	}
	if (!state.ps_bodystate.crouching && crouching == true)
	{
		KeyDown(AppConfigKeybinds::KEYBIND_CROUCH);
		KeyDown(AppConfigKeybinds::KEYBIND_CROUCH);
		KeyDown(AppConfigKeybinds::KEYBIND_CROUCH);
#ifdef DSTRAND
		Sleep(10);
#endif
		KeyUp(AppConfigKeybinds::KEYBIND_CROUCH);
		std::cout << "UNCROUCHING\n";
		crouching = false;
	}


	//===================HAND MOTIONS=====================

	// =====RELOAD=====
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
	
	// =====RIGHT GRAB LSHOULDER=====
	if (state.ps_bodystate.rightOverLeftShoulder) //hold down
	{
		KeyDown(AppConfigKeybinds::KEYBIND_RGRABLSHOULDER);
		KeyDown(AppConfigKeybinds::KEYBIND_RGRABLSHOULDER);
		KeyDown(AppConfigKeybinds::KEYBIND_RGRABLSHOULDER);
		rGrabLShoulder = true;
		std::cout << "RGRABLSHOULDER E\n";
	}
	if (!state.ps_bodystate.rightOverLeftShoulder && rGrabLShoulder == true)
	{
		KeyUp(AppConfigKeybinds::KEYBIND_RGRABLSHOULDER);
		rGrabLShoulder = false;
		std::cout << "END RGRABLSHOULDER E\n";
	}

	// =====LEFT GRAB RSHOULDER=====
	if (state.ps_bodystate.leftOverRightShoulder )
	{
		KeyDown(AppConfigKeybinds::KEYBIND_LGRABRSHOULDER);
		KeyDown(AppConfigKeybinds::KEYBIND_LGRABRSHOULDER);
		KeyDown(AppConfigKeybinds::KEYBIND_LGRABRSHOULDER);
		lGrabRShoulder = true;
		std::cout << "LGRABRSHOULDER\n";
	}
	if (!state.ps_bodystate.leftOverRightShoulder && lGrabRShoulder == true)
	{
		KeyUp(AppConfigKeybinds::KEYBIND_LGRABRSHOULDER);
		lGrabRShoulder = false;
		std::cout << "END LGRABRSHOULDER\n";
	}

	// =====LEFT COVER MOUTH=====
	if (state.ps_bodystate.leftCoverMouth && leftCoverMouth == false)
	{
		KeyDown(AppConfigKeybinds::KEYBIND_HOLDBREATH);
		KeyDown(AppConfigKeybinds::KEYBIND_HOLDBREATH);
		KeyDown(AppConfigKeybinds::KEYBIND_HOLDBREATH);
#ifdef DSTRAND
		Sleep(10);
#endif
		KeyUp(AppConfigKeybinds::KEYBIND_HOLDBREATH);
		leftCoverMouth = true;
		std::cout << "HOLD BREATH\n";
	}
	if (!state.ps_bodystate.leftCoverMouth && leftCoverMouth == true)
	{
		KeyDown(AppConfigKeybinds::KEYBIND_HOLDBREATH);
		KeyDown(AppConfigKeybinds::KEYBIND_HOLDBREATH);
		KeyDown(AppConfigKeybinds::KEYBIND_HOLDBREATH);
#ifdef DSTRAND
		Sleep(10);
#endif
		KeyUp(AppConfigKeybinds::KEYBIND_HOLDBREATH);
		leftCoverMouth = false;
		std::cout << "END HOLD BREATH\n";
	}


	updateInput(); //update all keystrokes
}
