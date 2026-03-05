#include "../include/UpdateChecker.h"

#include <array>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#endif

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static std::string ReadFile(const char* path)
{
    std::ifstream f(path);
    if (!f.is_open()) return {};
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

// Extract the value of a JSON string field, e.g. "tag_name": "v1.0.1" -> "v1.0.1"
static std::string ExtractJsonString(const std::string& json, const std::string& key)
{
    std::string search = "\"" + key + "\": \"";
    auto pos = json.find(search);
    if (pos == std::string::npos) return {};
    pos += search.size();
    auto end = json.find('"', pos);
    if (end == std::string::npos) return {};
    return json.substr(pos, end - pos);
}

// Strip leading 'v' from a version tag (e.g. "v1.0.1" -> "1.0.1")
static std::string StripV(std::string v)
{
    if (!v.empty() && v[0] == 'v') v.erase(0, 1);
    return v;
}

// ---------------------------------------------------------------------------
// UpdateChecker::CompareVersions
// Returns -1 if a < b, 0 if a == b, +1 if a > b
// ---------------------------------------------------------------------------
int UpdateChecker::CompareVersions(const std::string& a, const std::string& b)
{
    auto parse = [](const std::string& s) -> std::array<int, 3>
    {
        std::array<int, 3> parts = {0, 0, 0};
        std::istringstream ss(s);
        std::string token;
        int i = 0;
        while (std::getline(ss, token, '.') && i < 3)
            parts[i++] = std::stoi(token);
        return parts;
    };

    auto va = parse(a);
    auto vb = parse(b);
    for (int i = 0; i < 3; ++i)
    {
        if (va[i] < vb[i]) return -1;
        if (va[i] > vb[i]) return  1;
    }
    return 0;
}

// ---------------------------------------------------------------------------
// UpdateChecker::DoCheck  (runs on background thread)
// ---------------------------------------------------------------------------
void UpdateChecker::DoCheck()
{
#ifdef _WIN32
    const char* tmpFile = "engine_update_check.json";
    std::string cmd =
        "curl -s -L --max-time 10 "
        "-H \"Accept: application/vnd.github+json\" "
        "-H \"X-GitHub-Api-Version: 2022-11-28\" "
        "\"https://api.github.com/repos/nsabikahope-dev/mweru-game-engine/releases/latest\" "
        "-o engine_update_check.json 2>nul";
#else
    const char* tmpFile = "/tmp/engine_update_check.json";
    std::string cmd =
        "curl -s -L --max-time 10 "
        "-H \"Accept: application/vnd.github+json\" "
        "-H \"X-GitHub-Api-Version: 2022-11-28\" "
        "\"https://api.github.com/repos/nsabikahope-dev/mweru-game-engine/releases/latest\" "
        "-o /tmp/engine_update_check.json 2>/dev/null";
#endif

    std::system(cmd.c_str());

    std::string json = ReadFile(tmpFile);
    if (json.empty())
    {
        m_State.store(State::Error, std::memory_order_release);
        return;
    }

    std::string tag = StripV(ExtractJsonString(json, "tag_name"));
    if (tag.empty())
    {
        m_State.store(State::Error, std::memory_order_release);
        return;
    }

    // Write version string BEFORE updating state so it's visible to the main thread
    m_LatestVersion = tag;

    int cmp = CompareVersions(ENGINE_VERSION, tag);
    if (cmp < 0)
        m_State.store(State::UpdateAvailable, std::memory_order_release);
    else
        m_State.store(State::UpToDate, std::memory_order_release);
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

UpdateChecker::~UpdateChecker()
{
    if (m_Thread.joinable())
        m_Thread.join();
}

void UpdateChecker::CheckAsync()
{
    // Only start a new check when idle
    State expected = State::Idle;
    if (!m_State.compare_exchange_strong(expected, State::Checking,
                                         std::memory_order_acq_rel))
        return;

    if (m_Thread.joinable())
        m_Thread.join();

    m_Thread = std::thread(&UpdateChecker::DoCheck, this);
}

UpdateChecker::State UpdateChecker::GetState() const
{
    return m_State.load(std::memory_order_acquire);
}

std::string UpdateChecker::GetLatestVersion() const
{
    return m_LatestVersion;
}

bool UpdateChecker::IsUpdateAvailable() const
{
    return GetState() == State::UpdateAvailable;
}

void UpdateChecker::OpenReleasePage() const
{
    const char* url = "https://github.com/nsabikahope-dev/mweru-game-engine/releases/latest";
#ifdef _WIN32
    ShellExecuteA(NULL, "open", url, NULL, NULL, SW_SHOWNORMAL);
#else
    std::string cmd = std::string("xdg-open \"") + url + "\" &";
    std::system(cmd.c_str());
#endif
}

void UpdateChecker::Dismiss()
{
    m_State.store(State::Idle, std::memory_order_release);
}
