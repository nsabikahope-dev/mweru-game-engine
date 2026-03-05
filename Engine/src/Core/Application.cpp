#include "Engine/Core/Application.h"
#include "Engine/Input/Input.h"

#include <SDL2/SDL.h>
#include <glad/glad.h>
#include <iostream>

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
static void EmscriptenLoop(void* arg)
{
    Engine::Application* app = static_cast<Engine::Application*>(arg);
    app->RunOneFrame();
}
#endif

namespace Engine {

Application* Application::s_Instance = nullptr;

Application::Application(const std::string& name, uint32_t width, uint32_t height)
{
    // Ensure singleton
    if (s_Instance)
    {
        std::cerr << "Application already exists!\n";
        return;
    }
    s_Instance = this;

    // Create window
    WindowProps props(name, width, height);
    m_Window = std::make_unique<Window>(props);
}

Application::~Application()
{
    OnShutdown();
}

void Application::RunOneFrame()
{
    if (!m_Running || m_Window->ShouldClose())
    {
#ifdef __EMSCRIPTEN__
        emscripten_cancel_main_loop();
        OnShutdown();
#endif
        return;
    }

    float time = SDL_GetTicks() / 1000.0f;
    Timestep timestep = time - m_LastFrameTime;
    m_LastFrameTime = time;

    Input::Update();
    m_Window->PollEvents();
    OnUpdate(timestep);

    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    OnRender();

    m_Window->SwapBuffers();
}

void Application::Run()
{
    OnInit();
    m_Window->SetEventCallback([this](SDL_Event* e) { HandleEvent(e); });

    std::cout << "Starting application...\n";
    m_Running = true;
    m_LastFrameTime = SDL_GetTicks() / 1000.0f;

#ifdef __EMSCRIPTEN__
    // Browser controls the loop — hand control to Emscripten
    emscripten_set_main_loop_arg(EmscriptenLoop, this, 0, 1);
#else
    while (m_Running && !m_Window->ShouldClose())
        RunOneFrame();

    std::cout << "Application shutting down...\n";
#endif
}

} // namespace Engine
