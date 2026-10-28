#include <tavoos/clipboard.h>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

namespace Tavoos {

std::string Clipboard::text() {
    const char* const value = glfwGetClipboardString(nullptr);
    return value ? value : std::string{};
}

void Clipboard::setText(const std::string& text) {
    glfwSetClipboardString(nullptr, text.c_str());
}

}
