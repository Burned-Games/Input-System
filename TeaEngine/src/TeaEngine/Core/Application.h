#pragma once

#include "TeaEngine/Core/Base.h"
#include "TeaEngine/Core/Window.h"
namespace Tea
{

    class Application
    {
      public:
        Application();
        virtual ~Application();

        void Run();

        void OnEvent(Event& e);
        private:
        Scope<Window> m_Window;
        bool m_Running = true;
    };

    // To be defined in CLIENT
    Application* CreateApplication();

} // namespace Tea
