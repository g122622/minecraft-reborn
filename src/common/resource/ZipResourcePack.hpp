#pragma once

#include "common/resource/IResourcePack.hpp"
#include <filesystem>
#include <map>
#include <unordered_map>

namespace mr {

/**
 * @brief ZIP 资源包实现
 *
 * 从 ZIP 文件读取资源，支持标准的 Minecraft 资源包格式。
 * 使用 zlib 进行解压，支持缓存已读取的资源以提高性能。
 *
 * 使用示例:
 * @code
 * auto result = ZipResourcePack::create("packs/fancy.zip");
 * if (result.success()) {
 *     auto pack = std::move(result.value());
 *     auto initResult = pack->initialize();
 *     if (initResult.success()) {
 *         auto data = pack->readResource("assets/minecraft/textures/block/stone.png");
 *         // ...
 *     }
 * }
 * @endcode
 */
class ZipResourcePack : public IResourcePack {
public:
    /**
     * @brief 析构函数
     */
    ~ZipResourcePack() override;

    /**
     * @brief 创建 ZIP 资源包
     * @param zipPath ZIP 文件路径
     * @return 资源包实例或错误
     */
    [[nodiscard]] static Result<std::unique_ptr<ZipResourcePack>> create(const std::filesystem::path& zipPath);

    // IResourcePack 接口实现

    /**
     * @brief 初始化资源包
     *
     * 读取 pack.mcmeta 元数据，构建文件索引。
     *
     * @return 成功或错误
     */
    [[nodiscard]] Result<void> initialize() override;

    /**
     * @brief 获取元数据
     */
    [[nodiscard]] const PackMetadata& metadata() const override { return m_metadata; }

    /**
     * @brief 检查资源是否存在
     * @param resourcePath 资源路径（如 "assets/minecraft/textures/block/stone.png"）
     * @return 是否存在
     */
    [[nodiscard]] bool hasResource(StringView resourcePath) const override;

    /**
     * @brief 读取资源内容
     * @param resourcePath 资源路径
     * @return 资源数据或错误
     */
    [[nodiscard]] Result<std::vector<u8>> readResource(StringView resourcePath) const override;

    /**
     * @brief 列出目录下的所有资源
     * @param directory 目录路径
     * @param extension 文件扩展名过滤（可选）
     * @return 资源路径列表
     */
    [[nodiscard]] Result<std::vector<String>> listResources(
        StringView directory,
        StringView extension = "") const override;

    /**
     * @brief 获取资源包名称
     */
    [[nodiscard]] String name() const override { return m_name; }

    // 额外方法

    /**
     * @brief 获取 ZIP 文件路径
     */
    [[nodiscard]] const std::filesystem::path& zipPath() const { return m_zipPath; }

    /**
     * @brief 获取 ZIP 文件中的条目数量
     */
    [[nodiscard]] size_t entryCount() const { return m_entries.size(); }

    /**
     * @brief 清除缓存
     */
    void clearCache();

private:
    /**
     * @brief ZIP 文件条目信息
     */
    struct ZipEntry {
        u64 localHeaderOffset;  ///< 本地文件头偏移
        u64 compressedSize;     ///< 压缩后大小
        u64 uncompressedSize;   ///< 解压后大小
        u16 compressionMethod;  ///< 压缩方法 (0=存储, 8=Deflate)
        String path;            ///< 文件路径（已规范化）
    };

    /// 私有构造函数
    explicit ZipResourcePack(std::filesystem::path zipPath);

    /**
     * @brief 读取 ZIP 文件的中央目录
     * @return 成功或错误
     */
    [[nodiscard]] Result<void> readCentralDirectory();

    /**
     * @brief 解压单个文件
     * @param entry ZIP 条目信息
     * @return 解压后的数据或错误
     */
    [[nodiscard]] Result<std::vector<u8>> decompressEntry(const ZipEntry& entry) const;

    /**
     * @brief 规范化资源路径
     * @param path 原始路径
     * @return 规范化后的路径（统一使用正斜杠，移除前导斜杠）
     */
    [[nodiscard]] static String normalizePath(StringView path);

    std::filesystem::path m_zipPath;              ///< ZIP 文件路径
    String m_name;                                 ///< 资源包名称（文件名）
    PackMetadata m_metadata;                       ///< 元数据
    std::unordered_map<String, ZipEntry> m_entries; ///< 文件条目索引（路径 -> 条目）

    /// 可变缓存（mutable 以支持 const 方法中的缓存）
    mutable std::unordered_map<String, std::vector<u8>> m_cache;

    /// ZIP 文件指针（使用指针以支持 mutable 访问）
    mutable FILE* m_file = nullptr;

    /**
     * @brief 打开 ZIP 文件
     * @return 成功或错误
     */
    [[nodiscard]] Result<void> openFile() const;

    /**
     * @brief 关闭 ZIP 文件
     */
    void closeFile() const;
};

} // namespace mr
