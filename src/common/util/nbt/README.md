# NBT Library for C++17

Minecraft NBT (Named Binary Tag) 格式序列化库，已集成到 Minecraft Reborn 项目。

## 支持的格式

- **Java Edition**: 大端序二进制格式 (level.dat, player.dat 等)
- **Bedrock Network**: 小端序 + VarInt/Zigzag 编码 (网络协议)
- **Bedrock Disk**: 小端序二进制格式
- **Mojangson**: 文本格式 (命令行和调试输出)

## 使用方法

```cpp
#include "util/nbt/Nbt.hpp"
#include <fstream>

using namespace mc::nbt;

// 读取 Java Edition NBT 文件
std::ifstream input("level.dat", std::ios::binary);
input >> contexts::java;
auto root = tags::compound_tag::read(input);

// 访问数据
auto& levelName = root->get<tags::string_tag>("LevelName");
auto& gameType = root->get<tags::int_tag("GameType");

// 创建 NBT 数据
tags::compound_tag player;
player.put("name", std::string("Steve"));
player.put("level", 100);
player.put("health", 20.0f);

// 写入文件
std::ofstream output("player.dat", std::ios::binary);
output << contexts::java << player;

// Mojangson 文本格式输出
std::cout << contexts::mojangson << player;
```

## 类型映射

| NBT 类型 | C++ 类型 | 标签类 |
|---------|---------|-------|
| Byte | int8_t | `tags::byte_tag` |
| Short | int16_t | `tags::short_tag` |
| Int | int32_t | `tags::int_tag` |
| Long | int64_t | `tags::long_tag` |
| Float | float | `tags::float_tag` |
| Double | double | `tags::double_tag` |
| String | std::string | `tags::string_tag` |
| ByteArray | std::vector<int8_t> | `tags::bytearray_tag` |
| IntArray | std::vector<int32_t> | `tags::intarray_tag` |
| LongArray | std::vector<int64_t> | `tags::longarray_tag` |
| List | - | `tags::list_tag` |
| Compound | std::map | `tags::compound_tag` |

## 项目类型别名

为了符合项目命名风格，提供了以下类型别名：

```cpp
namespace mc::nbt {
    using Tag = tags::tag;
    using CompoundTag = tags::compound_tag;
    using ListTag = tags::list_tag;
    using StringTag = tags::string_tag;
    using IntTag = tags::int_tag;
    using LongTag = tags::long_tag;
    using DoubleTag = tags::double_tag;
    using FloatTag = tags::float_tag;
    // ...
}
```

## 命名空间

- `mc::nbt` - 主命名空间
- `mc::nbt::tags` - 标签类型命名空间
- `mc::nbt::Contexts` - 预定义上下文

## License

原始库许可证见 LICENSE 文件。
