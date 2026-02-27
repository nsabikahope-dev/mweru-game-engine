#pragma once

#include <string>
#include <cstdint>
#include <functional>

struct SDL_Window;
typedef void* SDL_GLContext;
union SDL_Event;

namespace Engine {

struct WindowProps
{
    std::string Title;
    uint32_t Width;
    uint32_t Height;
    bool VSync;

    WindowProps(const std::string& title = "Game Engine",
                uint32_t width = 1280,
                uint32_t height = 720,
                bool vsync = true)
        : Title(title), Width(width), Height(height), VSync(vsync)
    {
    }
};

/**
 * @brief Window abstraction wrapping SDL2 and OpenGL context
 *
 * Usage:
 *   WindowProps props("My Game", 1920, 1080);
 *   Window window(props);
 *   while (!window.ShouldClose()) {
 *       window.OnUpdate();
 *   }
 */
class Window
{
public:
    Window(const WindowProps& props);
    ~Window();

    void OnUpdate();
    void PollEvents();
    void SwapBuffers();

    using EventCallback = std::function<void(SDL_Event*)>;
    void SetEventCallback(EventCallback callback) { m_EventCallback = std::move(callback); }

    uint32_t GetWidth() const { return m_Data.Width; }
    uint32_t GetHeight() const { return m_Data.Height; }

    void SetVSync(bool enabled);
    bool IsVSync() const { return m_Data.VSync; }

    bool ShouldClose() const { return m_ShouldClose; }

    SDL_Window* GetNativeWindow() const { return m_Window; }
    SDL_GLContext GetNativeContext() const { return m_Context; }

private:
    void Init(const WindowProps& props);
    void Shutdown();

private:
    SDL_Window* m_Window;
    EventCallback m_EventCallback;
    SDL_GLContext m_Context;
    bool m_ShouldClose = false;

    struct WindowData
    {
        std::string Title;
        uint32_t Width, Height;
        bool VSync;
    };

    WindowData m_Data;
};

} // namespace Engine
