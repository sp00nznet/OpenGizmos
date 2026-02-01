#include "input.h"
#include <SDL.h>
#include <fstream>
#include <sstream>
#include <algorithm>

namespace opengg {

InputSystem::InputSystem() {
    // Initialize key state arrays (512 scancodes should be enough)
    keyState_.resize(512, false);
    prevKeyState_.resize(512, false);

    // Initialize action states
    actionState_.resize(static_cast<size_t>(GameAction::Count), false);
    prevActionState_.resize(static_cast<size_t>(GameAction::Count), false);

    // Set default bindings
    resetToDefaults();
}

InputSystem::~InputSystem() {
    if (textInputActive_) {
        SDL_StopTextInput();
    }
}

void InputSystem::resetToDefaults() {
    keyBindings_.clear();

    // Movement
    bindKey(SDL_SCANCODE_LEFT, GameAction::MoveLeft);
    bindKey(SDL_SCANCODE_A, GameAction::MoveLeft);
    bindKey(SDL_SCANCODE_RIGHT, GameAction::MoveRight);
    bindKey(SDL_SCANCODE_D, GameAction::MoveRight);
    bindKey(SDL_SCANCODE_UP, GameAction::MoveUp);
    bindKey(SDL_SCANCODE_W, GameAction::MoveUp);
    bindKey(SDL_SCANCODE_DOWN, GameAction::MoveDown);
    bindKey(SDL_SCANCODE_S, GameAction::MoveDown);
    bindKey(SDL_SCANCODE_SPACE, GameAction::Jump);
    bindKey(SDL_SCANCODE_LSHIFT, GameAction::Climb);

    // Actions
    bindKey(SDL_SCANCODE_RETURN, GameAction::Action);
    bindKey(SDL_SCANCODE_E, GameAction::Action);
    bindKey(SDL_SCANCODE_ESCAPE, GameAction::Cancel);
    bindKey(SDL_SCANCODE_P, GameAction::Pause);
    bindKey(SDL_SCANCODE_I, GameAction::Inventory);
    bindKey(SDL_SCANCODE_TAB, GameAction::Inventory);

    // Menu navigation (same as movement by default)
    bindKey(SDL_SCANCODE_UP, GameAction::MenuUp);
    bindKey(SDL_SCANCODE_DOWN, GameAction::MenuDown);
    bindKey(SDL_SCANCODE_LEFT, GameAction::MenuLeft);
    bindKey(SDL_SCANCODE_RIGHT, GameAction::MenuRight);
    bindKey(SDL_SCANCODE_RETURN, GameAction::MenuSelect);
    bindKey(SDL_SCANCODE_ESCAPE, GameAction::MenuBack);

    // Debug
    bindKey(SDL_SCANCODE_F1, GameAction::DebugToggle);
    bindKey(SDL_SCANCODE_F12, GameAction::Screenshot);
}

void InputSystem::processEvents() {
    // Reset per-frame state
    wheelDelta_ = 0;

    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_QUIT:
                quit_ = true;
                break;

            case SDL_KEYDOWN:
                handleKeyDown(event);
                break;

            case SDL_KEYUP:
                handleKeyUp(event);
                break;

            case SDL_MOUSEBUTTONDOWN:
            case SDL_MOUSEBUTTONUP:
                handleMouseButton(event);
                break;

            case SDL_MOUSEMOTION:
                handleMouseMotion(event);
                break;

            case SDL_MOUSEWHEEL:
                handleMouseWheel(event);
                break;

            case SDL_TEXTINPUT:
                if (textInputActive_) {
                    handleTextInput(event);
                }
                break;

            case SDL_WINDOWEVENT:
                // Handle window focus, resize, etc.
                break;
        }
    }
}

void InputSystem::handleKeyDown(const SDL_Event& event) {
    int scancode = event.key.keysym.scancode;
    if (scancode < 0 || scancode >= static_cast<int>(keyState_.size())) return;

    // Ignore key repeat
    if (event.key.repeat) return;

    keyState_[scancode] = true;

    // Fire key event
    InputEvent inputEvent;
    inputEvent.type = InputEventType::KeyDown;
    inputEvent.key = scancode;
    inputEvent.action = GameAction::None;
    fireEvent(inputEvent);

    // Check for bound action
    auto it = keyBindings_.find(scancode);
    if (it != keyBindings_.end()) {
        GameAction action = it->second;
        actionState_[static_cast<size_t>(action)] = true;

        InputEvent actionEvent;
        actionEvent.type = InputEventType::ActionPressed;
        actionEvent.key = scancode;
        actionEvent.action = action;
        fireEvent(actionEvent);
    }
}

