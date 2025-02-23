#pragma once

#include "CoffeeEngine/Core/ControllerCodes.h"
#include "CoffeeEngine/Core/KeyCodes.h"
#include "CoffeeEngine/Core/MouseCodes.h"
#include "CoffeeEngine/Input/Gamepad.h"
#include "CoffeeEngine/Events/ControllerEvent.h"

#include "CoffeeEngine/Events/Event.h"

#include <SDL3/SDL_gamepad.h>
#include <glm/glm.hpp>
#include <unordered_map>

namespace Coffee {
	// TODO Change for better action map layers method?
	/**
	 * @brief Current action context. Each key/button can only be in one InputAction per context layer
	 */
	enum class InputLayer : int
	{
		None = 0,
		Gameplay = BIT(1),
		Menu = BIT(2)
	};

    /**
     * @brief List of possible actions in ActionMap v0.1
     */
    enum class InputAction
    {
        // UI
    	Up,
    	Down,
    	Left,
    	Right,
        Confirm,
        Cancel,

        // Gameplay
    	MoveHorizontal,
		MoveVertical,
        Attack,
        Ability,
        Pause,

        // Action count for array creation and iteration
        ActionCount
    };

    /** Struct containing input details for a given action.
     *
     */
    class InputBinding
    {
		std::string Name = "Undefined";

        // Digital
        KeyCode KeyPos = Key::UNKNOWN; // Positive keyboard key
        KeyCode KeyNeg = Key::UNKNOWN; // Negative keyboard key
        ButtonCode ButtonPos = Button::Invalid; // Positive gamepad button
        ButtonCode ButtonNeg = Button::Invalid; // Negative gamepad button

        // Analog
        KeyCode Axis = Axis::Invalid; // Defined axis for analog control
        float AxisVal = 0; // Axis value

    };

    /**
     * @defgroup core Core
     * @brief Core components of the CoffeeEngine.
     * @{
     */
	class Input
	{
	public:

		static InputLayer CurrentInputContext;

        void Init();

        static void OnEvent(Event& e);

	    private:

	    static void OnAddController(ControllerAddEvent* cEvent);
	    static void OnRemoveController(ControllerRemoveEvent* event);
        static void OnButtonPressed(ButtonPressEvent& e);
        static void OnButtonReleased(ButtonReleaseEvent& e);
        static void OnAxisMoved(AxisMoveEvent& e);

        static std::vector<InputBinding> m_bindings;

	    static std::vector<Ref<Gamepad>> m_gamepads;
	    static std::unordered_map<ControllerCode, std::unordered_map<ButtonCode, bool>> m_buttonStates;
	    static std::unordered_map<ControllerCode, std::unordered_map<AxisCode, float>> m_axisStates;
	};
    /** @} */
}