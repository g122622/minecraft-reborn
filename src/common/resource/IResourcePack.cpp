#include "IResourcePack.hpp"

namespace mc {

Result<String> IResourcePack::readTextResource(StringView resourcePath) const {
    auto result = readResource(resourcePath);
    if (result.failed()) {
        return result.error();
    }

    const auto& data = result.value();
    return String(data.begin(), data.end());
}

} // namespace mc