void InputSystem::handleKeyUp(const SDL_Event& event) {
    int scancode = event.key.keysym.scancode;
    if (scancode < 0 || scancode >= static_cast<int>(keyState_.size())) return;

    keyState_[scancode] = false;

    // Fire key event
    InputEvent inputEvent;
    inputEvent.type = InputEventType::KeyUp;
    inputEvent.key = scancode;
    inputEvent.action = GameAction::None;
    fireEvent(inputEvent);

    // Check for bound action
    auto it = keyBindings_.find(scancode);
    if (it != keyBindings_.end()) {
        GameAction action = it->second;

        // Only release action if no other key for this action is pressed
        bool stillPressed = false;
        for (const auto& binding : keyBindings_) {
            if (binding.second == action && binding.first != scancode) {
                if (keyState_[binding.first]) {
                    stillPressed = true;
                    break;
                }
            }
        }

        if (!stillPressed) {
            actionState_[static_cast<size_t>(action)] = false;

            InputEvent actionEvent;
            actionEvent.type = InputEventType::ActionReleased;
            actionEvent.key = scancode;
            actionEvent.action = action;
            fireEvent(actionEvent);
        }
    }
}

void InputSystem::handleMouseButton(const SDL_Event& event) {
    bool down = (event.type == SDL_MOUSEBUTTONDOWN);
    int button = event.button.button - 1;  // SDL buttons are 1-indexed

    if (button >= 0 && button < 8) {
        if (down) {
            mouseButtonState_ |= (1 << button);
        } else {
            mouseButtonState_ &= ~(1 << button);
        }
    }

    InputEvent inputEvent;
    inputEvent.type = down ? InputEventType::MouseButtonDown : InputEventType::MouseButtonUp;
    inputEvent.mouseButton = button;
    inputEvent.mouseX = event.button.x;
    inputEvent.mouseY = event.button.y;
    fireEvent(inputEvent);
}

void InputSystem::handleMouseMotion(const SDL_Event& event) {
    mouseX_ = event.motion.x;
    mouseY_ = event.motion.y;

    InputEvent inputEvent;
    inputEvent.type = InputEventType::MouseMove;
    inputEvent.mouseX = mouseX_;
    inputEvent.mouseY = mouseY_;
    fireEvent(inputEvent);
}

void InputSystem::handleMouseWheel(const SDL_Event& event) {
    wheelDelta_ = event.wheel.y;

    InputEvent inputEvent;
    inputEvent.type = InputEventType::MouseWheel;
    inputEvent.wheelX = event.wheel.x;
    inputEvent.wheelY = event.wheel.y;
    fireEvent(inputEvent);
}

void InputSystem::handleTextInput(const SDL_Event& event) {
    textBuffer_ += event.text.text;
}

void InputSystem::fireEvent(const InputEvent& event) {
    if (eventCallback_) {
        eventCallback_(event);
    }
}

void InputSystem::endFrame() {
    prevKeyState_ = keyState_;
    prevMouseButtonState_ = mouseButtonState_;
    prevActionState_ = actionState_;
}

bool InputSystem::isKeyDown(int scancode) const {
    if (scancode < 0 || scancode >= static_cast<int>(keyState_.size())) return false;
    return keyState_[scancode];
}

bool InputSystem::isKeyPressed(int scancode) const {
    if (scancode < 0 || scancode >= static_cast<int>(keyState_.size())) return false;
    return keyState_[scancode] && !prevKeyState_[scancode];
}

bool InputSystem::isKeyReleased(int scancode) const {
    if (scancode < 0 || scancode >= static_cast<int>(keyState_.size())) return false;
    return !keyState_[scancode] && prevKeyState_[scancode];
}

bool InputSystem::isActionDown(GameAction action) const {
    size_t idx = static_cast<size_t>(action);
    if (idx >= actionState_.size()) return false;
    return actionState_[idx];
}

bool InputSystem::isActionPressed(GameAction action) const {
    size_t idx = static_cast<size_t>(action);
    if (idx >= actionState_.size()) return false;
    return actionState_[idx] && !prevActionState_[idx];
}

bool InputSystem::isActionReleased(GameAction action) const {
    size_t idx = static_cast<size_t>(action);
    if (idx >= actionState_.size()) return false;
    return !actionState_[idx] && prevActionState_[idx];
}

