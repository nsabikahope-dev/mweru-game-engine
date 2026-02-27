#include "Engine/Core/Application.h"
#include "Engine/Input/Input.h"

#include <SDL2/SDL.h>
#include <glad/glad.h>
#include <iostream>

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

void Application::Run()
{
    // Call user init now that object is fully constructed
    OnInit();

    // Register per-event callback so subclasses (e.g. the Editor) can forward
    // raw SDL events to ImGui or other backends before Input state is updated.
    m_Window->SetEventCallback([this](SDL_Event* e) { HandleEvent(e); });

    std::cout << "Starting application...\n";

    m_Running = true;
    m_LastFrameTime = SDL_GetTicks() / 1000.0f;

    // Main game loop
    while (m_Running && !m_Window->ShouldClose())
    {
        // Calculate delta time
        float time = SDL_GetTicks() / 1000.0f;
        Timestep timestep = time - m_LastFrameTime;
        m_LastFrameTime = time;

        // Snapshot previous frame's input state BEFORE processing new events
        Input::Update();

        // Poll SDL events and feed them into the Input state
        m_Window->PollEvents();

        // Update game logic
        OnUpdate(timestep);

        // Render
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        OnRender();

        // Swap buffers to display
        m_Window->SwapBuffers();
    }

    std::cout << "Application shutting down...\n";
}

} // namespace Engine
