#include "UIPipeline.hpp"

namespace mc::client::ui::kagero::backends::trident::pipelines {

void UIPipeline::beginFrame(i32 width, i32 height) {
    m_width = width;
    m_height = height;
}

void UIPipeline::endFrame() {
    // 当前版本为占位实现，后续接入Trident Vulkan命令提交流程。
}

} // namespace mc::client::ui::kagero::backends::trident::pipelines
