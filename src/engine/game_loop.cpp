#include "game_loop.h"
#include "renderer.h"
#include "audio.h"
#include "input.h"
#include "asset_cache.h"
#include "font.h"
#include "game_registry.h"
#include "bot/bot_manager.h"
#include "neptune/neptune_game.h"
#ifdef _WIN32
#include "menu.h"
#include "asset_viewer.h"
#include <ShlObj.h>
#include <commdlg.h>
#endif
#include <SDL.h>
#include <fstream>
#include <filesystem>
#include <thread>

namespace fs = std::filesystem;

namespace opengg {

Game::Game() = default;

Game::~Game() {
    shutdown();
}

bool Game::initialize(const GameConfig& config) {
    config_ = config;

#ifdef _WIN32
    // Initialize COM for file dialogs
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
#endif

    // Auto-detect game path if not specified
    if (config_.gamePath.empty()) {
        if (!detectGame()) {
            return false;
        }
    }

    // Set default cache path if not specified
    if (config_.cachePath.empty()) {
        // Use user's app data directory
        #ifdef _WIN32
        const char* appData = std::getenv("LOCALAPPDATA");
        if (appData) {
            config_.cachePath = std::string(appData) + "/OpenGizmos/cache";
        } else {
            config_.cachePath = "./cache";
        }
        #else
        const char* home = std::getenv("HOME");
        if (home) {
            config_.cachePath = std::string(home) + "/.opengg/cache";
        } else {
            config_.cachePath = "./cache";
        }
        #endif
    }

    // Set default config path
    if (config_.configPath.empty()) {
        #ifdef _WIN32
        const char* appData = std::getenv("LOCALAPPDATA");
        if (appData) {
            config_.configPath = std::string(appData) + "/OpenGizmos";
        } else {
            config_.configPath = ".";
        }
        #else
        const char* home = std::getenv("HOME");
        if (home) {
            config_.configPath = std::string(home) + "/.opengg";
        } else {
            config_.configPath = ".";
        }
        #endif
    }

    // Create directories
    try {
        fs::create_directories(config_.cachePath);
        fs::create_directories(config_.configPath);
    } catch (const std::exception&) {
        // Non-fatal, continue anyway
    }

    // Initialize renderer
    renderer_ = std::make_unique<Renderer>();
    if (!renderer_->initialize(config_.windowTitle, config_.windowWidth, config_.windowHeight)) {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error",
            ("Failed to initialize renderer: " + renderer_->getLastError()).c_str(), nullptr);
        return false;
    }

    if (config_.fullscreen) {
        renderer_->setFullscreen(true);
    }

    // Initialize audio
    audio_ = std::make_unique<AudioSystem>();
    if (!audio_->initialize()) {
        // Audio failure is non-fatal, just log it
        SDL_Log("Warning: Audio initialization failed: %s", audio_->getLastError().c_str());
    }

    // Initialize input
    input_ = std::make_unique<InputSystem>();

    // Try to load key bindings
    std::string bindingsPath = config_.configPath + "/keybindings.cfg";
    input_->loadBindings(bindingsPath);

    // Initialize asset cache
    assetCache_ = std::make_unique<AssetCache>();
    if (!assetCache_->initialize(config_.gamePath, config_.cachePath)) {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error",
            ("Failed to initialize asset cache: " + assetCache_->getLastError()).c_str(), nullptr);
        return false;
    }
    assetCache_->setRenderer(renderer_->getSDLRenderer());
    audio_->setAssetCache(assetCache_.get());

    // Initialize text renderer
    textRenderer_ = std::make_unique<TextRenderer>();
    if (!textRenderer_->initialize(renderer_->getSDLRenderer())) {
        SDL_Log("Warning: Text renderer initialization failed");
    }

#ifdef _WIN32
    // Initialize menu bar
    menuBar_ = std::make_unique<MenuBar>();
    if (!menuBar_->initialize(renderer_->getSDLWindow())) {
        SDL_Log("Warning: Menu bar initialization failed");
    } else {
        // Set up menu callback
        menuBar_->setCallback([this](MenuID id) {
            handleMenuCommand(id);
        });
    }
#endif

    // Initialize game registry and discover extracted games
    gameRegistry_ = std::make_unique<GameRegistry>();
    {
        // Search for extracted games directory relative to the executable or known paths
        std::vector<std::string> extractedPaths = {
            "C:/ggng/extracted",
            "../extracted",
            "./extracted",
            config_.gamePath + "/extracted",
        };

        for (const auto& path : extractedPaths) {
            if (fs::exists(path + "/all_games_manifest.json")) {
                if (gameRegistry_->discoverGames(path)) {
                    // Configure asset cache to use extracted base path
                    assetCache_->setExtractedBasePath(path);
                    SDL_Log("Found extracted games at: %s", path.c_str());
                    break;
                }
            }
        }
    }

    // Load config
    loadConfig();

    // Initialize bot manager
    Bot::BotManager::getInstance().initialize(this);

    // Initialize timing
    startTime_ = Clock::now();
    lastFrameTime_ = startTime_;

    running_ = true;
    return true;
}

