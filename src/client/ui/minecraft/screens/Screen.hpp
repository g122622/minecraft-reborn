#pragma once

#include "../../kagero/widget/ContainerWidget.hpp"

namespace mc::client::ui::minecraft {

class Screen : public kagero::widget::ContainerWidget {
public:
    explicit Screen(String id);

    virtual void onOpen();
    virtual void onClose();

    void render(kagero::RenderContext& ctx, i32 mouseX, i32 mouseY, f32 partialTick) override;

    [[nodiscard]] bool isModal() const;
    void setModal(bool modal);

private:
    bool m_modal = true;
};

} // namespace mc::client::ui::minecraft
