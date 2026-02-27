#pragma once

#include "Engine/Core/Window.h"
#include "Engine/Core/Timestep.h"
#include <memory>

union SDL_Event;

namespace Engine {

/**
 * @brief Base application class with game loop
 *
 * Usage:
 *   class MyApp : public Application {
 *     public:
 *       MyApp() : Application("My Game") {}
 *       void OnUpdate(Timestep ts) override {
 *           // Update game logic
 *       }
 *       void OnRender() override {
 *           // Render game
 *       }
 *   };
 *
 *   int main() {
 *       MyApp app;
 *       app.Run();
 *       return 0;
 *   }
 */
class Application
{
public:
    Application(const std::string& name = "Game Engine",
                uint32_t width = 1280,
                uint32_t height = 720);
    virtual ~Application();

    void Run();

    /** Request the application to stop at the end of the current frame. */
    void Close() { m_Running = false; }

    Window& GetWindow() { return *m_Window; }

    static Application& Get() { return *s_Instance; }

protected:
    /**
     * @brief Called once at startup
     */
    virtual void OnInit() {}

    /**
     * @brief Called every frame for game logic updates
     */
    virtual void OnUpdate(Timestep ts) {}

    /**
     * @brief Called every frame for rendering
     */
    virtual void OnRender() {}

    /**
     * @brief Called once at shutdown
     */
    virtual void OnShutdown() {}

    /**
     * @brief Called for every SDL event before engine input processing.
     *        Override to forward events to third-party backends (e.g. ImGui).
     */
    virtual void HandleEvent(SDL_Event* event) {}

private:
    std::unique_ptr<Window> m_Window;
    bool m_Running = true;
    float m_LastFrameTime = 0.0f;

    static Application* s_Instance;
};

} // namespace Engine