void Game::run() {
    while (running_) {
        processFrame();
    }
}

void Game::processFrame() {
    updateTiming();

    // Process input
    input_->processEvents();

    if (input_->shouldQuit()) {
        running_ = false;
        return;
    }

#ifdef _WIN32
    // Update asset viewer window if open
    if (assetViewer_ && assetViewer_->isOpen()) {
        assetViewer_->update();
    }
#endif

    // Get current state
    GameState* state = getCurrentState();

    if (state) {
        // Handle input
        state->handleInput();

        // Update (unless paused)
        if (!paused_) {
            state->update(deltaTime_);

            // Update bot system
            auto& botMgr = Bot::BotManager::getInstance();
            if (botMgr.isEnabled()) {
                botMgr.update(deltaTime_);

                // In AutoPlay or SpeedRun mode, execute bot decisions
                if (botMgr.getMode() == Bot::BotMode::AutoPlay ||
                    botMgr.getMode() == Bot::BotMode::SpeedRun) {
                    botMgr.executeDecision(input_.get());
                }
            }
        }

        // Render
        renderer_->beginFrame();
        state->render();
        renderer_->endFrame();
        renderer_->present();
    } else {
        // No state - just clear screen
        renderer_->clear();
        renderer_->present();
    }

    input_->endFrame();
    frameCount_++;

    // Frame rate limiting (if not using vsync)
    if (!config_.vsync && config_.targetFPS > 0) {
        float targetFrameTime = 1.0f / config_.targetFPS;
        float elapsed = deltaTime_;
        if (elapsed < targetFrameTime) {
            int sleepMs = static_cast<int>((targetFrameTime - elapsed) * 1000);
            if (sleepMs > 0) {
                std::this_thread::sleep_for(std::chrono::milliseconds(sleepMs));
            }
        }
    }
}

void Game::updateTiming() {
    auto now = Clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(now - lastFrameTime_);
    deltaTime_ = duration.count() / 1000000.0f;
    lastFrameTime_ = now;

    // Clamp delta time to avoid spiral of death
    if (deltaTime_ > 0.1f) {
        deltaTime_ = 0.1f;
    }

    // Update FPS counter
    fpsAccumulator_ += deltaTime_;
    fpsFrameCount_++;
    if (fpsAccumulator_ >= 1.0f) {
        fps_ = fpsFrameCount_ / fpsAccumulator_;
        fpsAccumulator_ = 0.0f;
        fpsFrameCount_ = 0;
    }
}

double Game::getElapsedTime() const {
    auto now = Clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(now - startTime_);
    return duration.count() / 1000000.0;
}

void Game::shutdown() {
    // Save config
    saveConfig();

    // Save key bindings
    if (input_) {
        std::string bindingsPath = config_.configPath + "/keybindings.cfg";
        input_->saveBindings(bindingsPath);
    }

    // Shutdown bot manager
    Bot::BotManager::getInstance().shutdown();

    // Clear state stack
    while (!stateStack_.empty()) {
        stateStack_.back()->exit();
        stateStack_.pop_back();
    }

    // Shutdown subsystems in reverse order
#ifdef _WIN32
    assetViewer_.reset();
    menuBar_.reset();
#endif
    gameRegistry_.reset();
    textRenderer_.reset();
    assetCache_.reset();
    input_.reset();
    audio_.reset();
    renderer_.reset();

    SDL_Quit();

#ifdef _WIN32
    CoUninitialize();
#endif
}

void Game::pushState(std::unique_ptr<GameState> state) {
    if (!stateStack_.empty()) {
        // Pause current state (don't exit)
    }
    stateStack_.push_back(std::move(state));
    stateStack_.back()->enter();
}

void Game::popState() {
    if (!stateStack_.empty()) {
        stateStack_.back()->exit();
        stateStack_.pop_back();

        if (!stateStack_.empty()) {
            // Resume previous state
        }
    }
}

void Game::changeState(std::unique_ptr<GameState> state) {
    // Exit all current states
    while (!stateStack_.empty()) {
        stateStack_.back()->exit();
        stateStack_.pop_back();
    }

    // Push new state
    stateStack_.push_back(std::move(state));
    stateStack_.back()->enter();
}

