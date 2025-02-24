#include "CoffeeEngine/Core/Input.h"

#include "CoffeeEngine/Events/ControllerEvent.h"
#include "CoffeeEngine/Events/Event.h"
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

    void Input::OnAddController(ControllerAddEvent* event)
    {
        m_gamepads.emplace_back(new Gamepad(event->Controller));
    }


    void Input::OnRemoveController(ControllerRemoveEvent* event)
    {
        // Remove controller by SDL_Gamepad ID
        auto pred = [&event](Ref<Gamepad> gamepad) {
            return gamepad->getId() == event->Controller;
        };
        erase_if(m_gamepads, pred);
    }
    void Input::OnButtonPressed(ButtonPressEvent& bEvent) {
        COFFEE_INFO("Gamepad {0} Button {1} Pressed", bEvent.Controller, bEvent.Button);
        m_buttonStates[bEvent.Controller][bEvent.Button] = true;
    }

    void Input::OnButtonReleased(ButtonReleaseEvent& bEvent) {
        COFFEE_INFO("Gamepad {0} Button {1} Released", bEvent.Controller, bEvent.Button);
        m_buttonStates[bEvent.Controller][bEvent.Button] = false;
    }

    void Input::OnAxisMoved(AxisMoveEvent& aEvent) {
        COFFEE_INFO("Gamepad {0} Axis {1} value {2}", aEvent.Controller, aEvent.Axis, aEvent.Value);
        m_axisStates[aEvent.Controller][aEvent.Axis] = aEvent.Value;
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
                    break;
                }
                case KeyReleased:
                {
                    break;
                }
                case MouseButtonReleased:
                {
                    break;
                }
                case MouseMoved:
                {
                    break;
                }
                case MouseScrolled:
                {
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