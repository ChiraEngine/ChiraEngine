#include "FramePanel.h"

#include <entity/root/Frame.h>

using namespace chira;

FramePanel::FramePanel(const std::string& title_, Frame* frame_, bool startVisible, ImVec2 windowSize, bool enforceSize)
    : IPanel(title_, startVisible, windowSize, enforceSize)
    , frame(frame_)
    , currentSize(windowSize.x, windowSize.y) {}

void FramePanel::renderContents() {
    if (ImGui::BeginChild("__internal_frame__")) {
        ImVec2 guiSize = ImGui::GetWindowSize();
        glm::vec2i size{guiSize.x, guiSize.y};
        if (this->currentSize != size) {
            this->frame->setFrameSize(size);
            this->currentSize = size;
        }
        ImGui::Image(Renderer::getImGuiFrameBufferHandle(*this->frame->getRawHandle()), guiSize, ImVec2(0, 1), ImVec2(1, 0));
    }
    ImGui::EndChild();
}