GameState* Game::getCurrentState() {
    if (stateStack_.empty()) {
        return nullptr;
    }
    return stateStack_.back().get();
}

bool Game::detectGame() {
    // First, check for extracted games (the multi-game launcher path)
    std::vector<std::string> extractedPaths = {
        "C:/ggng/extracted",
        "../extracted",
        "./extracted",
    };

    for (const auto& path : extractedPaths) {
        if (fs::exists(path + "/all_games_manifest.json")) {
            SDL_Log("Found extracted games manifest at: %s", path.c_str());
            // Use parent of extracted dir as the game path
            config_.gamePath = fs::path(path).parent_path().string();
            if (config_.gamePath.empty()) config_.gamePath = ".";
            return true;
        }
    }

    // Legacy: search for original Gizmos & Gadgets installation
    std::vector<std::string> searchPaths = {
        ".",
        "./SSGWIN32",
        "C:/ggng/iso",
        "C:/GOG Games/Super Solvers Gizmos and Gadgets",
        "C:/Program Files (x86)/Steam/steamapps/common/Super Solvers Gizmos and Gadgets",
        "C:/Program Files/TLC/Gizmos & Gadgets",
    };

    std::vector<std::string> requiredFiles = {
        "SSGWINCD/GIZMO.DAT",
    };

    for (const auto& basePath : searchPaths) {
        bool found = true;
        for (const auto& file : requiredFiles) {
            std::string fullPath = basePath + "/" + file;
            if (!fs::exists(fullPath)) {
                found = false;
                break;
            }
        }

        if (found) {
            config_.gamePath = basePath;
            return true;
        }
    }

    // No game files found - run in demo/launcher mode anyway
    config_.gamePath = ".";
    return true;
}

bool Game::loadConfig() {
    std::string configFile = config_.configPath + "/opengg.cfg";
    std::ifstream file(configFile);
    if (!file) {
        return false;
    }

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;

        size_t eq = line.find('=');
        if (eq == std::string::npos) continue;

        std::string key = line.substr(0, eq);
        std::string value = line.substr(eq + 1);

        // Trim whitespace
        while (!key.empty() && std::isspace(key.back())) key.pop_back();
        while (!value.empty() && std::isspace(value.front())) value.erase(0, 1);

        if (key == "fullscreen") {
            config_.fullscreen = (value == "true" || value == "1");
        } else if (key == "vsync") {
            config_.vsync = (value == "true" || value == "1");
        } else if (key == "gamePath") {
            config_.gamePath = value;
        } else if (key == "extractedPath") {
            if (assetCache_) assetCache_->setExtractedBasePath(value);
            if (gameRegistry_ && gameRegistry_->getAvailableCount() == 0) {
                gameRegistry_->discoverGames(value);
            }
        } else if (key == "sfxVolume") {
            if (audio_) audio_->setSFXVolume(std::stof(value));
        } else if (key == "musicVolume") {
            if (audio_) audio_->setMusicVolume(std::stof(value));
        }
    }

    return true;
}

bool Game::saveConfig() {
    std::string configFile = config_.configPath + "/opengg.cfg";
    std::ofstream file(configFile);
    if (!file) {
        return false;
    }

    file << "# OpenGG Configuration\n\n";
    file << "fullscreen=" << (config_.fullscreen ? "true" : "false") << "\n";
    file << "vsync=" << (config_.vsync ? "true" : "false") << "\n";
    file << "gamePath=" << config_.gamePath << "\n";
    if (assetCache_ && !assetCache_->getExtractedBasePath().empty()) {
        file << "extractedPath=" << assetCache_->getExtractedBasePath() << "\n";
    }

    if (audio_) {
        file << "sfxVolume=" << audio_->getSFXVolume() << "\n";
        file << "musicVolume=" << audio_->getMusicVolume() << "\n";
    }

    return true;
}

