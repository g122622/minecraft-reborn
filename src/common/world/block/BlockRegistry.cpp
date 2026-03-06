#include "BlockRegistry.hpp"

namespace mr {

BlockRegistry& BlockRegistry::instance() {
    static BlockRegistry instance;
    return instance;
}

} // namespace mr
