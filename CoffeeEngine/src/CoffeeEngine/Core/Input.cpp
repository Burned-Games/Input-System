#include "CoffeeEngine/Core/Input.h"

#include "CoffeeEngine/Events/ControllerEvent.h"
#include "CoffeeEngine/Events/Event.h"
#include "CoffeeEngine/Events/KeyEvent.h"
#include "CoffeeEngine/Events/MouseEvent.h"
#include "SDL3/SDL_keyboard.h"
#include "SDL3/SDL_mouse.h"

#include <SDL3/SDL_init.h>
#include <glm/gtc/constants.hpp>

namespace Coffee {

    std::vector<Ref<Gamepad>> Input::m_gamepads;
    std::unordered_map<ControllerCode, std::unordered_map<ButtonCode, bool>> Input::m_buttonStates;
    std::unordered_map<ControllerCode, std::unordered_map<AxisCode, float>> Input::m_axisStates;
    std::vector<InputBinding> Input::m_bindings = std::vector<InputBinding>(static_cast<int>(InputAction::ActionCount));


    void Input::Init()
    {
        SDL_InitSubSystem(SDL_INIT_GAMEPAD);

        COFFEE_CORE_INFO("Input initialized");
    }


     bool Input::IsKeyPressed(const KeyCode key)
    {
        const bool* state = SDL_GetKeyboardState(nullptr);

        return state[key];
    }

    bool Input::IsMouseButtonPressed(const MouseCode button)
    {
        return SDL_GetMouseState(nullptr, nullptr) & SDL_BUTTON_MASK(button);
    }

    glm::vec2 Input::GetMousePosition()
    {
        float x, y;
        SDL_GetMouseState(&x, &y);

        return {x, y};
    }

    float Input::GetMouseX()
    {
        return GetMousePosition().x;
    }

    float Input::GetMouseY()
    {
        return GetMousePosition().y;
    }

    bool Input::GetButton(const ButtonCode button, const ControllerCode controller)
    {
        if (controller == 0) return false;

        auto controllerIt = m_buttonStates.find(controller);
        if (controllerIt != m_buttonStates.end())
        {
            const auto& buttonMap = controllerIt->second;
            auto buttonIt = buttonMap.find(button);
            return (buttonIt != buttonMap.end()) ? buttonIt->second : false;
        }
        return false;
    }

    float Input::GetAxis(const AxisCode axis, const ControllerCode controller)
    {
        if (controller == 0) return 0.0f;

        auto controllerIt = m_axisStates.find(controller);
        if (controllerIt != m_axisStates.end())
        {
            const auto& axisMap = controllerIt->second;
            auto axisIt = axisMap.find(axis);
            return (axisIt != axisMap.end()) ? axisIt->second : 0.0f;
        }
        return 0.0f;
    }

    void Input::OnAddController(const ControllerAddEvent* cEvent)
    {
        m_gamepads.emplace_back(new Gamepad(cEvent->Controller));
    }


    void Input::OnRemoveController(ControllerRemoveEvent* cEvent)
    {
        // Remove controller by SDL_Gamepad ID
        auto pred = [&cEvent](const Ref<Gamepad>& gamepad) {
            return gamepad->getId() == cEvent->Controller;
        };
        erase_if(m_gamepads, pred);
    }
    void Input::OnButtonPressed(ButtonPressEvent& e) {
        COFFEE_INFO("Gamepad {0} Button {1} Pressed", e.Controller, e.Button);
        m_buttonStates[e.Controller][e.Button] = true;
    }

    void Input::OnButtonReleased(ButtonReleaseEvent& e) {
        COFFEE_INFO("Gamepad {0} Button {1} Released", e.Controller, e.Button);
        m_buttonStates[e.Controller][e.Button] = false;
    }

