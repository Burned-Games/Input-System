#include "CoffeeEngine/Input/InputBinding.h"


 float InputBinding::GetValue(ControllerCode controller)
{
    if (IsAnalog && Axis != Axis::Invalid)
    {
        return Input::GetAxis(Axis, controller);
    }

    float value = 0.0f;
    if (Input::IsKeyPressed(KeyPos) || Input::GetButtonRaw(ButtonPos, controller))
        value += 1.0f;
    if (Input::IsKeyPressed(KeyNeg) || Input::GetButtonRaw(ButtonNeg, controller))
        value -= 1.0f;

    return value;
}