#include "BlockModelLoader.hpp"
#include "../common/resource/IResourcePack.hpp"
#include <nlohmann/json.hpp>
#include <random>

namespace mr {

Direction parseDirection(StringView str) {
    auto result = Directions::fromName(str);
    return result.has_value() ? result.value() : Direction::None;
}

String directionToString(Direction dir) {
    if (dir == Direction::None) return "";
    return Directions::toString(dir);
}

ResourceLocation BakedBlockModel::resolveTexture(StringView textureRef) const {
    String ref(textureRef);

    // 移除前导 #
    if (!ref.empty() && ref[0] == '#') {
        ref = ref.substr(1);
