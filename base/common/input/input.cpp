#include <common/input/input.h>
#include <common/window.h>
#include <common/event/keyboard_event.h>
#include <common/event/mouse_event.h>

#include <format>
#include <iostream>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

static std::size_t getIndex(KeyCode keycode) noexcept {
    return static_cast<std::size_t>(keycode);
}

static std::size_t getIndex(MouseButton button) noexcept {
    return static_cast<std::size_t>(button);
}

Input::Input(Window& window)
    : m_window(window)
    , m_mousePosition{0.0f, 0.0f}
    , m_mouseScroll{0.0f, 0.0f} {
    m_keyStates.fill(KeyState::None);
    m_mouseButtonStates.fill(MouseButtonState::None);
}

bool Input::isKeyPressed(KeyCode keycode) const noexcept {
    return m_keyStates[getIndex(keycode)] == KeyState::Pressed;
}

bool Input::isKeyHeld(KeyCode keycode) const noexcept {
    return m_keyStates[getIndex(keycode)] == KeyState::Held;
}

bool Input::isKeyReleased(KeyCode keycode) const noexcept {
    return m_keyStates[getIndex(keycode)] == KeyState::Released;
}

bool Input::isKeyDown(KeyCode keycode) const noexcept {
    return isKeyPressed(keycode) || isKeyHeld(keycode);
}

bool Input::isMouseButtonPressed(MouseButton button) const noexcept {
    return m_mouseButtonStates[getIndex(button)] == MouseButtonState::Pressed;
}

bool Input::isMouseButtonHeld(MouseButton button) const noexcept {
    return m_mouseButtonStates[getIndex(button)] == MouseButtonState::Held;
}

bool Input::isMouseButtonDown(MouseButton button) const noexcept {
    return isMouseButtonPressed(button) || isMouseButtonHeld(button);
}

float Input::getMouseX() const noexcept {
    return getMousePosition().first;
}

float Input::getMouseY() const noexcept {
    return getMousePosition().second;
}

std::pair<float, float> Input::getMousePosition() const noexcept {
    return m_mousePosition;
}

void Input::setMousePosition(float x, float y) {
    auto* window{static_cast<GLFWwindow*>(m_window.handle())};
    if (window != nullptr) {
        glfwSetCursorPos(window, x, y);
    }
    m_mousePosition = {x, y};
}

float Input::getMouseScrollX() const noexcept {
    return getMouseScroll().first;
}

float Input::getMouseScrollY() const noexcept {
    return getMouseScroll().second;
}

std::pair<float, float> Input::getMouseScroll() const noexcept {
    return m_mouseScroll;
}

CursorMode Input::getCursorMode() const {
    auto* window{static_cast<GLFWwindow*>(m_window.handle())};
    if (window == nullptr) {
        return CursorMode::Normal;
    }

    switch (glfwGetInputMode(window, GLFW_CURSOR)) {
    case GLFW_CURSOR_HIDDEN:
        return CursorMode::Hidden;
    case GLFW_CURSOR_DISABLED:
        return CursorMode::Disabled;
    }

    return CursorMode::Normal;
}

void Input::setCursorMode(CursorMode mode) {
    auto* window{static_cast<GLFWwindow*>(m_window.handle())};
    if (window == nullptr) {
        return;
    }

    switch (mode) {
    case CursorMode::Normal:
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        break;
    case CursorMode::Hidden:
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
        break;
    case CursorMode::Disabled:
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        break;
    }
}

void Input::printKeyStates() const {
    for (std::size_t i = 0; i < m_keyStates.size(); ++i) {
        if (m_keyStates[i] != KeyState::None) {
            std::cout << std::format(
                "KeyCode {} ({}) state {} ({})\n",
                i,
                toString(static_cast<KeyCode>(i)),
                static_cast<int>(m_keyStates[i]),
                toString(m_keyStates[i]));
        }
    }
}

void Input::printMouseButtonStates() const {
    for (std::size_t i = 0; i < m_mouseButtonStates.size(); ++i) {
        if (m_mouseButtonStates[i] != MouseButtonState::None) {
            std::cout << std::format(
                "MouseButton {} ({}) state {} ({})\n",
                i,
                toString(static_cast<MouseButton>(i)),
                static_cast<int>(m_mouseButtonStates[i]),
                toString(m_mouseButtonStates[i]));
        }
    }
}

void Input::processEvent(Event& event) noexcept {
    if (event.isInCategory(Event::Category::Keyboard)) {
        // filter key type event
        if (event.getType() == Event::Type::KeyType) {
            return;
        }

        KeyState state{KeyState::None};
        switch (event.getType()) {
        case Event::Type::KeyPress:
            state = static_cast<KeyPressEvent const&>(event).isRepeated() ? KeyState::Held : KeyState::Pressed;
            break;
        case Event::Type::KeyRelease:
            state = KeyState::Released;
            break;
        case Event::Type::KeyType:
            state = KeyState::Pressed;
            break;
        default:
            break;
        }

        updateKeyState(static_cast<KeyboardEvent const&>(event).getKeyCode(), state);
    }
    else if (event.isInCategory(Event::Category::Mouse)) {
        switch (event.getType()) {
        case Event::Type::MouseMove:
            m_mousePosition = static_cast<MouseMoveEvent const&>(event).getPosition();
            break;
        case Event::Type::MouseScroll:
            m_mouseScroll = static_cast<MouseScrollEvent const&>(event).getScroll();
            break;
        default:
            break;
        }
    }
    else if (event.isInCategory(Event::Category::MouseButton)) {
        MouseButtonState state{MouseButtonState::None};
        MouseButton button{MouseButton::Unknown};
        switch (event.getType()) {
        case Event::Type::MouseButtonPress:
            state = MouseButtonState::Pressed;
            button = static_cast<MouseButtonPressEvent const&>(event).getButton();
            break;
        case Event::Type::MouseButtonHold:
            state = MouseButtonState::Held;
            button = static_cast<MouseButtonHoldEvent const&>(event).getButton();
            break;
        case Event::Type::MouseButtonRelease:
            state = MouseButtonState::Released;
            button = static_cast<MouseButtonReleaseEvent const&>(event).getButton();
            break;
        default:
            break;
        }

        if (state != MouseButtonState::None) {
            updateMouseButtonState(button, state);
        }
    }
}

void Input::updateKeyState(KeyCode keycode, KeyState state) noexcept {
    m_keyStates[getIndex(keycode)] = state;
}

void Input::forwardKeyStates() noexcept {
    for (auto& state : m_keyStates) {
        if (state == KeyState::Pressed) {
            state = KeyState::Held;
        }
        else if (state == KeyState::Released) {
            state = KeyState::None;
        }
    }
}

void Input::updateMouseButtonState(MouseButton button, MouseButtonState state) noexcept {
    m_mouseButtonStates[getIndex(button)] = state;
}

void Input::forwardMouseStates() noexcept {
    m_mouseScroll = {0.0f, 0.0f};

    for (auto& state : m_mouseButtonStates) {
        if (state == MouseButtonState::Pressed) {
            state = MouseButtonState::Held;
        }
        else if (state == MouseButtonState::Released) {
            state = MouseButtonState::None;
        }
    }
}
