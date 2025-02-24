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

    void Input::Init()
    {
        SDL_InitSubSystem(SDL_INIT_GAMEPAD);

        COFFEE_CORE_INFO("Input initialized");
    }

    void Input::OnAddController(ControllerAddEvent* event)
    {
        m_gamepads.emplace_back(new Gamepad(event->Controller));
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

    void Input::OnRemoveController(ControllerRemoveEvent* event)
    {
        // Remove controller by SDL_Gamepad ID
        auto pred = [&event](const Ref<Gamepad>& gamepad) {
            return gamepad->getId() == event->Controller;
        };
        erase_if(m_gamepads, pred);
    }
    void Input::OnButtonPressed(ButtonPressEvent& event) {
        COFFEE_INFO("Gamepad {0} Button {1} Pressed", event.Controller, event.Button);
        m_buttonStates[event.Controller][event.Button] = true;
    }

    void Input::OnButtonReleased(ButtonReleaseEvent& event) {
        COFFEE_INFO("Gamepad {0} Button {1} Released", event.Controller, event.Button);
        m_buttonStates[event.Controller][event.Button] = false;
    }

    void Input::OnAxisMoved(AxisMoveEvent& event) {
        //COFFEE_INFO("Gamepad {0} Axis {1} value {2}", event.Controller, event.Axis, event.Value);
        m_axisStates[event.Controller][event.Axis] = event.Value;
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

                default:
                {
                    break;
                }
            }
	    }
	}

}