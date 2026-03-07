#include "ZipResourcePack.hpp"

#include <spdlog/spdlog.h>
#include <zlib.h>

#include <algorithm>
#include <cstring>

namespace mr {

// ============================================================================
// ZIP 文件格式常量
// ============================================================================

// 本地文件头签名
constexpr u32 LOCAL_FILE_HEADER_SIG = 0x04034B50;
// 中央目录文件头签名
constexpr u32 CENTRAL_DIR_HEADER_SIG = 0x02014B50;
// 中央目录结束签名
constexpr u32 END_OF_CENTRAL_DIR_SIG = 0x06054B50;

// 压缩方法
constexpr u16 COMPRESSION_STORE = 0;
constexpr u16 COMPRESSION_DEFLATE = 8;

// ============================================================================
// 构造函数 / 析构函数
// ============================================================================

ZipResourcePack::ZipResourcePack(std::filesystem::path zipPath)
    : m_zipPath(std::move(zipPath))
    , m_name(m_zipPath.stem().string())
{
}

ZipResourcePack::~ZipResourcePack()
{
    closeFile();
}

// ============================================================================
// 静态工厂方法
// ============================================================================

Result<std::unique_ptr<ZipResourcePack>> ZipResourcePack::create(const std::filesystem::path& zipPath)
{
    // 检查文件是否存在
    if (!std::filesystem::exists(zipPath)) {
        return Error(ErrorCode::FileNotFound,
                     "ZIP file not found: " + zipPath.string());
    }

    // 检查是否是文件
    if (!std::filesystem::is_regular_file(zipPath)) {
        return Error(ErrorCode::FileNotFound,
                     "Path is not a regular file: " + zipPath.string());
    }

    auto pack = std::unique_ptr<ZipResourcePack>(new ZipResourcePack(zipPath));
    return pack;
}

// ============================================================================
// IResourcePack 接口实现
// ============================================================================

Result<void> ZipResourcePack::initialize()
{
    spdlog::debug("Initializing ZIP resource pack: {}", m_zipPath.string());

    // 打开文件
    auto result = openFile();
    if (result.failed()) {
        return result;
    }

    // 读取中央目录
    auto dirResult = readCentralDirectory();
    if (dirResult.failed()) {
        closeFile();
        return dirResult;
    }

    // 读取 pack.mcmeta
    const String mcmetaPath = "pack.mcmeta";
    if (hasResource(mcmetaPath)) {
        auto dataResult = readResource(mcmetaPath);
        if (dataResult.success()) {
            const auto& data = dataResult.value();
            String jsonStr(data.begin(), data.end());
            auto metadataResult = PackMetadata::parse(jsonStr);
            if (metadataResult.success()) {
                m_metadata = std::move(metadataResult.value());
            } else {
                spdlog::warn("Failed to parse pack.mcmeta for {}: {}",
                             m_name, metadataResult.error().toString());
                // 使用默认元数据
                m_metadata = PackMetadata();
            }
        }
    } else {
        spdlog::debug("No pack.mcmeta found in {}, using default metadata", m_name);
        m_metadata = PackMetadata();
    }

    closeFile();

    spdlog::info("ZIP resource pack '{}' loaded: {} entries, format {}",
                 m_name, m_entries.size(), m_metadata.packFormat());
    return Result<void>::ok();
}

bool ZipResourcePack::hasResource(StringView resourcePath) const
{
    String normalized = normalizePath(resourcePath);
    return m_entries.find(normalized) != m_entries.end();
}

Result<std::vector<u8>> ZipResourcePack::readResource(StringView resourcePath) const
{
    String normalized = normalizePath(resourcePath);

    // 检查缓存
    auto cacheIt = m_cache.find(normalized);
    if (cacheIt != m_cache.end()) {
        return cacheIt->second;
    }

    // 查找条目
    auto entryIt = m_entries.find(normalized);
    if (entryIt == m_entries.end()) {
        return Error(ErrorCode::ResourceNotFound,
                     "Resource not found in ZIP: " + normalized);
    }

    // 解压条目
    auto result = decompressEntry(entryIt->second);
    if (result.failed()) {
        return result;
    }

    // 缓存结果
    m_cache[normalized] = result.value();

    return result;
}

Result<std::vector<String>> ZipResourcePack::listResources(
    StringView directory,
    StringView extension) const
{
    std::vector<String> resources;
    String normalizedDir = normalizePath(directory);

    // 确保目录以斜杠结尾
    if (!normalizedDir.empty() && normalizedDir.back() != '/') {
        normalizedDir += '/';
    }

    for (const auto& [path, entry] : m_entries) {
        // 检查是否在指定目录下
        if (path.size() > normalizedDir.size() &&
            path.substr(0, normalizedDir.size()) == normalizedDir) {

            // 检查扩展名
            if (extension.empty()) {
                resources.push_back(path);
            } else {
                if (path.size() >= extension.size() &&
                    path.substr(path.size() - extension.size()) == extension) {
                    resources.push_back(path);
                }
            }
        }
    }

    // 排序
    std::sort(resources.begin(), resources.end());

    return resources;
}

// ============================================================================
// 额外方法
// ============================================================================

void ZipResourcePack::clearCache()
{
    m_cache.clear();
}

// ============================================================================
// 私有方法
// ============================================================================

Result<void> ZipResourcePack::readCentralDirectory()
{
    // ZIP 文件结构：
    // [本地文件头 + 文件数据] ... [中央目录] [中央目录结束记录]

    // 打开文件（如果未打开）
    if (!m_file) {
        auto result = openFile();
        if (result.failed()) {
            return result;
        }
    }

    // 定位到文件末尾，查找中央目录结束记录
    fseek(m_file, 0, SEEK_END);
    long fileSize = ftell(m_file);

    // EOCD 记录最小 22 字节，从文件末尾向前搜索
    // 最多搜索 64KB（ZIP 规范允许的最大注释长度）
    const long maxSearch = std::min<long>(fileSize, 65536 + 22);
    long searchStart = fileSize - maxSearch;

    if (searchStart < 0) searchStart = 0;

    fseek(m_file, searchStart, SEEK_SET);

    // 查找 EOCD 签名
    u32 signature = 0;
    long eocdOffset = -1;

    while (ftell(m_file) < fileSize - 3) {
        long currentPos = ftell(m_file);
        if (fread(&signature, sizeof(signature), 1, m_file) != 1) {
            break;
        }

        if (signature == END_OF_CENTRAL_DIR_SIG) {
            eocdOffset = currentPos;
            break;
        }

        // 向前移动一个字节继续搜索
        fseek(m_file, currentPos + 1, SEEK_SET);
    }

    if (eocdOffset < 0) {
        return Error(ErrorCode::FileCorrupted,
                     "Invalid ZIP file: End of Central Directory not found");
    }

    // 读取 EOCD 记录
    // 结构：
    // 4 bytes - 签名 (0x06054B50)
    // 2 bytes - 当前磁盘号
    // 2 bytes - 中央目录开始磁盘号
    // 2 bytes - 当前磁盘上的条目数
    // 2 bytes - 中央目录总条目数
    // 4 bytes - 中央目录大小
    // 4 bytes - 中央目录偏移
    // 2 bytes - 注释长度

    fseek(m_file, eocdOffset + 4, SEEK_SET);  // 跳过签名

    u16 diskNumber, centralDirDisk, entriesOnDisk, totalEntries;
    u32 centralDirSize, centralDirOffset;

    if (fread(&diskNumber, 2, 1, m_file) != 1 ||
        fread(&centralDirDisk, 2, 1, m_file) != 1 ||
        fread(&entriesOnDisk, 2, 1, m_file) != 1 ||
        fread(&totalEntries, 2, 1, m_file) != 1 ||
        fread(&centralDirSize, 4, 1, m_file) != 1 ||
        fread(&centralDirOffset, 4, 1, m_file) != 1) {
        return Error(ErrorCode::FileCorrupted,
                     "Failed to read ZIP End of Central Directory");
    }

    // 定位到中央目录
    fseek(m_file, centralDirOffset, SEEK_SET);

    // 读取所有中央目录条目
    m_entries.clear();

    for (u16 i = 0; i < totalEntries; ++i) {
        // 读取中央目录文件头
        // 结构：
        // 4 bytes - 签名 (0x02014B50)
        // 2 bytes - 版本
        // 2 bytes - 需要的版本
        // 2 bytes - 标志
        // 2 bytes - 压缩方法
        // 2 bytes - 修改时间
        // 2 bytes - 修改日期
        // 4 bytes - CRC32
        // 4 bytes - 压缩大小
        // 4 bytes - 解压大小
        // 2 bytes - 文件名长度
        // 2 bytes - 额外字段长度
        // 2 bytes - 注释长度
        // 2 bytes - 磁盘号开始
        // 2 bytes - 内部属性
        // 4 bytes - 外部属性
        // 4 bytes - 本地文件头偏移

        u32 entrySig;
        u16 versionNeeded, flags, compressionMethod;
        u16 modTime, modDate;
        u32 crc32;
        u32 compressedSize, uncompressedSize;
        u16 filenameLen, extraLen, commentLen;
        u32 localHeaderOffset;

        if (fread(&entrySig, 4, 1, m_file) != 1) {
            return Error(ErrorCode::FileCorrupted,
                         "Failed to read ZIP central directory entry signature");
        }

        if (entrySig != CENTRAL_DIR_HEADER_SIG) {
            return Error(ErrorCode::FileCorrupted,
                         "Invalid ZIP central directory entry signature");
        }

        // 跳过版本和标志
        fseek(m_file, 4, SEEK_CUR);

        if (fread(&compressionMethod, 2, 1, m_file) != 1) {
            return Error(ErrorCode::FileCorrupted,
                         "Failed to read compression method");
        }

        // 跳过修改时间和日期、CRC32
        fseek(m_file, 8, SEEK_CUR);

        if (fread(&compressedSize, 4, 1, m_file) != 1 ||
            fread(&uncompressedSize, 4, 1, m_file) != 1 ||
            fread(&filenameLen, 2, 1, m_file) != 1 ||
            fread(&extraLen, 2, 1, m_file) != 1 ||
            fread(&commentLen, 2, 1, m_file) != 1) {
            return Error(ErrorCode::FileCorrupted,
                         "Failed to read ZIP entry sizes");
        }

        // 跳过磁盘号、内部属性、外部属性
        fseek(m_file, 8, SEEK_CUR);

        if (fread(&localHeaderOffset, 4, 1, m_file) != 1) {
            return Error(ErrorCode::FileCorrupted,
                         "Failed to read local header offset");
        }

        // 读取文件名
        std::vector<char> filename(filenameLen + 1);
        if (filenameLen > 0 && fread(filename.data(), 1, filenameLen, m_file) != filenameLen) {
            return Error(ErrorCode::FileCorrupted,
                         "Failed to read ZIP entry filename");
        }
        filename[filenameLen] = '\0';

        // 跳过额外字段和注释
        fseek(m_file, extraLen + commentLen, SEEK_CUR);

        // 创建条目
        ZipEntry entry;
        entry.path = normalizePath(String(filename.data(), filenameLen));
        entry.localHeaderOffset = localHeaderOffset;
        entry.compressedSize = compressedSize;
        entry.uncompressedSize = uncompressedSize;
        entry.compressionMethod = compressionMethod;

        // 跳过目录条目
        if (!entry.path.empty() && entry.path.back() != '/') {
            m_entries[entry.path] = entry;
        }
    }

    spdlog::debug("ZIP central directory read: {} entries", m_entries.size());
    return Result<void>::ok();
}

Result<std::vector<u8>> ZipResourcePack::decompressEntry(const ZipEntry& entry) const
{
    // 打开文件
    auto openResult = openFile();
    if (openResult.failed()) {
        return Error(ErrorCode::FileOpenFailed, "Failed to open ZIP file");
    }

    // 定位到本地文件头
    fseek(m_file, entry.localHeaderOffset, SEEK_SET);

    // 读取本地文件头
    // 结构：
    // 4 bytes - 签名 (0x04034B50)
    // 2 bytes - 需要的版本
    // 2 bytes - 标志
    // 2 bytes - 压缩方法
    // 2 bytes - 修改时间
    // 2 bytes - 修改日期
    // 4 bytes - CRC32
    // 4 bytes - 压缩大小
    // 4 bytes - 解压大小
    // 2 bytes - 文件名长度
    // 2 bytes - 额外字段长度

    u32 localSig;
    u16 versionNeeded, flags, compressionMethod;
    u16 filenameLen, extraLen;
    u32 compressedSize, uncompressedSize;

    if (fread(&localSig, 4, 1, m_file) != 1 || localSig != LOCAL_FILE_HEADER_SIG) {
        return Error(ErrorCode::FileCorrupted, "Invalid local file header signature");
    }

    // 跳过版本、标志、压缩方法、时间、CRC
    fseek(m_file, 10, SEEK_CUR);

    if (fread(&compressedSize, 4, 1, m_file) != 1 ||
        fread(&uncompressedSize, 4, 1, m_file) != 1 ||
        fread(&filenameLen, 2, 1, m_file) != 1 ||
        fread(&extraLen, 2, 1, m_file) != 1) {
        return Error(ErrorCode::FileCorrupted, "Failed to read local file header");
    }

    // 跳过文件名和额外字段
    fseek(m_file, filenameLen + extraLen, SEEK_CUR);

    // 读取压缩数据
    std::vector<u8> compressedData(entry.compressedSize);
    if (entry.compressedSize > 0) {
        if (fread(compressedData.data(), 1, entry.compressedSize, m_file) != entry.compressedSize) {
            return Error(ErrorCode::FileReadFailed, "Failed to read compressed data");
        }
    }

    // 解压
    std::vector<u8> data(entry.uncompressedSize);

    if (entry.compressionMethod == COMPRESSION_STORE) {
        // 存储（无压缩）
        data = std::move(compressedData);
    } else if (entry.compressionMethod == COMPRESSION_DEFLATE) {
        // Deflate 解压
        z_stream stream = {};
        stream.next_in = compressedData.data();
        stream.avail_in = static_cast<uInt>(compressedData.size());
        stream.next_out = data.data();
        stream.avail_out = static_cast<uInt>(data.size());

        // 使用 zlib 的 inflate，使用 -15 作为窗口大小（原始 deflate）
        if (inflateInit2(&stream, -15) != Z_OK) {
            return Error(ErrorCode::DecompressionFailed, "Failed to initialize inflate");
        }

        int ret = inflate(&stream, Z_FINISH);
        inflateEnd(&stream);

        if (ret != Z_STREAM_END) {
            return Error(ErrorCode::DecompressionFailed,
                         String("Failed to decompress: ") + zError(ret));
        }
    } else {
        return Error(ErrorCode::Unsupported,
                     "Unsupported compression method: " + std::to_string(entry.compressionMethod));
    }

    // 关闭文件（释放资源）
    closeFile();

    return data;
}

String ZipResourcePack::normalizePath(StringView path)
{
    String result(path);

    // 统一使用正斜杠
    std::replace(result.begin(), result.end(), '\\', '/');

    // 移除前导斜杠
    while (!result.empty() && result.front() == '/') {
        result.erase(0, 1);
    }

    return result;
}

Result<void> ZipResourcePack::openFile() const
{
    if (!m_file) {
#ifdef _WIN32
        _wfopen_s(&m_file, m_zipPath.wstring().c_str(), L"rb");
#else
        m_file = fopen(m_zipPath.string().c_str(), "rb");
#endif
        if (!m_file) {
            return Error(ErrorCode::FileOpenFailed,
                         "Failed to open ZIP file: " + m_zipPath.string());
        }
    }
    return Result<void>::ok();
}

void ZipResourcePack::closeFile() const
{
    if (m_file) {
        fclose(m_file);
        m_file = nullptr;
    }
}

} // namespace mr
