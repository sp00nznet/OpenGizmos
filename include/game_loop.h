#pragma once

#include <string>
#include <memory>
#include <functional>
#include <chrono>

namespace opengg {

// Forward declarations
class Renderer;
class AudioSystem;
class InputSystem;
class AssetCache;
class TextRenderer;
class MenuBar;
class AssetViewerWindow;
class GameRegistry;

// Game state interface
class GameState {
public:
    virtual ~GameState() = default;

    virtual void enter() = 0;
    virtual void exit() = 0;
    virtual void update(float dt) = 0;
    virtual void render() = 0;
    virtual void handleInput() = 0;
};

// Main game loop configuration
struct GameConfig {
    std::string windowTitle = "OpenGizmos";
    int windowWidth = 0;   // 0 = auto (2x game resolution)
    int windowHeight = 0;
    bool fullscreen = false;
    bool vsync = true;
    int targetFPS = 60;
    std::string gamePath;   // Path to original game
    std::string cachePath;  // Path for asset cache
    std::string configPath; // Path for config files
};

// Main game class
class Game {
public:
    Game();
    ~Game();

    // Initialize game
    bool initialize(const GameConfig& config);

    // Run main loop (blocks until quit)
    void run();

    // Shutdown
    void shutdown();

    // State management
    void pushState(std::unique_ptr<GameState> state);
    void popState();
    void changeState(std::unique_ptr<GameState> state);
    GameState* getCurrentState();

    // Get subsystems
    Renderer* getRenderer() { return renderer_.get(); }
    AudioSystem* getAudio() { return audio_.get(); }
    InputSystem* getInput() { return input_.get(); }
    AssetCache* getAssetCache() { return assetCache_.get(); }
    TextRenderer* getTextRenderer() { return textRenderer_.get(); }
    GameRegistry* getGameRegistry() { return gameRegistry_.get(); }
#ifdef _WIN32
    MenuBar* getMenuBar() { return menuBar_.get(); }
#endif

    // Game control
    void quit() { running_ = false; }
    bool isRunning() const { return running_; }

    // Timing
    float getDeltaTime() const { return deltaTime_; }
    float getFPS() const { return fps_; }
    uint64_t getFrameCount() const { return frameCount_; }
    double getElapsedTime() const;

    // Config
    const GameConfig& getConfig() const { return config_; }

    // Pause
    void setPaused(bool paused) { paused_ = paused; }
    bool isPaused() const { return paused_; }

    // Menu callbacks
    void setNewGameCallback(std::function<void()> callback) { onNewGame_ = callback; }
    void setAssetViewerCallback(std::function<void()> callback) { onAssetViewer_ = callback; }

    // Import game data (opens native folder picker)
#ifdef _WIN32
    bool browseForGameFolder();
#else
    bool browseForGameFolder() { return false; }
#endif

private:
    void processFrame();
    void updateTiming();
    bool detectGame();
    bool loadConfig();
    bool saveConfig();
#ifdef _WIN32
    void handleMenuCommand(int menuId);
#endif

    GameConfig config_;

    // Subsystems
    std::unique_ptr<Renderer> renderer_;
    std::unique_ptr<AudioSystem> audio_;
    std::unique_ptr<InputSystem> input_;
    std::unique_ptr<AssetCache> assetCache_;
    std::unique_ptr<TextRenderer> textRenderer_;
    std::unique_ptr<GameRegistry> gameRegistry_;
#ifdef _WIN32
    std::unique_ptr<MenuBar> menuBar_;
    std::unique_ptr<AssetViewerWindow> assetViewer_;
#endif

    // State stack
    std::vector<std::unique_ptr<GameState>> stateStack_;

    // Timing
    using Clock = std::chrono::high_resolution_clock;
    using TimePoint = std::chrono::time_point<Clock>;

    TimePoint startTime_;
    TimePoint lastFrameTime_;
    float deltaTime_ = 0.0f;
    float fps_ = 0.0f;
    float fpsAccumulator_ = 0.0f;
    int fpsFrameCount_ = 0;
    uint64_t frameCount_ = 0;

    // Control
    bool running_ = false;
    bool paused_ = false;

    // Menu callbacks
    std::function<void()> onNewGame_;
    std::function<void()> onAssetViewer_;
};

} // namespace opengg
