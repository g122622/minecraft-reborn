#pragma once

#include "Screen.hpp"
#include <vector>
#include <memory>

namespace mc::client::ui::minecraft {

class ScreenManager {
public:
    void push(std::unique_ptr<Screen> screen);
    void pop();
    void clear();

    [[nodiscard]] Screen* top();
    [[nodiscard]] const Screen* top() const;

    void render(kagero::RenderContext& ctx, i32 mouseX, i32 mouseY, f32 partialTick);

private:
    std::vector<std::unique_ptr<Screen>> m_stack;
};

} // namespace mc::client::ui::minecraft
