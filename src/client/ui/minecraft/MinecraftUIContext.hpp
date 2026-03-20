#pragma once

#include "../kagero/backend/IRenderBackend.hpp"
#include "../kagero/state/StateStore.hpp"
#include "../kagero/event/EventBus.hpp"
#include "../kagero/template/binder/BindingContext.hpp"
#include "../kagero/template/compiler/TemplateCompiler.hpp"
#include "../kagero/template/runtime/TemplateInstance.hpp"
#include "resources/ResourceProvider.hpp"
#include <memory>

namespace mc::client::ui::minecraft {

/**
 * @brief Minecraft UI业务上下文
 */
class MinecraftUIContext {
public:
    MinecraftUIContext(
        kagero::backend::IRenderBackend& backend,
        kagero::state::StateStore& stateStore,
        kagero::event::EventBus& eventBus
    );

    [[nodiscard]] std::unique_ptr<kagero::tpl::runtime::TemplateInstance> createScreen(const String& templatePath);

    [[nodiscard]] kagero::tpl::binder::BindingContext& bindingContext();
    [[nodiscard]] const kagero::tpl::binder::BindingContext& bindingContext() const;

private:
    void setupStateBindings();
    void setupDefaultResources();

    kagero::backend::IRenderBackend& m_backend;
    kagero::state::StateStore& m_stateStore;
    kagero::event::EventBus& m_eventBus;
    kagero::tpl::binder::BindingContext m_bindingContext;
    ResourceProvider m_resources;

    i32 m_playerHealth = 20;
    i32 m_playerHunger = 20;
    i32 m_playerXP = 0;
    String m_playerName = "Steve";
};

} // namespace mc::client::ui::minecraft
