#pragma once
#include <string>
#include <thread>
#include <atomic>

#define ENGINE_VERSION "1.0.0"

class UpdateChecker
{
public:
    enum class State { Idle, Checking, UpdateAvailable, UpToDate, Error };

    // Start a background check. No-op if a check is already in progress.
    void CheckAsync();

    State       GetState()         const;
    std::string GetLatestVersion() const; // e.g. "1.0.1" — valid after UpdateAvailable
    bool        IsUpdateAvailable() const;

    // Opens the GitHub releases page in the system browser.
    void OpenReleasePage() const;

    // Dismiss the notification (resets state to Idle).
    void Dismiss();

    ~UpdateChecker(); // Joins background thread

private:
    void DoCheck(); // Runs on background thread

    static int CompareVersions(const std::string& a, const std::string& b); // -1 / 0 / +1

    std::thread        m_Thread;
    std::atomic<State> m_State { State::Idle };
    std::string        m_LatestVersion; // written before m_State update; safe to read after
};
