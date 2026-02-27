#include "Engine/Core/Window.h"
#include "Engine/Input/Input.h"

#include <SDL2/SDL.h>
#include <glad/glad.h>
#include <iostream>

namespace Engine {

Window::Window(const WindowProps& props)
{
    Init(props);
}

Window::~Window()
{
    Shutdown();
}

void Window::Init(const WindowProps& props)
{
    m_Data.Title = props.Title;
    m_Data.Width = props.Width;
    m_Data.Height = props.Height;
    m_Data.VSync = props.VSync;

    std::cout << "Creating window " << m_Data.Title << " (" << m_Data.Width << "x" << m_Data.Height << ")\n";

    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        std::cerr << "Failed to initialize SDL: " << SDL_GetError() << "\n";
        return;
    }

    // Set OpenGL version (3.3 Core)
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

    // Enable double buffering
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

    // Create window
    m_Window = SDL_CreateWindow(
        m_Data.Title.c_str(),
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        m_Data.Width,
        m_Data.Height,
        SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE
    );

    if (!m_Window)
    {
        std::cerr << "Failed to create window: " << SDL_GetError() << "\n";
        SDL_Quit();
        return;
    }

    // Create OpenGL context
    m_Context = SDL_GL_CreateContext(m_Window);
    if (!m_Context)
    {
        std::cerr << "Failed to create OpenGL context: " << SDL_GetError() << "\n";
        SDL_DestroyWindow(m_Window);
        SDL_Quit();
        return;
    }

    // Make context current
    SDL_GL_MakeCurrent(m_Window, m_Context);

    // Initialize GLAD
    int version = gladLoadGL((GLADloadfunc)SDL_GL_GetProcAddress);
    if (version == 0)
    {
        std::cerr << "Failed to initialize GLAD\n";
        return;
    }

    std::cout << "OpenGL Info:\n";
    std::cout << "  Vendor: " << glGetString(GL_VENDOR) << "\n";
    std::cout << "  Renderer: " << glGetString(GL_RENDERER) << "\n";
    std::cout << "  Version: " << glGetString(GL_VERSION) << "\n";

    // Set VSync
    SetVSync(m_Data.VSync);
}

void Window::Shutdown()
{
    if (m_Context)
    {
        SDL_GL_DeleteContext(m_Context);
    }

    if (m_Window)
    {
        SDL_DestroyWindow(m_Window);
    }

    SDL_Quit();
}

void Window::PollEvents()
{
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        if (m_EventCallback)
            m_EventCallback(&event);

        if (event.type == SDL_QUIT)
        {
            m_ShouldClose = true;
        }
        else if (event.type == SDL_WINDOWEVENT)
        {
            if (event.window.event == SDL_WINDOWEVENT_RESIZED)
            {
                m_Data.Width = event.window.data1;
                m_Data.Height = event.window.data2;
                glViewport(0, 0, m_Data.Width, m_Data.Height);
            }
        }
        else if (event.type == SDL_KEYDOWN)
        {
            int scancode = event.key.keysym.scancode;
            Input::SetKeyState(static_cast<KeyCode>(scancode), true);
        }
        else if (event.type == SDL_KEYUP)
        {
            // Update Input system with key release
            int scancode = event.key.keysym.scancode;
            Input::SetKeyState(static_cast<KeyCode>(scancode), false);
        }
        else if (event.type == SDL_MOUSEBUTTONDOWN)
        {
            // Update Input system with mouse button press
            Input::SetMouseButtonState(static_cast<MouseButton>(event.button.button), true);
        }
        else if (event.type == SDL_MOUSEBUTTONUP)
        {
            // Update Input system with mouse button release
            Input::SetMouseButtonState(static_cast<MouseButton>(event.button.button), false);
        }
        else if (event.type == SDL_MOUSEMOTION)
        {
            // Update Input system with mouse position
            Input::SetMousePosition(static_cast<float>(event.motion.x), static_cast<float>(event.motion.y));
        }
        else if (event.type == SDL_MOUSEWHEEL)
        {
            // Update Input system with mouse scroll
            Input::SetMouseScroll(static_cast<float>(event.wheel.y));
        }
    }
}

void Window::SwapBuffers()
{
    SDL_GL_SwapWindow(m_Window);
}

void Window::OnUpdate()
{
    PollEvents();
    SwapBuffers();
}

void Window::SetVSync(bool enabled)
{
    if (SDL_GL_SetSwapInterval(enabled ? 1 : 0) < 0)
    {
        std::cerr << "Warning: Failed to set VSync: " << SDL_GetError() << "\n";
    }
    m_Data.VSync = enabled;
}

} // namespace Engine