#ifdef _WIN32
bool Game::browseForGameFolder() {
    HWND hwnd = menuBar_ ? menuBar_->getHWND() : nullptr;

    // Use IFileDialog for modern folder picker (Vista+)
    IFileDialog* pFileDialog = nullptr;
    HRESULT hr = CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER,
                                   IID_IFileDialog, reinterpret_cast<void**>(&pFileDialog));

    if (FAILED(hr)) {
        SDL_Log("Failed to create file dialog");
        return false;
    }

    // Set options for folder selection
    DWORD options;
    pFileDialog->GetOptions(&options);
    pFileDialog->SetOptions(options | FOS_PICKFOLDERS | FOS_FORCEFILESYSTEM);

    // Set title
    pFileDialog->SetTitle(L"Select Game Installation Folder");

    // Show the dialog
    hr = pFileDialog->Show(hwnd);

    if (SUCCEEDED(hr)) {
        IShellItem* pItem = nullptr;
        hr = pFileDialog->GetResult(&pItem);

        if (SUCCEEDED(hr)) {
            PWSTR pszPath = nullptr;
            hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszPath);

            if (SUCCEEDED(hr)) {
                // Convert wide string to std::string
                int size = WideCharToMultiByte(CP_UTF8, 0, pszPath, -1, nullptr, 0, nullptr, nullptr);
                std::string selectedPath(size - 1, '\0');
                WideCharToMultiByte(CP_UTF8, 0, pszPath, -1, &selectedPath[0], size, nullptr, nullptr);

                CoTaskMemFree(pszPath);

                SDL_Log("Selected folder: %s", selectedPath.c_str());

                // Validate the folder contains game files
                std::vector<std::string> possiblePaths = {
                    selectedPath + "/SSGWINCD/GIZMO.DAT",
                    selectedPath + "/GIZMO.DAT",
                    selectedPath + "/SSGWINCD/GIZMO256.DAT",
                    selectedPath + "/GIZMO256.DAT",
                };

                bool found = false;
                for (const auto& testPath : possiblePaths) {
                    if (fs::exists(testPath)) {
                        found = true;
                        break;
                    }
                }

                if (found) {
                    // Valid game folder - update config
                    config_.gamePath = selectedPath;
                    saveConfig();

                    // Reinitialize asset cache with new path
                    if (assetCache_) {
                        assetCache_->initialize(config_.gamePath, config_.cachePath);
                        assetCache_->setRenderer(renderer_->getSDLRenderer());
                    }

                    SDL_Window* sdlWindow = renderer_ ? renderer_->getSDLWindow() : nullptr;
                    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, "Success",
                        ("Game files found!\n\nPath: " + selectedPath +
                         "\n\nThe game will now use these files.").c_str(), sdlWindow);

                    pItem->Release();
                    pFileDialog->Release();
                    return true;
                } else {
                    SDL_Window* sdlWindow = renderer_ ? renderer_->getSDLWindow() : nullptr;
                    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_WARNING, "Invalid Folder",
                        "Could not find Gizmos & Gadgets files in the selected folder.\n\n"
                        "Please select a folder containing GIZMO.DAT or the SSGWINCD subfolder.",
                        sdlWindow);
                }
            }
            pItem->Release();
        }
    }

    pFileDialog->Release();
    return false;
}