bool InputSystem::isMouseButtonDown(MouseButton button) const {
    return (mouseButtonState_ & (1 << static_cast<int>(button))) != 0;
}

bool InputSystem::isMouseButtonPressed(MouseButton button) const {
    int mask = 1 << static_cast<int>(button);
    return (mouseButtonState_ & mask) && !(prevMouseButtonState_ & mask);
}

bool InputSystem::isMouseButtonReleased(MouseButton button) const {
    int mask = 1 << static_cast<int>(button);
    return !(mouseButtonState_ & mask) && (prevMouseButtonState_ & mask);
}

void InputSystem::bindKey(int scancode, GameAction action) {
    keyBindings_[scancode] = action;
}

void InputSystem::unbindKey(int scancode) {
    keyBindings_.erase(scancode);
}

void InputSystem::unbindAction(GameAction action) {
    for (auto it = keyBindings_.begin(); it != keyBindings_.end(); ) {
        if (it->second == action) {
            it = keyBindings_.erase(it);
        } else {
            ++it;
        }
    }
}

GameAction InputSystem::getKeyBinding(int scancode) const {
    auto it = keyBindings_.find(scancode);
    if (it != keyBindings_.end()) {
        return it->second;
    }
    return GameAction::None;
}

std::vector<int> InputSystem::getActionKeys(GameAction action) const {
    std::vector<int> keys;
    for (const auto& binding : keyBindings_) {
        if (binding.second == action) {
            keys.push_back(binding.first);
        }
    }
    return keys;
}

bool InputSystem::loadBindings(const std::string& path) {
    std::ifstream file(path);
    if (!file) return false;

    keyBindings_.clear();

    std::string line;
    while (std::getline(file, line)) {
        // Skip comments and empty lines
        if (line.empty() || line[0] == '#') continue;

        std::istringstream ss(line);
        int scancode;
        int actionInt;

        if (ss >> scancode >> actionInt) {
            if (actionInt >= 0 && actionInt < static_cast<int>(GameAction::Count)) {
                keyBindings_[scancode] = static_cast<GameAction>(actionInt);
            }
        }
    }

    return true;
}

bool InputSystem::saveBindings(const std::string& path) const {
    std::ofstream file(path);
    if (!file) return false;

    file << "# OpenGizmos Key Bindings\n";
    file << "# Format: scancode action\n\n";

    for (const auto& binding : keyBindings_) {
        file << binding.first << " " << static_cast<int>(binding.second) << "\n";
    }

    return true;
}

void InputSystem::setEventCallback(InputCallback callback) {
    eventCallback_ = std::move(callback);
}

void InputSystem::clearEventCallback() {
    eventCallback_ = nullptr;
}

void InputSystem::startTextInput() {
    textInputActive_ = true;
    textBuffer_.clear();
    SDL_StartTextInput();
}

void InputSystem::stopTextInput() {
    textInputActive_ = false;
    SDL_StopTextInput();
}

void InputSystem::clearTextInput() {
    textBuffer_.clear();
}

std::string InputSystem::getActionName(GameAction action) {
    switch (action) {
        case GameAction::MoveLeft:    return "Move Left";
        case GameAction::MoveRight:   return "Move Right";
        case GameAction::MoveUp:      return "Move Up";
        case GameAction::MoveDown:    return "Move Down";
        case GameAction::Jump:        return "Jump";
        case GameAction::Climb:       return "Climb";
        case GameAction::Action:      return "Action";
        case GameAction::Cancel:      return "Cancel";
        case GameAction::Pause:       return "Pause";
        case GameAction::Inventory:   return "Inventory";
        case GameAction::MenuUp:      return "Menu Up";
        case GameAction::MenuDown:    return "Menu Down";
        case GameAction::MenuLeft:    return "Menu Left";
        case GameAction::MenuRight:   return "Menu Right";
        case GameAction::MenuSelect:  return "Menu Select";
        case GameAction::MenuBack:    return "Menu Back";
        case GameAction::DebugToggle: return "Debug Toggle";
        case GameAction::Screenshot:  return "Screenshot";
        default:                      return "Unknown";
    }
}

std::string InputSystem::getKeyName(int scancode) {
    const char* name = SDL_GetScancodeName(static_cast<SDL_Scancode>(scancode));
    if (name && name[0]) {
        return name;
    }
    return "Key " + std::to_string(scancode);
}

} // namespace opengg