    void Input::OnAxisMoved(AxisMoveEvent& e) {

        constexpr float DEADZONE = 0.15f;
        float normalizedValue = e.Value / 32767.0f;

        if (std::abs(normalizedValue) < DEADZONE)
        {
            normalizedValue = 0.0f;
        }

        COFFEE_INFO("Gamepad {0} Axis {1} value {2}", e.Controller, e.Axis, normalizedValue);
        m_axisStates[e.Controller][e.Axis] = normalizedValue;
    }
    void Input::OnKeyPressed(const KeyPressedEvent& kEvent) {
        COFFEE_INFO("Key {0} Pressed", kEvent.GetKeyCode());
    }

    void Input::OnKeyReleased(const KeyReleasedEvent& kEvent) {
        COFFEE_INFO("Key {0} Released", kEvent.GetKeyCode());
    }

    void Input::OnMouseButtonPressed(const MouseButtonPressedEvent& mEvent) {
        COFFEE_INFO("Mouse Button {0} Pressed", mEvent.GetMouseButton());
    }

    void Input::OnMouseButtonReleased(const MouseButtonReleasedEvent& mEvent) {
        COFFEE_INFO("Mouse Button {0} Released", mEvent.GetMouseButton());
    }

    void Input::OnMouseMoved(const MouseMovedEvent& mEvent) {
        COFFEE_INFO("Mouse Moved: {0}, {1}", mEvent.GetX(), mEvent.GetY());
    }

    void Input::OnEvent(Event& e)
	{
	    if (e.IsInCategory(EventCategoryInput))
	    {
            switch (e.GetEventType())
            {
            	using enum EventType;
                case ControllerConnected:
                {
                    ControllerAddEvent* cEvent = static_cast<ControllerAddEvent*>(&e);
                    if (cEvent)
                        OnAddController(cEvent);
                    break;
                }
                case ControllerDisconnected:
                {
                    ControllerRemoveEvent* cEvent = static_cast<ControllerRemoveEvent*>(&e);
                    if (cEvent)
                        OnRemoveController(cEvent);
                    break;
                }
                case ButtonPressed:
                {
                    ButtonPressEvent* bEvent = static_cast<ButtonPressEvent*>(&e);
                    if (bEvent)
                        OnButtonPressed(*bEvent);
                    break;
                }
                case ButtonReleased:
                {
                    ButtonReleaseEvent* bEvent = static_cast<ButtonReleaseEvent*>(&e);
                    if (bEvent)
                        OnButtonReleased(*bEvent);
                    break;
                }
                case AxisMoved:
                {
                    AxisMoveEvent* aEvent = static_cast<AxisMoveEvent*>(&e);
                    if (aEvent)
                        OnAxisMoved(*aEvent);
                    break;
                }
                case KeyPressed:
                {
                    KeyPressedEvent* kEvent = static_cast<KeyPressedEvent*>(&e);
                    if (kEvent)
                        OnKeyPressed(*kEvent);
                    break;
                }
                case KeyReleased:
                {
                    KeyReleasedEvent* kEvent = static_cast<KeyReleasedEvent*>(&e);
                    if (kEvent)
                        OnKeyReleased(*kEvent);
                    break;
                }
                case MouseButtonPressed:
                {
                    MouseButtonPressedEvent* mEvent = static_cast<MouseButtonPressedEvent*>(&e);
                    if (mEvent)
                        OnMouseButtonPressed(*mEvent);
                    break;
                }
                case MouseButtonReleased:
                {
                    MouseButtonReleasedEvent* mEvent = static_cast<MouseButtonReleasedEvent*>(&e);
                    if (mEvent)
                        OnMouseButtonReleased(*mEvent);
                    break;
                }
                case MouseMoved:
                {
                    MouseMovedEvent* mEvent = static_cast<MouseMovedEvent*>(&e);
                    if (mEvent)
                        OnMouseMoved(*mEvent);
                    break;
                }

                default:
                {
                    break;
                }
            }
	    }
	}

}