#include "ResourcePackList.hpp"

#include <spdlog/spdlog.h>
#include <algorithm>
#include <set>

namespace mc {

// ============================================================================
// 资源包管理
// ============================================================================

Result<size_t> ResourcePackList::scanDirectory(const std::filesystem::path& dir)
{
    if (!std::filesystem::exists(dir)) {
        spdlog::debug("Resource pack directory does not exist: {}", dir.string());
        return static_cast<size_t>(0);
    }

    if (!std::filesystem::is_directory(dir)) {
        return Error(ErrorCode::InvalidArgument,
                     "Path is not a directory: " + dir.string());
    }

    size_t addedCount = 0;
    i32 nextPriority = static_cast<i32>(m_packs.size());

    // 遍历目录
    for (const auto& entry : std::filesystem::directory_iterator(dir)) {
        const auto& path = entry.path();

        // 跳过已存在的资源包
        String normalizedPath = normalizePath(path);
        if (findPack(normalizedPath) != nullptr) {
            continue;
        }

        // 检查是否是 ZIP 文件或资源包目录
        bool isZip = isZipFile(path);
        bool isPackDir = !isZip && isResourcePackDir(path);

        if (!isZip && !isPackDir) {
            continue;
        }

        // 添加资源包
        auto result = addPack(path, true, nextPriority++);
        if (result.success()) {
            ++addedCount;
            if (result.value().initialized) {
                spdlog::info("Found resource pack: {} ({})",
                             result.value().pack->name(),
                             isZip ? "ZIP" : "folder");
            } else {
                spdlog::warn("Resource pack failed to initialize: {} - {}",
                             path.filename().string(),
                             result.value().error);
            }
        } else {
            spdlog::warn("Failed to add resource pack {}: {}",
                         path.filename().string(),
                         result.error().toString());
        }
    }

    notifyChange();
    return addedCount;
}

Result<ResourcePackList::PackInfo> ResourcePackList::addPack(
    const std::filesystem::path& path,
    bool enabled,
    i32 priority)
{
    if (!std::filesystem::exists(path)) {
        return Error(ErrorCode::FileNotFound,
                     "Resource pack not found: " + path.string());
    }

    String normalizedPath = normalizePath(path);

    // 检查是否已存在
    auto existingIt = std::find_if(m_packs.begin(), m_packs.end(),
        [&](const PackInfo& info) { return info.path == normalizedPath; });

    if (existingIt != m_packs.end()) {
        // 更新已存在的资源包
        existingIt->enabled = enabled;
        existingIt->priority = priority;
        notifyChange();
        return *existingIt;
    }

    // 创建资源包信息
    PackInfo info;
    info.path = normalizedPath;
    info.enabled = enabled;
    info.priority = priority;
    info.isZip = isZipFile(path);

    // 创建资源包实例
    if (info.isZip) {
        auto result = ZipResourcePack::create(path);
        if (result.failed()) {
            info.initialized = false;
            info.error = result.error().toString();
            // 仍然添加到列表，但标记为未初始化
            m_packs.push_back(info);
            notifyChange();
            return info;
        }
        info.pack = std::shared_ptr<IResourcePack>(result.value().release());
    } else {
        info.pack = std::make_shared<FolderResourcePack>(path.string());
    }

    // 初始化资源包
    auto initResult = info.pack->initialize();
    if (initResult.failed()) {
        info.initialized = false;
        info.error = initResult.error().toString();
        spdlog::warn("Failed to initialize resource pack {}: {}",
                     path.filename().string(), info.error);
    } else {
        info.initialized = true;
    }

    m_packs.push_back(info);
    notifyChange();

    return info;
}

bool ResourcePackList::removePack(const String& path)
{
    String normalizedPath = path;
    // 如果路径不是规范的，尝试规范化
    if (normalizedPath.find('\\') != String::npos) {
        std::replace(normalizedPath.begin(), normalizedPath.end(), '\\', '/');
    }

    auto it = std::find_if(m_packs.begin(), m_packs.end(),
        [&](const PackInfo& info) { return info.path == normalizedPath; });

    if (it != m_packs.end()) {
        m_packs.erase(it);
        notifyChange();
        return true;
    }

    return false;
}

void ResourcePackList::clear()
{
    m_packs.clear();
    notifyChange();
}

// ============================================================================
// 启用/禁用和优先级
// ============================================================================

bool ResourcePackList::setEnabled(const String& path, bool enabled)
{
    auto it = std::find_if(m_packs.begin(), m_packs.end(),
        [&](const PackInfo& info) { return info.path == path; });

    if (it != m_packs.end() && it->enabled != enabled) {
        it->enabled = enabled;
        notifyChange();
        return true;
    }

    return false;
}

bool ResourcePackList::setPriority(const String& path, i32 priority)
{
    auto it = std::find_if(m_packs.begin(), m_packs.end(),
        [&](const PackInfo& info) { return info.path == path; });

    if (it != m_packs.end() && it->priority != priority) {
        it->priority = priority;
        notifyChange();
        return true;
    }

    return false;
}

bool ResourcePackList::moveUp(const String& path)
{
    auto it = std::find_if(m_packs.begin(), m_packs.end(),
        [&](const PackInfo& info) { return info.path == path; });

    if (it == m_packs.end()) {
        return false;
    }

    // 找到比当前优先级高的下一个资源包
    i32 currentPriority = it->priority;
    i32 maxPriority = currentPriority;

    for (auto& pack : m_packs) {
        if (pack.priority > currentPriority) {
            maxPriority = std::max(maxPriority, pack.priority);
        }
    }

    if (maxPriority == currentPriority) {
        // 已经是最高优先级
        return false;
    }

    // 交换优先级
    for (auto& pack : m_packs) {
        if (pack.priority == maxPriority) {
            pack.priority = currentPriority;
            break;
        }
    }
    it->priority = maxPriority;

    notifyChange();
    return true;
}

bool ResourcePackList::moveDown(const String& path)
{
    auto it = std::find_if(m_packs.begin(), m_packs.end(),
        [&](const PackInfo& info) { return info.path == path; });

    if (it == m_packs.end()) {
        return false;
    }

    // 找到比当前优先级低的下一个资源包
    i32 currentPriority = it->priority;
    i32 minPriority = currentPriority;

    for (auto& pack : m_packs) {
        if (pack.priority < currentPriority) {
            minPriority = std::min(minPriority, pack.priority);
        }
    }

    if (minPriority == currentPriority) {
        // 已经是最低优先级
        return false;
    }

    // 交换优先级
    for (auto& pack : m_packs) {
        if (pack.priority == minPriority) {
            pack.priority = currentPriority;
            break;
        }
    }
    it->priority = minPriority;

    notifyChange();
    return true;
}

// ============================================================================
// 查询方法
// ============================================================================

std::vector<ResourcePackPtr> ResourcePackList::getEnabledPacks() const
{
    std::vector<ResourcePackPtr> result;

    // 收集启用的资源包
    for (const auto& info : m_packs) {
        if (info.enabled && info.initialized && info.pack) {
            result.push_back(info.pack);
        }
    }

    // 按优先级降序排序（高优先级在前）
    std::stable_sort(result.begin(), result.end(),
        [this](const ResourcePackPtr& a, const ResourcePackPtr& b) {
            auto infoA = findPack(a->name());
            auto infoB = findPack(b->name());
            i32 prioA = infoA ? infoA->priority : 0;
            i32 prioB = infoB ? infoB->priority : 0;
            return prioA > prioB;
        });

    return result;
}

std::vector<ResourcePackList::PackInfo> ResourcePackList::getEnabledPackInfos() const
{
    std::vector<PackInfo> result;

    for (const auto& info : m_packs) {
        if (info.enabled && info.initialized) {
            result.push_back(info);
        }
    }

    // 按优先级降序排序
    std::stable_sort(result.begin(), result.end(),
        [](const PackInfo& a, const PackInfo& b) {
            return a.priority > b.priority;
        });

    return result;
}

const ResourcePackList::PackInfo* ResourcePackList::findPack(const String& path) const
{
    auto it = std::find_if(m_packs.begin(), m_packs.end(),
        [&](const PackInfo& info) { return info.path == path; });

    return it != m_packs.end() ? &(*it) : nullptr;
}

ResourcePackList::PackInfo* ResourcePackList::findPack(const String& path)
{
    auto it = std::find_if(m_packs.begin(), m_packs.end(),
        [&](const PackInfo& info) { return info.path == path; });

    return it != m_packs.end() ? &(*it) : nullptr;
}

size_t ResourcePackList::enabledPackCount() const
{
    return std::count_if(m_packs.begin(), m_packs.end(),
        [](const PackInfo& info) { return info.enabled && info.initialized; });
}

// ============================================================================
// 资源访问
// ============================================================================

bool ResourcePackList::hasResource(StringView resourcePath) const
{
    auto packs = getEnabledPacks();

    for (const auto& pack : packs) {
        if (pack->hasResource(resourcePath)) {
            return true;
        }
    }

    return false;
}

Result<std::vector<u8>> ResourcePackList::readResource(StringView resourcePath) const
{
    auto packs = getEnabledPacks();

    for (const auto& pack : packs) {
        if (pack->hasResource(resourcePath)) {
            auto result = pack->readResource(resourcePath);
            if (result.success()) {
                return result;
            }
            // 如果读取失败，继续尝试下一个资源包
            spdlog::debug("Failed to read resource {} from pack {}: {}",
                          resourcePath, pack->name(), result.error().toString());
        }
    }

    return Error(ErrorCode::ResourceNotFound,
                 "Resource not found in any enabled pack: " + String(resourcePath));
}

Result<String> ResourcePackList::readTextResource(StringView resourcePath) const
{
    auto dataResult = readResource(resourcePath);
    if (dataResult.failed()) {
        return dataResult.error();
    }

    const auto& data = dataResult.value();
    return String(data.begin(), data.end());
}

Result<std::vector<String>> ResourcePackList::listResources(
    StringView directory,
    StringView extension) const
{
    std::vector<String> result;
    std::set<String> seen;

    auto packs = getEnabledPacks();

    for (const auto& pack : packs) {
        auto listResult = pack->listResources(directory, extension);
        if (listResult.success()) {
            for (const auto& path : listResult.value()) {
                if (seen.find(path) == seen.end()) {
                    seen.insert(path);
                    result.push_back(path);
                }
            }
        }
    }

    // 排序
    std::sort(result.begin(), result.end());

    return result;
}

// ============================================================================
// 设置同步
// ============================================================================

void ResourcePackList::loadFromSettings(const ResourcePackListOption& settings)
{
    bool changed = false;

    for (const auto& entry : settings.getSortedEnabledEntries()) {
        // 检查资源包是否已存在
        auto* info = findPack(entry.path);
        if (info != nullptr) {
            if (info->enabled != entry.enabled || info->priority != entry.priority) {
                info->enabled = entry.enabled;
                info->priority = entry.priority;
                changed = true;
            }
        } else {
            // 尝试添加资源包
            std::filesystem::path packPath(entry.path);
            auto result = addPack(packPath, entry.enabled, entry.priority);
            if (result.success()) {
                changed = true;
            } else {
                spdlog::warn("Failed to add resource pack from settings {}: {}",
                             entry.path, result.error().toString());
            }
        }
    }

    if (changed) {
        notifyChange();
    }
}

void ResourcePackList::saveToSettings(ResourcePackListOption& settings) const
{
    std::vector<ResourcePackEntry> entries;

    for (const auto& info : m_packs) {
        entries.emplace_back(info.path, info.enabled, info.priority);
    }

    settings.setEntries(std::move(entries));
}

// ============================================================================
// 私有方法
// ============================================================================

String ResourcePackList::normalizePath(const std::filesystem::path& path)
{
    String result = path.string();
    // 统一使用正斜杠
    std::replace(result.begin(), result.end(), '\\', '/');
    return result;
}

bool ResourcePackList::isZipFile(const std::filesystem::path& path)
{
    if (!std::filesystem::is_regular_file(path)) {
        return false;
    }

    String ext = path.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    return ext == ".zip";
}

bool ResourcePackList::isResourcePackDir(const std::filesystem::path& path)
{
    if (!std::filesystem::is_directory(path)) {
        return false;
    }

    // 检查是否包含 pack.mcmeta
    std::filesystem::path mcmetaPath = path / "pack.mcmeta";
    return std::filesystem::exists(mcmetaPath);
}

} // namespace mc
