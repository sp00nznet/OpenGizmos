#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <cstdint>

union SDL_Event;

namespace opengg {

// Game actions that can be bound to keys
enum class GameAction {
    None = 0,

    // Movement
    MoveLeft,
    MoveRight,
    MoveUp,
    MoveDown,
    Jump,
    Climb,

    // Actions
    Action,       // Generic action/interact
    Cancel,       // Cancel/back
    Pause,        // Pause menu
    Inventory,    // Open inventory/parts

    // UI
    MenuUp,
    MenuDown,
    MenuLeft,
    MenuRight,
    MenuSelect,
    MenuBack,

    // Debug
    DebugToggle,
    Screenshot,

    Count
};

// Mouse button IDs
enum class MouseButton {
    Left = 0,
    Middle = 1,
    Right = 2
};

// Input event types
enum class InputEventType {
    KeyDown,
    KeyUp,
    MouseButtonDown,
    MouseButtonUp,
    MouseMove,
    MouseWheel,
    ActionPressed,
    ActionReleased
};

// Input event data
struct InputEvent {
    InputEventType type;
    int key;              // SDL_Scancode for key events
    int mouseButton;      // For mouse button events
    int mouseX, mouseY;   // Mouse position
    int wheelX, wheelY;   // Mouse wheel delta
    GameAction action;    // For action events
};

// Callback type for input events
using InputCallback = std::function<void(const InputEvent&)>;

// Input System
// Handles keyboard and mouse input, with action mapping
class InputSystem {
public:
    InputSystem();
    ~InputSystem();

    // Process SDL events (call each frame)
    void processEvents();

    // Check if should quit
    bool shouldQuit() const { return quit_; }
    void requestQuit() { quit_ = true; }

    // Key state queries
    bool isKeyDown(int scancode) const;
    bool isKeyPressed(int scancode) const;  // Just pressed this frame
    bool isKeyReleased(int scancode) const; // Just released this frame

    // Action state queries (preferred over raw keys)
    bool isActionDown(GameAction action) const;
    bool isActionPressed(GameAction action) const;
    bool isActionReleased(GameAction action) const;

    // Mouse state
    int getMouseX() const { return mouseX_; }
    int getMouseY() const { return mouseY_; }
    bool isMouseButtonDown(MouseButton button) const;
    bool isMouseButtonPressed(MouseButton button) const;
    bool isMouseButtonReleased(MouseButton button) const;
    int getMouseWheelDelta() const { return wheelDelta_; }

    // Key binding
    void bindKey(int scancode, GameAction action);
    void unbindKey(int scancode);
    void unbindAction(GameAction action);
    GameAction getKeyBinding(int scancode) const;
    std::vector<int> getActionKeys(GameAction action) const;

    // Load/save key bindings
    bool loadBindings(const std::string& path);
    bool saveBindings(const std::string& path) const;
    void resetToDefaults();

    // Event callbacks
    void setEventCallback(InputCallback callback);
    void clearEventCallback();

    // Text input mode (for name entry, etc.)
    void startTextInput();
    void stopTextInput();
    bool isTextInputActive() const { return textInputActive_; }
    const std::string& getTextInput() const { return textBuffer_; }
    void clearTextInput();

    // Update (call at end of frame to update previous state)
    void endFrame();

    // Get action name for display
    static std::string getActionName(GameAction action);
    static std::string getKeyName(int scancode);

private:
    void handleKeyDown(const SDL_Event& event);
    void handleKeyUp(const SDL_Event& event);
    void handleMouseButton(const SDL_Event& event);
    void handleMouseMotion(const SDL_Event& event);
    void handleMouseWheel(const SDL_Event& event);
    void handleTextInput(const SDL_Event& event);
    void fireEvent(const InputEvent& event);

    // Key states (indexed by SDL_Scancode)
    std::vector<bool> keyState_;
    std::vector<bool> prevKeyState_;

    // Mouse state
    int mouseX_ = 0;
    int mouseY_ = 0;
    uint8_t mouseButtonState_ = 0;
    uint8_t prevMouseButtonState_ = 0;
    int wheelDelta_ = 0;

    // Action states
    std::vector<bool> actionState_;
    std::vector<bool> prevActionState_;

    // Key to action mapping
    std::unordered_map<int, GameAction> keyBindings_;

    // Event callback
    InputCallback eventCallback_;

    // Text input
    bool textInputActive_ = false;
    std::string textBuffer_;

    // Quit flag
    bool quit_ = false;
};

} // namespace opengg
