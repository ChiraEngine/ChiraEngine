#pragma once

#include <unordered_map>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <imgui.h>

#include <utility/UUIDGenerator.h>
#include "Frame.h"

namespace chira {

class IPanel;

class Window : public Frame {
    // We don't want people making windows and adding them to the entity tree
    // Only the Engine can make windows!
    friend class Engine;
public:
    void render(glm::mat4 parentTransform) override;
    ~Window() override;
    uuids::uuid addPanel(IPanel* panel);
    [[nodiscard]] IPanel* getPanel(const uuids::uuid& panelID);
    void removePanel(const uuids::uuid& panelID);
    void removeAllPanels();
    void setFrameSize(glm::vec2i newSize) override;
    [[nodiscard]] glm::vec2d getMousePosition() const;
    void captureMouse(bool capture);
    [[nodiscard]] bool isMouseCaptured() const;
    [[nodiscard]] bool isIconified() const;
    void setVisible(bool visibility) override;
    void setFullscreen(bool goFullscreen) const;
    [[nodiscard]] bool isFullscreen() const;
    void setMaximized(bool maximize);
    [[nodiscard]] bool isMaximized() const;
    void moveToPosition(glm::vec2i pos) const;
    void moveToCenter() const;
    /// Note: Images must have a bit depth of 8
    void setIcon(const std::string& identifier) const;
    void shouldStopAfterThisFrame(bool yes = true) const;
    /// Renders the splashscreen to all window's default framebuffer
    void displaySplashScreen();
private:
    MeshDataBuilder surface;
    GLFWwindow* window = nullptr;
    ImGuiContext* imguiContext = nullptr;
    bool mouseCaptured = false, iconified = false;
    double lastMouseX = -1.0, lastMouseY = -1.0;
    std::unordered_map<uuids::uuid, IPanel*> panels{};

    Window(std::string name_, std::string_view title);
    explicit Window(std::string_view title);

    bool createGLFWWindow(std::string_view title);
    static void setImGuiConfigPath();
};

} // namespace chira
