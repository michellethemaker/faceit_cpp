#pragma once
#include "Keypoint.h"
#include "PoseState.h"

class InputController
{
public:
    void update(const PSPoseState & state);
};