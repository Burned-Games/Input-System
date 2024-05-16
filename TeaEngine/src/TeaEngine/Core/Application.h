#ifndef APPLICATION_H
#define APPLICATION_H

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
        private:
        Scope<Window> m_Window;
    };

    // To be defined in CLIENT
    Application* CreateApplication();

} // namespace Tea
#endif /* APPLICATION_H */
