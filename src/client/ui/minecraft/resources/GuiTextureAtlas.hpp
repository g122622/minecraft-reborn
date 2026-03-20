#pragma once

#include "GuiSprite.hpp"
#include <unordered_map>

namespace mc::client::ui::minecraft {

/**
 * @brief GUI纹理图集
 *
 * 管理精灵图集，不再依赖 IImage 接口。
 * 精灵的UV坐标直接存储在 GuiSprite 中。
 */
class GuiTextureAtlas {
public:
    /**
     * @brief 添加精灵
     */
    void addSprite(GuiSprite sprite);

    /**
     * @brief 查找精灵
     * @param id 精灵ID
     * @return 精灵指针，未找到返回 nullptr
     */
    [[nodiscard]] const GuiSprite* findSprite(const String& id) const;

    /**
     * @brief 检查是否为空
     */
    [[nodiscard]] bool empty() const { return m_sprites.empty(); }

    /**
     * @brief 获取精灵数量
     */
    [[nodiscard]] size_t size() const { return m_sprites.size(); }

private:
    std::unordered_map<String, GuiSprite> m_sprites;
};

} // namespace mc::client::ui::minecraft
