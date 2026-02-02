#ifdef _WIN32

#include "menu.h"
#include <SDL_syswm.h>

namespace opengg {

// Static instance for window procedure
MenuBar* MenuBar::instance_ = nullptr;

MenuBar::MenuBar() {
    instance_ = this;
}

MenuBar::~MenuBar() {
    // Restore original window procedure
    if (hwnd_ && originalWndProc_) {
        SetWindowLongPtr(hwnd_, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(originalWndProc_));
    }

    // Destroy menus
    if (menuBar_) {
        DestroyMenu(menuBar_);
    }

    instance_ = nullptr;
}

bool MenuBar::initialize(SDL_Window* window) {
    // Get the Windows HWND from SDL
    SDL_SysWMinfo wmInfo;
    SDL_VERSION(&wmInfo.version);

    if (!SDL_GetWindowWMInfo(window, &wmInfo)) {
        SDL_Log("Failed to get window info: %s", SDL_GetError());
        return false;
    }

    hwnd_ = wmInfo.info.win.window;

    // Create the menu bar
    createMenus();

    // Attach menu to window
    if (!SetMenu(hwnd_, menuBar_)) {
        SDL_Log("Failed to set menu bar");
        return false;
    }

    // Subclass the window to intercept menu messages
    originalWndProc_ = reinterpret_cast<WNDPROC>(
        SetWindowLongPtr(hwnd_, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(MenuWndProc))
    );

    // Force window to recalculate size with menu
    DrawMenuBar(hwnd_);

    return true;
}

void MenuBar::createMenus() {
    // Create the main menu bar
    menuBar_ = CreateMenu();

    // === File Menu ===
    fileMenu_ = CreatePopupMenu();
    AppendMenuW(fileMenu_, MF_STRING, ID_FILE_NEW_GAME, L"&New Game\tCtrl+N");
    AppendMenuW(fileMenu_, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(fileMenu_, MF_STRING, ID_FILE_SAVE, L"&Save\tCtrl+S");
    AppendMenuW(fileMenu_, MF_STRING, ID_FILE_SAVE_AS, L"Save &As...\tCtrl+Shift+S");
    AppendMenuW(fileMenu_, MF_STRING, ID_FILE_LOAD, L"&Load...\tCtrl+O");
    AppendMenuW(fileMenu_, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(fileMenu_, MF_STRING, ID_FILE_EXIT, L"E&xit\tAlt+F4");
    AppendMenuW(menuBar_, MF_POPUP, reinterpret_cast<UINT_PTR>(fileMenu_), L"&File");

    // Disable save options until game is loaded
    EnableMenuItem(fileMenu_, ID_FILE_SAVE, MF_GRAYED);
    EnableMenuItem(fileMenu_, ID_FILE_SAVE_AS, MF_GRAYED);

    // === Config Menu ===
    configMenu_ = CreatePopupMenu();
    AppendMenuW(configMenu_, MF_STRING, ID_CONFIG_LOAD_GG_FILES, L"&Load GG Files...");
    AppendMenuW(configMenu_, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(configMenu_, MF_STRING, ID_CONFIG_CONTROLS, L"&Controls...");
    AppendMenuW(configMenu_, MF_STRING, ID_CONFIG_SCALING, L"&Scaling...");
    AppendMenuW(menuBar_, MF_POPUP, reinterpret_cast<UINT_PTR>(configMenu_), L"&Config");

    // === Debug Menu ===
    debugMenu_ = CreatePopupMenu();
    AppendMenuW(debugMenu_, MF_STRING, ID_DEBUG_ASSET_VIEWER, L"&Asset Viewer");
    AppendMenuW(debugMenu_, MF_STRING, ID_DEBUG_MAP_VIEWER, L"&Map Viewer");
    AppendMenuW(debugMenu_, MF_STRING, ID_DEBUG_PUZZLE_DEBUGGER, L"&Puzzle Debugger");
    AppendMenuW(debugMenu_, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(debugMenu_, MF_STRING, ID_DEBUG_SAVE_EDITOR, L"&Save Editor");
    AppendMenuW(menuBar_, MF_POPUP, reinterpret_cast<UINT_PTR>(debugMenu_), L"&Debug");

    // === About Menu ===
    aboutMenu_ = CreatePopupMenu();
    AppendMenuW(aboutMenu_, MF_STRING, ID_ABOUT_INFO, L"&About OpenGizmos...");
    AppendMenuW(menuBar_, MF_POPUP, reinterpret_cast<UINT_PTR>(aboutMenu_), L"&About");
}

LRESULT CALLBACK MenuBar::MenuWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (instance_ && instance_->originalWndProc_) {
        switch (msg) {
            case WM_COMMAND: {
                WORD id = LOWORD(wParam);
                if (id >= 1000 && id < 5000) {
                    // This is one of our menu commands
                    if (instance_->callback_) {
                        instance_->callback_(static_cast<MenuID>(id));
                    }
                    return 0;
                }
                break;
            }
        }

        // Call original window procedure
        return CallWindowProc(instance_->originalWndProc_, hwnd, msg, wParam, lParam);
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

bool MenuBar::processMessage(const SDL_Event& event) {
    // SDL handles most Windows messages internally
    // Menu commands are processed via the subclassed window procedure
    return false;
}

void MenuBar::setItemEnabled(MenuID id, bool enabled) {
    HMENU menu = nullptr;

    if (id >= 1001 && id < 2000) menu = fileMenu_;
    else if (id >= 2001 && id < 3000) menu = configMenu_;
    else if (id >= 3001 && id < 4000) menu = debugMenu_;
    else if (id >= 4001 && id < 5000) menu = aboutMenu_;

    if (menu) {
        EnableMenuItem(menu, id, enabled ? MF_ENABLED : MF_GRAYED);
    }
}

void MenuBar::setItemChecked(MenuID id, bool checked) {
    HMENU menu = nullptr;

    if (id >= 1001 && id < 2000) menu = fileMenu_;
    else if (id >= 2001 && id < 3000) menu = configMenu_;
    else if (id >= 3001 && id < 4000) menu = debugMenu_;
    else if (id >= 4001 && id < 5000) menu = aboutMenu_;

    if (menu) {
        CheckMenuItem(menu, id, checked ? MF_CHECKED : MF_UNCHECKED);
    }
}

} // namespace opengg

#endif // _WIN32
