#include "CoffeeEngine/Input/InputBinding.h"

#include "CoffeeEngine/Core/Input.h"

namespace Coffee
{

 float InputBinding::GetValue(ControllerCode controller)
{
    if (IsAnalog && Axis != Axis::Invalid)
    {
        return Input::GetAxisRaw(Axis);
    }

    float value = 0.0f;
    if (Input::IsKeyPressed(KeyPos) || Input::GetButtonRaw(ButtonPos))
        value += 1.0f;
    if (Input::IsKeyPressed(KeyNeg) || Input::GetButtonRaw(ButtonNeg))
        value -= 1.0f;

    return value;
}

/**
 * Returns the input value as a float
 * @return A value between -1 and 1
 */
template <>
float InputBinding::Get<float>()
{
    if (IsAnalog && Axis != Axis::Invalid)
    {
        return Input::GetAxisRaw(Axis);
    }
    else // Digital input
    {
        return (Input::IsKeyPressed(KeyPos) || Input::GetButtonRaw(ButtonPos)) - (Input::IsKeyPressed(KeyNeg) || Input::GetButtonRaw(ButtonNeg));
    }
}

/**
 * Reads the action as a boolean. Any axis movement returns true, and negative buttons and keys are treated as alternate
 * binds
 * @return Whether the binding is active or not
 */
template <>
bool InputBinding::Get<bool>()
{
     bool value = Input::GetButtonRaw(ButtonPos);
     value |= Input::GetButtonRaw(ButtonNeg);
     value |= Input::IsKeyPressed(KeyPos);
     value |= Input::IsKeyPressed(KeyNeg);
     value |= Input::GetAxisRaw(Axis) != 0.0f;
     return value;
}

}
