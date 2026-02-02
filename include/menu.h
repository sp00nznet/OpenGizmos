#pragma once

#ifdef _WIN32

#include <Windows.h>
#include <SDL.h>
#include <functional>
#include <string>

namespace opengg {

// Menu item IDs
enum MenuID {
    // File menu
    ID_FILE_NEW_GAME = 1001,
    ID_FILE_SAVE,
    ID_FILE_SAVE_AS,
    ID_FILE_LOAD,
    ID_FILE_EXIT,

    // Config menu
    ID_CONFIG_LOAD_GG_FILES = 2001,
    ID_CONFIG_CONTROLS,
    ID_CONFIG_SCALING,

    // Debug menu
    ID_DEBUG_ASSET_VIEWER = 3001,
    ID_DEBUG_MAP_VIEWER,
    ID_DEBUG_PUZZLE_DEBUGGER,
    ID_DEBUG_SAVE_EDITOR,

    // Game launch (under Debug)
    ID_DEBUG_LAUNCH_NEPTUNE = 3050,
    ID_DEBUG_LAUNCH_LABYRINTH,

    // Bot submenu (under Debug)
    ID_DEBUG_BOT_ENABLE = 3100,
    ID_DEBUG_BOT_DISABLE,
    ID_DEBUG_BOT_MODE_OBSERVE,
    ID_DEBUG_BOT_MODE_ASSIST,
    ID_DEBUG_BOT_MODE_AUTOPLAY,
    ID_DEBUG_BOT_MODE_SPEEDRUN,
    ID_DEBUG_BOT_GAME_GIZMOS,
    ID_DEBUG_BOT_GAME_NEPTUNE,
    ID_DEBUG_BOT_GAME_OUTNUMBERED,
    ID_DEBUG_BOT_GAME_SPELLBOUND,
    ID_DEBUG_BOT_GAME_TREASURE_MT,
    ID_DEBUG_BOT_GAME_TREASURE_MS,
    ID_DEBUG_BOT_GAME_TREASURE_COVE,
    ID_DEBUG_BOT_SHOW_STATUS,

    // About menu
    ID_ABOUT_INFO = 4001,
};

// Menu event callback type
using MenuCallback = std::function<void(MenuID)>;

class MenuBar {
public:
    MenuBar();
    ~MenuBar();

    // Initialize menu bar and attach to SDL window
    bool initialize(SDL_Window* window);

    // Set callback for menu events
    void setCallback(MenuCallback callback) { callback_ = callback; }

    // Process Windows messages (call from message loop)
    bool processMessage(const SDL_Event& event);

    // Enable/disable menu items
    void setItemEnabled(MenuID id, bool enabled);

    // Check/uncheck menu items
    void setItemChecked(MenuID id, bool checked);

    // Get the HWND
    HWND getHWND() const { return hwnd_; }

private:
    void createMenus();

    HWND hwnd_ = nullptr;
    HMENU menuBar_ = nullptr;
    HMENU fileMenu_ = nullptr;
    HMENU configMenu_ = nullptr;
    HMENU debugMenu_ = nullptr;
    HMENU botMenu_ = nullptr;      // Bot submenu
    HMENU botModeMenu_ = nullptr;  // Bot mode submenu
    HMENU botGameMenu_ = nullptr;  // Bot game type submenu
    HMENU aboutMenu_ = nullptr;

    MenuCallback callback_;

    // Original window procedure for subclassing
    WNDPROC originalWndProc_ = nullptr;

    // Static instance for window procedure callback
    static MenuBar* instance_;
    static LRESULT CALLBACK MenuWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
};

} // namespace opengg

#endif // _WIN32
