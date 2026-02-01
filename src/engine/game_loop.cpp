#include "game_loop.h"
#include "renderer.h"
#include "audio.h"
#include "input.h"
#include "asset_cache.h"
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

    // Load config
    loadConfig();

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

    // Get current state
    GameState* state = getCurrentState();

    if (state) {
        // Handle input
        state->handleInput();

        // Update (unless paused)
        if (!paused_) {
            state->update(deltaTime_);
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

    // Clear state stack
    while (!stateStack_.empty()) {
        stateStack_.back()->exit();
        stateStack_.pop_back();
    }

    // Shutdown subsystems in reverse order
    assetCache_.reset();
    input_.reset();
    audio_.reset();
    renderer_.reset();

    SDL_Quit();
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
    // Common installation paths to check
    std::vector<std::string> searchPaths = {
        ".",
        "./SSGWIN32",
        "C:/GOG Games/Super Solvers Gizmos and Gadgets",
        "C:/Program Files (x86)/Steam/steamapps/common/Super Solvers Gizmos and Gadgets",
        "C:/Program Files/TLC/Gizmos & Gadgets",
    };

    // Key files that must exist
    std::vector<std::string> requiredFiles = {
        "SSGWIN32.EXE",
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

    // Game not found - show dialog
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_WARNING, "Game Not Found",
        "Could not locate Super Solvers: Gizmos & Gadgets installation.\n\n"
        "Please ensure you have a legitimate copy of the game installed,\n"
        "or specify the game path in the configuration file.",
        nullptr);

    return false;
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

    file << "# OpenGizmos Configuration\n\n";
    file << "fullscreen=" << (config_.fullscreen ? "true" : "false") << "\n";
    file << "vsync=" << (config_.vsync ? "true" : "false") << "\n";
    file << "gamePath=" << config_.gamePath << "\n";

    if (audio_) {
        file << "sfxVolume=" << audio_->getSFXVolume() << "\n";
        file << "musicVolume=" << audio_->getMusicVolume() << "\n";
    }

    return true;
}

} // namespace opengg