void Game::handleMenuCommand(int menuId) {
    switch (menuId) {
        // File menu
        case ID_FILE_SELECT_GAME:
            if (onNewGame_) onNewGame_();
            break;
        case ID_FILE_IMPORT_GAME:
            browseForGameFolder();
            break;
        case ID_FILE_SAVE:
            SDL_Log("Menu: Save");
            // TODO: Implement save
            break;
        case ID_FILE_SAVE_AS:
            SDL_Log("Menu: Save As");
            // TODO: Implement save as
            break;
        case ID_FILE_LOAD:
            SDL_Log("Menu: Load");
            // TODO: Implement load
            break;
        case ID_FILE_EXIT:
            quit();
            break;

        // Config menu
        case ID_CONFIG_LOAD_GG_FILES:
            browseForGameFolder();
            break;
        case ID_CONFIG_CONTROLS:
            SDL_Log("Menu: Controls");
            // TODO: Show controls dialog
            break;
        case ID_CONFIG_SCALING:
            SDL_Log("Menu: Scaling");
            // TODO: Show scaling options
            break;

        // Debug menu
        case ID_DEBUG_ASSET_VIEWER:
            if (!assetViewer_) {
                assetViewer_ = std::make_unique<AssetViewerWindow>();
            }
            assetViewer_->show(menuBar_->getHWND(), assetCache_.get(), renderer_->getSDLRenderer());
            break;
        case ID_DEBUG_MAP_VIEWER:
            SDL_Log("Menu: Map Viewer");
            // TODO: Open map viewer
            break;
        case ID_DEBUG_PUZZLE_DEBUGGER:
            SDL_Log("Menu: Puzzle Debugger");
            // TODO: Open puzzle debugger
            break;
        case ID_DEBUG_SAVE_EDITOR:
            SDL_Log("Menu: Save Editor");
            // TODO: Open save editor
            break;

        // Game launch
        case ID_DEBUG_LAUNCH_NEPTUNE:
            SDL_Log("Menu: Launch Operation Neptune");
            changeState(std::make_unique<NeptuneGameState>(this));
            break;
        case ID_DEBUG_LAUNCH_LABYRINTH:
            SDL_Log("Menu: Launch Labyrinth Test");
            pushState(std::make_unique<LabyrinthGameState>(this, 1));
            break;

        // Bot submenu
        case ID_DEBUG_BOT_ENABLE:
            SDL_Log("Menu: Enable Bot");
            Bot::BotManager::getInstance().setEnabled(true);
            break;
        case ID_DEBUG_BOT_DISABLE:
            SDL_Log("Menu: Disable Bot");
            Bot::BotManager::getInstance().setEnabled(false);
            break;
        case ID_DEBUG_BOT_MODE_OBSERVE:
            SDL_Log("Menu: Bot Mode - Observe");
            Bot::BotManager::getInstance().setMode(Bot::BotMode::Observe);
            break;
        case ID_DEBUG_BOT_MODE_ASSIST:
            SDL_Log("Menu: Bot Mode - Assist");
            Bot::BotManager::getInstance().setMode(Bot::BotMode::Assist);
            break;
        case ID_DEBUG_BOT_MODE_AUTOPLAY:
            SDL_Log("Menu: Bot Mode - AutoPlay");
            Bot::BotManager::getInstance().setMode(Bot::BotMode::AutoPlay);
            break;
        case ID_DEBUG_BOT_MODE_SPEEDRUN:
            SDL_Log("Menu: Bot Mode - SpeedRun");
            Bot::BotManager::getInstance().setMode(Bot::BotMode::SpeedRun);
            break;
        case ID_DEBUG_BOT_GAME_GIZMOS:
            SDL_Log("Menu: Bot Game - Gizmos & Gadgets");
            Bot::BotManager::getInstance().setGameType(Bot::GameType::GizmosAndGadgets);
            break;
        case ID_DEBUG_BOT_GAME_NEPTUNE:
            SDL_Log("Menu: Bot Game - Operation Neptune");
            Bot::BotManager::getInstance().setGameType(Bot::GameType::OperationNeptune);
            break;
        case ID_DEBUG_BOT_GAME_OUTNUMBERED:
            SDL_Log("Menu: Bot Game - OutNumbered!");
            Bot::BotManager::getInstance().setGameType(Bot::GameType::OutNumbered);
            break;
        case ID_DEBUG_BOT_GAME_SPELLBOUND:
            SDL_Log("Menu: Bot Game - Spellbound!");
            Bot::BotManager::getInstance().setGameType(Bot::GameType::Spellbound);
            break;
        case ID_DEBUG_BOT_GAME_TREASURE_MT:
            SDL_Log("Menu: Bot Game - Treasure Mountain!");
            Bot::BotManager::getInstance().setGameType(Bot::GameType::TreasureMountain);
            break;
        case ID_DEBUG_BOT_GAME_TREASURE_MS:
            SDL_Log("Menu: Bot Game - Treasure MathStorm!");
            Bot::BotManager::getInstance().setGameType(Bot::GameType::TreasureMathStorm);
            break;
        case ID_DEBUG_BOT_GAME_TREASURE_COVE:
            SDL_Log("Menu: Bot Game - Treasure Cove!");
            Bot::BotManager::getInstance().setGameType(Bot::GameType::TreasureCove);
            break;
        case ID_DEBUG_BOT_SHOW_STATUS:
            {
                SDL_Log("Menu: Show Bot Status");
                auto& botMgr = Bot::BotManager::getInstance();
                std::string status = botMgr.getStatusText();
                float progress = botMgr.getCompletionProgress();
                std::string message = "Bot Status:\n\n" + status +
                                      "\n\nCompletion: " + std::to_string(int(progress * 100)) + "%";
                SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, "Bot Status",
                    message.c_str(), renderer_ ? renderer_->getSDLWindow() : nullptr);
            }
            break;

        // About menu
        case ID_ABOUT_INFO:
            SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, "About OpenGG",
                "OpenGG v0.2.0\n\n"
                "A multi-game launcher for TLC Educational Games\n"
                "Supports: Gizmos & Gadgets, Operation Neptune,\n"
                "OutNumbered!, Spellbound!, Treasure Cove!,\n"
                "Treasure MathStorm!, Treasure Mountain!, and more.\n\n"
                "This is an open-source project that requires the original game files.\n"
                "No copyrighted assets are included.\n\n"
                "https://github.com/sp00nznet/OpenGG",
                renderer_ ? renderer_->getSDLWindow() : nullptr);
            break;

        default:
            SDL_Log("Unknown menu command: %d", menuId);
            break;
    }
}
#endif

} // namespace opengg
