# 📋 Minecraft Reborn 项目代码规范 v1.0

---

## 📖 目录

1. [总则](#1-总则)
2. [C++语言特性规范](#2-c语言特性规范)
3. [命名规范](#3-命名规范)
4. [文件组织规范](#4-文件组织规范)
5. [注释与文档规范](#5-注释与文档规范)
6. [内存管理规范](#6-内存管理规范)
7. [错误处理规范](#7-错误处理规范)
8. [并发编程规范](#8-并发编程规范)
9. [性能相关规范](#9-性能相关规范)
10. [安全规范](#10-安全规范)
11. [日志规范](#11-日志规范)
12. [测试规范](#12-测试规范)
13. [Git提交规范](#13-git提交规范)
14. [代码审查规范](#14-代码审查规范)

---

## 1. 总则

### 1.1 规范目的

本规范旨在确保项目代码的**一致性**、**可维护性**、**可读性**和**安全性**，降低团队协作成本，提高代码质量。

### 1.2 适用范围

- 所有C++源代码文件 (`.hpp`, `.cpp`)
- 所有JavaScript模组代码 (`.js`)
- 所有CMake构建文件 (`.cmake`, `CMakeLists.txt`)
- 所有配置文件 (`.json`, `.yaml`)

### 1.3 规范优先级

```
安全规范 > 性能规范 > 语言规范 > 风格规范
```

### 1.4 例外处理

如有特殊情况需要违反本规范，必须：
1. 在代码中添加明确注释说明原因
2. 经过代码审查委员会批准
3. 记录在技术决策记录(ADR)中

---

## 2. C++语言特性规范

### 2.1 C++标准

```cmake
# 强制使用C++17标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
```

### 2.2 允许使用的C++17特性

| 特性 | 使用建议 | 示例 |
|------|----------|------|
| `auto` | ✅ 推荐 | `auto iter = vec.begin();` |
| 结构化绑定 | ✅ 推荐 | `auto [x, y, z] = position;` |
| `std::optional` | ✅ 推荐 | `std::optional<Block> getBlock();` |
| `std::variant` | ✅ 推荐 | `std::variant<Block, Entity> hitResult;` |
| `std::any` | ⚠️ 谨慎 | 避免过度使用 |
| `if constexpr` | ✅ 推荐 | 模板元编程 |
| `std::string_view` | ✅ 推荐 | 只读字符串参数 |
| 内联变量 | ✅ 推荐 | `inline constexpr int MAX = 100;` |
| 折叠表达式 | ✅ 推荐 | 模板编程 |
| `std::filesystem` | ✅ 推荐 | 文件操作 |

### 2.3 禁止使用的特性

```cpp
// ❌ 禁止：C风格强制转换
int* ptr = (int*)malloc(sizeof(int));

// ✅ 推荐：C++风格强制转换
int* ptr = static_cast<int*>(malloc(sizeof(int)));

// ❌ 禁止：原始数组
int arr[10];

// ✅ 推荐：std::array或std::vector
std::array<int, 10> arr;
std::vector<int> vec;

// ❌ 禁止：裸指针管理内存
int* ptr = new int(5);
delete ptr;

// ✅ 推荐：智能指针
auto ptr = std::make_unique<int>(5);

// ❌ 禁止：宏定义常量
#define MAX_PLAYERS 100

// ✅ 推荐：constexpr常量
inline constexpr int MAX_PLAYERS = 100;

// ❌ 禁止：异常用于流程控制
try {
    // 正常逻辑
} catch (...) {
    // 错误处理
}

// ✅ 推荐：使用Result/Expected类型
Result<void> result = doSomething();
if (!result.success()) {
    // 错误处理
}
```

### 2.4 类设计规范

```cpp
// ✅ 推荐：Rule of Five
class Chunk {
public:
    Chunk();                                    // 默认构造函数
    Chunk(const Chunk& other);                  // 拷贝构造
    Chunk(Chunk&& other) noexcept;              // 移动构造
    Chunk& operator=(const Chunk& other);       // 拷贝赋值
    Chunk& operator=(Chunk&& other) noexcept;   // 移动赋值
    ~Chunk();                                   // 析构函数
    
    // 工厂方法
    static std::unique_ptr<Chunk> create(int32_t x, int32_t z);
    
private:
    // 数据成员
    std::vector<Block> m_blocks;
    ChunkPos m_position;
};

// ✅ 推荐：明确指定访问控制
class Entity {
public:
    // 公共接口
    void update(float deltaTime);
    
protected:
    // 派生类可访问
    virtual void onTick();
    
private:
    // 私有实现
    void internalUpdate();
    EntityID m_id;
};

// ❌ 禁止：友元滥用
class A {
    friend class B;  // 除非必要，否则避免
};
```

### 2.5 函数设计规范

```cpp
// ✅ 推荐：参数顺序（必要参数在前，可选参数在后）
void spawnEntity(EntityType type, 
                 Vector3 position,
                 std::optional<EntityData> data = std::nullopt);

// ✅ 推荐：使用string_view传递只读字符串
void loadTexture(std::string_view path);

// ✅ 推荐：const引用传递大对象
void processChunk(const Chunk& chunk);

// ✅ 推荐：移动语义
void setTexture(std::unique_ptr<Texture> texture);

// ✅ 推荐：[[nodiscard]]标记必须检查返回值的函数
[[nodiscard]] Result<void> initialize();

// ✅ 推荐：[[nodiscard]]标记可能产生新资源的函数
[[nodiscard]] std::unique_ptr<Entity> createEntity();

// ❌ 禁止：过长的参数列表（超过5个考虑使用配置对象）
void createWindow(int width, int height, int x, int y, 
                  const std::string& title, bool fullscreen,
                  int monitor, bool vsync, int samples);

// ✅ 推荐：使用配置结构体
struct WindowConfig {
    int width = 1920;
    int height = 1080;
    int x = 0;
    int y = 0;
    std::string title = "Minecraft Reborn";
    bool fullscreen = false;
    int monitor = 0;
    bool vsync = true;
    int samples = 4;
};

void createWindow(const WindowConfig& config);
```

---

## 3. 命名规范

### 3.1 文件命名

| 类型 | 规范 | 示例 |
|------|------|------|
| 头文件 | `PascalCase.hpp` | `ChunkManager.hpp` |
| 源文件 | `PascalCase.cpp` | `ChunkManager.cpp` |
| CMake文件 | `PascalCase.cmake` | `CompilerWarnings.cmake` |
| 测试文件 | `test_*.cpp` | `test_chunk.cpp` |
| 配置脚本 | `kebab-case.sh` | `build-project.sh` |

### 3.2 类型命名

```cpp
// ✅ 类/结构体：PascalCase
class ChunkManager;
struct Vector3;
enum class BlockType;

// ✅ 模板参数：PascalCase
template<typename T, typename Allocator>
class Buffer;

// ✅ 类型别名：PascalCase
using ChunkPos = Vector3;
using EntityList = std::vector<Entity*>;

// ❌ 禁止：在类型名中添加前缀
class CChunkManager;  // 不要加C前缀
class IEntityManager; // 不要加I前缀（除非是纯接口）
```

### 3.3 变量命名

```cpp
// ✅ 成员变量：m_前缀 + camelCase
class Player {
private:
    float m_health;
    std::string m_name;
    Vector3 m_position;
};

// ✅ 局部变量：camelCase
void update() {
    float health = 100.0f;
    std::string playerName = "Steve";
}

// ✅ 全局变量：g_前缀 + camelCase
inline constexpr int g_maxPlayers = 100;

// ✅ 静态成员变量：s_前缀 + camelCase
class Entity {
private:
    static uint64_t s_nextId;
};

// ✅ 常量：UPPER_SNAKE_CASE
inline constexpr int MAX_CHUNKS = 1024;
inline constexpr float GRAVITY = 9.81f;

// ✅ 枚举值：PascalCase（scoped enum）
enum class BlockType {
    Air,
    Stone,
    Dirt,
    Grass
};

// ❌ 禁止：单字母变量（循环计数器除外）
int x;  // ❌
int i;  // ✅ (循环计数器)

// ❌ 禁止：匈牙利命名法
int nCount;    // ❌
std::string strName;  // ❌
```

### 3.4 函数命名

```cpp
// ✅ 函数：camelCase
void updatePlayer();
float calculateDistance();
std::string getPlayerName();

// ✅ 访问器：get/set前缀
int getHealth() const;
void setHealth(int health);

// ✅ 布尔查询：is/has/can前缀
bool isAlive() const;
bool hasItem() const;
bool canJump() const;

// ✅ 工厂函数：create/make前缀
std::unique_ptr<Entity> createEntity();
std::shared_ptr<Texture> makeTexture();

// ✅ 事件处理：on前缀
void onChunkLoaded();
void onPlayerJoin();

// ❌ 禁止：动词名词混用
void update_player();    // ❌ (应使用camelCase)
void UpdatePlayer();     // ❌ (应使用camelCase)
```

### 3.5 命名空间规范

```cpp
// ✅ 推荐：小写命名空间
namespace mc {
namespace client {
namespace renderer {

// ✅ 推荐：嵌套不超过3层
namespace mc::client::renderer {

// ✅ 推荐：命名空间别名（长命名空间）
namespace fs = std::filesystem;
namespace vk = vulkan;

// ❌ 禁止：不允许使用 using namespace std！
using namespace std;  // ❌
```

---

## 4. 文件组织规范

### 4.1 头文件结构

```cpp
// ChunkManager.hpp
#pragma once  // ✅ 推荐：使用pragma once

// 1. 所属模块的完整头文件
#include "world/Chunk.hpp"

// 2. 标准库头文件（按字母顺序）
#include <memory>
#include <vector>
#include <unordered_map>

// 3. 第三方库头文件
#include <glm/vec3.hpp>
#include <entt/entt.hpp>

// 4. 前置声明（减少编译依赖）
class World;
struct ChunkPos;

namespace mc::world {

/**
 * @brief 区块管理器
 * 
 * 负责区块的加载、卸载、缓存和管理
 */
class ChunkManager {
public:
    // 公共接口
};

} // namespace mc::world
```

### 4.2 源文件结构

```cpp
// ChunkManager.cpp

// 1. 对应的头文件（必须是第一个）
#include "world/ChunkManager.hpp"

// 2. 其他项目头文件
#include "world/Chunk.hpp"
#include "world/World.hpp"

// 3. 标准库
#include <algorithm>
#include <fstream>

// 4. 第三方库
#include <spdlog/spdlog.h>

namespace mc::world {

// 实现代码...

} // namespace mc::world
```

### 4.3 包含顺序规则

```
1. 对应的头文件
2. 项目内部头文件
3. 第三方库头文件
4. 标准库头文件
```

### 4.4 头文件自包含

```cpp
// ✅ 推荐：头文件必须自包含
// ChunkManager.hpp
#pragma once
#include "ChunkManager.hpp"  // 自身必须能编译

// ❌ 禁止：依赖包含顺序
// 用户必须按特定顺序包含头文件才能编译
```

---

## 5. 注释与文档规范

### 5.1 Doxygen文档注释

```cpp
/**
 * @brief 加载区块
 * 
 * 从磁盘或缓存中加载指定位置的区块。如果区块不存在，
 * 则使用世界生成器生成新区块。
 * 
 * @param pos 区块位置
 * @param priority 加载优先级（0-10，10最高）
 * @return Result<std::shared_ptr<Chunk>> 加载结果
 * 
 * @throws ChunkLoadException 当区块加载失败时
 * 
 * @note 此方法是异步的，返回后区块可能尚未完全加载
 * @warning 不要在主线程中调用高优先级加载
 * 
 * @see unloadChunk()
 * @see isChunkLoaded()
 * 
 * @example
 * ```cpp
 * auto result = chunkManager.loadChunk({0, 0, 0}, 5);
 * if (result.success()) {
 *     auto chunk = result.value();
 *     // 使用区块
 * }
 * ```
 */
Result<std::shared_ptr<Chunk>> loadChunk(ChunkPos pos, int priority);
```

### 5.2 行内注释

```cpp
// ✅ 推荐：解释"为什么"而不是"是什么"
// 使用二次插值平滑相机移动，避免突变
float alpha = smoothstep(0.0f, 1.0f, deltaTime * 5.0f);

// ❌ 禁止：冗余注释
i++;  // i加1 ❌

// ✅ 推荐：标记待办事项
// TODO: 优化区块加载算法，当前O(n²)复杂度
// FIXME: 内存泄漏问题，需要在析构函数中释放
// HACK: 临时解决方案，等待Vulkan驱动更新
// NOTE: 此处性能关键，不要随意修改
```

### 5.3 代码区域标记

```cpp
#pragma region 区块加载

// 相关代码...

#pragma endregion 区块加载

// 或使用注释
// ============================================================================
// 区块加载系统
// ============================================================================
```

---

## 6. 内存管理规范

### 6.1 智能指针使用

```cpp
// ✅ 推荐：优先使用std::unique_ptr
std::unique_ptr<Chunk> chunk = std::make_unique<Chunk>();

// ✅ 推荐：共享所有权使用std::shared_ptr
std::shared_ptr<Texture> texture = std::make_shared<Texture>();

// ✅ 推荐：使用make函数
auto ptr = std::make_unique<Type>(args...);
auto ptr = std::make_shared<Type>(args...);

// ❌ 禁止：裸new/delete
Type* ptr = new Type();  // ❌
delete ptr;              // ❌

// ❌ 禁止：shared_ptr循环引用
class A {
    std::shared_ptr<B> b;  // 可能导致循环引用
};
class B {
    std::shared_ptr<A> a;  // ❌
};

// ✅ 推荐：使用weak_ptr打破循环
class B {
    std::weak_ptr<A> a;  // ✅
};
```

### 6.2 容器使用

```cpp
// ✅ 推荐：预分配容量
std::vector<Block> blocks;
blocks.reserve(16 * 256 * 16);  // 区块大小

// ✅ 推荐：使用emplace_back代替push_back
vec.emplace_back(args...);  // ✅ 原地构造
vec.push_back(Type(args...)); // ❌ 可能产生临时对象

// ✅ 推荐：使用string_view避免拷贝
void processString(std::string_view str);

// ✅ 推荐：使用span传递连续内存
void processArray(std::span<const float> data);

// ❌ 禁止：在循环中频繁扩容
for (int i = 0; i < 1000; i++) {
    vec.push_back(i);  // ❌ 可能多次重新分配
}

// ✅ 推荐：预先分配
vec.reserve(1000);
for (int i = 0; i < 1000; i++) {
    vec.push_back(i);  // ✅
}
```

### 6.3 资源管理(RAII)

```cpp
// ✅ 推荐：使用RAII管理资源
class VulkanBuffer {
public:
    VulkanBuffer(Device& device, size_t size);
    ~VulkanBuffer();  // 自动释放资源
    
    // 禁止拷贝
    VulkanBuffer(const VulkanBuffer&) = delete;
    VulkanBuffer& operator=(const VulkanBuffer&) = delete;
    
    // 允许移动
    VulkanBuffer(VulkanBuffer&&) noexcept;
    VulkanBuffer& operator=(VulkanBuffer&&) noexcept;
    
private:
    Device& m_device;
    VkBuffer m_buffer;
    VkDeviceMemory m_memory;
};

// 使用
{
    VulkanBuffer buffer(device, size);
    // 使用buffer
}  // 自动释放
```

---

## 7. 错误处理规范

### 7.1 Result类型使用

```cpp
// ✅ 推荐：使用Result类型处理错误
Result<void> initialize() {
    auto result = createWindow();
    if (!result.success()) {
        return result;  // 传播错误
    }
    
    result = createRenderer();
    if (!result.success()) {
        return result;
    }
    
    return Result<void>::ok();
}

// ✅ 推荐：使用TRY宏简化代码
#define TRY(expr) \
    do { \
        auto _result = (expr); \
        if (!_result.success()) { \
            return _result; \
        } \
    } while(0)

Result<void> initialize() {
    TRY(createWindow());
    TRY(createRenderer());
    return Result<void>::ok();
}

// ✅ 推荐：携带错误信息
class Error {
public:
    enum class Code {
        Success = 0,
        NotFound,
        InvalidArgument,
        OutOfMemory,
        // ...
    };
    
    Code code() const;
    const std::string& message() const;
    const std::string& source() const;  // 错误位置
};

// 使用
Result<Chunk*> loadChunk(ChunkPos pos) {
    if (!chunkExists(pos)) {
        return Error{
            Error::Code::NotFound,
            fmt::format("Chunk at {} not found", pos),
            "ChunkManager::loadChunk"
        };
    }
    // ...
}
```

### 7.2 异常使用规范

```cpp
// ✅ 允许：不可恢复的错误
throw std::runtime_error("Critical system failure");

// ✅ 允许：编程错误
throw std::logic_error("Invalid state");

// ❌ 禁止：异常用于流程控制
try {
    value = map.at(key);
} catch (const std::out_of_range&) {
    value = defaultValue;
}

// ✅ 推荐：检查后访问
auto iter = map.find(key);
if (iter != map.end()) {
    value = iter->second;
} else {
    value = defaultValue;
}

// ✅ 推荐：异常边界（在API边界捕获异常）
extern "C" int api_function() {
    try {
        // C++实现
        return 0;
    } catch (...) {
        return -1;  // 转换为错误码
    }
}
```

### 7.3 断言使用

```cpp
// ✅ 推荐：调试时检查前置条件
assert(ptr != nullptr);
assert(index < size);

// ✅ 推荐：使用静态断言检查编译期条件
static_assert(sizeof(Block) == 16, "Block size must be 16 bytes");

// ❌ 禁止：断言用于验证用户输入
assert(userInput > 0);  // ❌ 应该用if检查

// ✅ 推荐：断言用于检查内部不变量
assert(m_health >= 0 && m_health <= 100);

// ✅ 推荐：发布版本禁用断言
#ifdef NDEBUG
    #define DEBUG_ASSERT(expr) ((void)0)
#else
    #define DEBUG_ASSERT(expr) assert(expr)
#endif
```

---

## 8. 并发编程规范

### 8.1 线程安全

```cpp
// ✅ 推荐：明确标注线程安全级别
/**
 * @brief 线程安全级别：
 * - 线程不安全：需要外部同步
 * - 线程安全：可安全并发访问
 * - 只读线程安全：并发读安全，写需要同步
 */
class ChunkManager {
    // 线程不安全 - 需要外部同步
};

// ✅ 推荐：使用互斥锁保护共享数据
class PlayerManager {
public:
    void addPlayer(std::shared_ptr<Player> player) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_players.push_back(player);
    }
    
private:
    std::mutex m_mutex;
    std::vector<std::shared_ptr<Player>> m_players;
};

// ✅ 推荐：使用原子操作
class Counter {
public:
    void increment() {
        m_count.fetch_add(1, std::memory_order_relaxed);
    }
    
    int get() const {
        return m_count.load(std::memory_order_relaxed);
    }
    
private:
    std::atomic<int> m_count{0};
};

// ❌ 禁止：数据竞争
int sharedCounter = 0;  // ❌ 多个线程访问

// ✅ 推荐：使用atomic
std::atomic<int> sharedCounter{0};  // ✅
```

### 8.2 锁的使用

```cpp
// ✅ 推荐：使用lock_guard或unique_lock
void criticalSection() {
    std::lock_guard<std::mutex> lock(m_mutex);
    // 临界区代码
}  // 自动释放

// ✅ 推荐：避免死锁（固定锁顺序）
// 总是先锁mutex1，再锁mutex2
void transfer(Account& from, Account& to) {
    std::lock(from.mutex(), to.mutex());
    std::lock_guard<std::mutex> lock1(from.mutex(), std::adopt_lock);
    std::lock_guard<std::mutex> lock2(to.mutex(), std::adopt_lock);
    // 转账操作
}

// ❌ 禁止：在持有锁时执行耗时操作
void process() {
    std::lock_guard<std::mutex> lock(m_mutex);
    doHeavyWork();  // ❌ 应该在锁外执行
}

// ✅ 推荐：缩小临界区范围
{
    std::lock_guard<std::mutex> lock(m_mutex);
    data = copyData();
}
doHeavyWork(data);  // ✅ 锁外执行
```

### 8.3 无锁编程

```cpp
// ✅ 推荐：使用无锁队列进行线程间通信
moodycamel::ConcurrentQueue<PacketPtr> m_packetQueue;

// 生产者
m_packetQueue.enqueue(packet);

// 消费者
PacketPtr packet;
if (m_packetQueue.try_dequeue(packet)) {
    process(packet);
}

// ✅ 推荐：使用读写锁（读多写少场景）
class ConfigManager {
public:
    std::string getConfig(const std::string& key) {
        std::shared_lock<std::shared_mutex> lock(m_mutex);
        return m_config.at(key);
    }
    
    void setConfig(const std::string& key, const std::string& value) {
        std::unique_lock<std::shared_mutex> lock(m_mutex);
        m_config[key] = value;
    }
    
private:
    mutable std::shared_mutex m_mutex;
    std::unordered_map<std::string, std::string> m_config;
};
```

---

## 9. 性能相关规范

### 9.1 尽可能使用f32而非f64以提升处理速度

f32精度已经足够，非必要不允许使用f64。

### 9.2 避免的性能陷阱

```cpp
// ❌ 禁止：不必要的拷贝
std::string getName() { return m_name; }  // ❌

// ✅ 推荐：返回const引用
const std::string& getName() const { return m_name; }  // ✅

// ❌ 禁止：在循环中重复计算
for (int i = 0; i < vec.size(); i++) {  // size()每次调用
    // ...
}

// ✅ 推荐：缓存循环条件
for (int i = 0, n = vec.size(); i < n; i++) {  // ✅
    // ...
}

// ❌ 禁止：不必要的虚函数调用（性能关键路径）
class Entity {
    virtual void update() = 0;  // ❌ 每帧调用
};

// ✅ 推荐：使用CRTP或函数指针
template<typename T>
class Entity {
    void update() { static_cast<T*>(this)->updateImpl(); }
};

// ❌ 禁止：频繁内存分配
void update() {
    std::vector<Entity*> visible = getVisibleEntities();  // ❌ 每帧分配
}

// ✅ 推荐：对象池或预分配
class EntityPool {
    std::vector<Entity*> m_pool;
    std::vector<Entity*> m_free;
    
    Entity* acquire() {
        if (m_free.empty()) {
            m_pool.push_back(new Entity());
            return m_pool.back();
        }
        Entity* e = m_free.back();
        m_free.pop_back();
        return e;
    }
};
```

### 9.3 缓存友好

```cpp
// ✅ 推荐：数据局部性
struct Entity {
    Vector3 position;  // 经常一起访问的数据放在一起
    Vector3 velocity;
    float health;
    // ...
};

// ✅ 推荐：SOA布局（适合批量处理）
struct EntityArray {
    std::vector<Vector3> positions;
    std::vector<Vector3> velocities;
    std::vector<float> healths;
};

// ❌ 禁止：指针追逐
class Node {
    Node* next;  // ❌ 缓存不友好
    Node* prev;
};

// ✅ 推荐：连续内存
std::vector<Node> nodes;  // ✅ 缓存友好
```

---

## 10. 安全规范

### 10.1 输入验证

```cpp
// ✅ 推荐：验证所有外部输入
Result<void> loadFile(const std::string& path) {
    // 验证路径
    if (path.empty()) {
        return Error::invalidArgument("Path cannot be empty");
    }
    
    // 防止路径遍历
    if (path.find("..") != std::string::npos) {
        return Error::invalidArgument("Invalid path");
    }
    
    // 验证文件扩展名
    if (!path.ends_with(".json")) {
        return Error::invalidArgument("Invalid file type");
    }
    
    // ...
}

// ✅ 推荐：限制输入大小
void processMessage(const std::string& message) {
    constexpr size_t MAX_MESSAGE_LENGTH = 256;
    if (message.length() > MAX_MESSAGE_LENGTH) {
        return Error::invalidArgument("Message too long");
    }
    // ...
}
```

---

## 12. 测试规范

### 12.1 测试命名

```cpp
// ✅ 推荐：测试名称格式
TEST(ChunkTest, CreateEmptyChunk)           // 类测试
TEST_F(PlayerTest, TakeDamage_ReduceHealth) // 带fixture的测试
TEST_P(NetworkTest, PacketSerialization)    // 参数化测试

// ✅ 推荐：测试名称描述行为
TEST(PlayerTest, TakeDamage_WhenHealthZero_Dies)
TEST(PlayerTest, TakeDamage_WithInvulnerability_IgnoresDamage)
```

### 12.2 测试结构

```cpp
// ✅ 推荐：AAA模式（Arrange-Act-Assert）
TEST(ChunkTest, SetAndGetBlock) {
    // Arrange - 准备
    Chunk chunk(0, 0, 0);
    BlockPos pos(5, 100, 5);
    
    // Act - 执行
    chunk.setBlock(pos, BlockType::Stone);
    auto block = chunk.getBlock(pos);
    
    // Assert - 断言
    EXPECT_EQ(block.type, BlockType::Stone);
}

// ✅ 推荐：测试夹具
class PlayerTest : public ::testing::Test {
protected:
    void SetUp() override {
        m_player = std::make_unique<Player>();
        m_player->setHealth(100);
    }
    
    void TearDown() override {
        m_player.reset();
    }
    
    std::unique_ptr<Player> m_player;
};

TEST_F(PlayerTest, TakeDamage_ReduceHealth) {
    m_player->takeDamage(20);
    EXPECT_EQ(m_player->getHealth(), 80);
}
```

## 14. 代码审查规范

### 14.1 审查清单

```markdown
## 代码审查清单

### 功能
- [ ] 代码实现是否符合需求
- [ ] 边界条件是否处理
- [ ] 错误处理是否完善

### 代码质量
- [ ] 是否符合代码规范
- [ ] 是否有重复代码
- [ ] 函数是否过长（>50行）
- [ ] 类是否过大（>500行）

### 性能
- [ ] 是否有明显的性能问题
- [ ] 是否有不必要的内存分配
- [ ] 是否有缓存不友好的代码

### 安全
- [ ] 输入是否验证
- [ ] 是否有缓冲区溢出风险
- [ ] 是否有资源泄漏

### 测试
- [ ] 是否有单元测试
- [ ] 测试覆盖率是否达标
- [ ] 是否有集成测试

### 文档
- [ ] 公共API是否有文档
- [ ] 复杂逻辑是否有注释
- [ ] 是否更新了相关文档
```
