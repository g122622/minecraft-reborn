# 流体子系统补齐计划

## 当前状态概述

流体子系统已完成基础框架，包括：
- `Fluid` / `FluidState` 基类
- `FlowingFluid` 流动流体核心逻辑
- `FluidRegistry` 流体注册表
- `FluidTags` 流体标签系统
- `WaterFluid` / `LavaFluid` / `EmptyFluid` 具体流体实现

## 对比 MC 1.16.5 缺失功能清单

### 阶段1：核心依赖接口（必须完成）

#### 1.1 IBlockReader 接口完善
**文件**: `src/common/world/IWorld.hpp`

当前 `IBlockReader` 只是简单继承 `IWorld`，需要添加以下方法：

```cpp
// 需要添加到 IBlockReader 的方法
class IBlockReader {
public:
    // 已有: getBlockState, getFluidState

    // 新增：获取方块光照
    [[nodiscard]] virtual u8 getLightValue(const BlockPos& pos) const;

    // 新增：获取最大光照等级
    [[nodiscard]] constexpr u8 getMaxLightLevel() const { return 15; }

    // 新增：获取世界高度
    [[nodiscard]] constexpr i32 getHeight() const { return 256; }

    // 新增：检查方块侧面是否为实体
    [[nodiscard]] virtual bool isSideSolid(const BlockPos& pos, Direction side) const;
};
```

#### 1.2 BlockState 扩展
**文件**: `src/common/world/block/BlockState.hpp`

需要添加以下方法：
- `getCollisionShape()` - 获取碰撞箱
- `isSolidSide(IBlockReader&, BlockPos, Direction)` - 检查侧面是否为实体
- `isSolid()` - 是否为固体方块
- `isOpaqueCube(IBlockReader&, BlockPos)` - 是否为不透明完整方块
- `getMaterial()` - 获取材质（已有Material类）

#### 1.3 Material 材质系统完善
**文件**: `src/common/world/block/Material.hpp`

需要添加以下材质属性和方法：
```cpp
class Material {
public:
    // 需要添加的材质
    static Material ICE;          // 冰
    static Material PORTAL;       // 传送门
    static Material STRUCTURE_VOID; // 结构空位
    static Material OCEAN_PLANT;  // 海洋植物
    static Material SEA_GRASS;    // 海草

    // 需要添加的方法
    [[nodiscard]] bool isFlammable() const;
    [[nodiscard]] bool isReplaceable() const;  // 可被流体替换
};
```

#### 1.4 CollisionShape 碰撞形状完善
**文件**: `src/common/physics/collision/CollisionShape.hpp`

需要确保以下方法正确实现：
- `static CollisionShape fullBlock()` - 返回完整方块碰撞箱 (0,0,0)-(1,1,1)
- `static CollisionShape empty()` - 返回空碰撞箱
- `static CollisionShape box(f32 x1, f32 y1, f32 z1, f32 x2, f32 y2, f32 z2)` - 创建自定义碰撞箱

---

### 阶段2：流体方块系统

#### 2.1 LiquidBlock 流体方块基类
**新建文件**: `src/common/world/block/LiquidBlock.hpp/cpp`

参考 MC 1.16.5 `FlowingFluidBlock` / `LiquidBlock`

```cpp
/**
 * @brief 流体方块基类
 *
 * 所有流体方块（水、岩浆）的基类。
 * 管理流体状态与方块状态的映射。
 */
class LiquidBlock : public Block {
public:
    // 流体方块需要持有对应的流体引用
    LiquidBlock(const ResourceLocation& id,
                fluid::FlowingFluid& sourceFluid,
                fluid::FlowingFluid& flowingFluid,
                const BlockProperties& properties);

    // 重写Block方法
    [[nodiscard]] const fluid::FluidState* getFluidState(const BlockState& state) const override;
    [[nodiscard]] bool propagatesSkylightDown() const override;
    [[nodiscard]] CollisionShape getCollisionShape(const BlockState& state) const override;

    // 流体方块特有方法
    void onBlockAdded(IWorld& world, const BlockPos& pos, const BlockState& state) override;
    void onBlockRemoved(IWorld& world, const BlockPos& pos, const BlockState& state) override;

protected:
    fluid::FlowingFluid* m_source;
    fluid::FlowingFluid* m_flowing;
};
```

#### 2.2 VanillaBlocks 更新
**文件**: `src/common/world/block/VanillaBlocks.hpp/cpp`

需要添加：
```cpp
// 流体方块
static Block* WATER;           // 水源头方块
static Block* FLOWING_WATER;   // 流动水方块（可能不需要，水方块自动处理）
static Block* LAVA;            // 岩浆源头方块
static Block* FLOWING_LAVA;    // 流动岩浆方块（同上）
```

---

### 阶段3：流体核心逻辑完善

#### 3.1 FlowingFluid 完善
**文件**: `src/common/world/fluid/FlowingFluid.cpp`

当前已有实现，需要修复以下问题：

1. **doesSideHaveHoles()** - 简化实现，需要使用 VoxelShapes.doAdjacentCubeSidesFillSquare 的正确逻辑
   ```cpp
   // 参考 MC 1.16.5: VoxelShapes.doAdjacentCubeSidesFillSquare
   // 需要检查两个碰撞箱在相邻面是否有空隙
   ```

2. **isBlocked()** - 需要添加特殊方块检测
   ```cpp
   // 检测门、告示牌、梯子、甘蔗、气泡柱
   // 检测 ILiquidContainer 接口
   ```

3. **flowInto()** - 需要处理 ILiquidContainer
   ```cpp
   void FlowingFluid::flowInto(IWorld& world, const BlockPos& pos,
                                const BlockState* blockState, Direction dir,
                                const FluidState& state) {
       if (blockState != nullptr) {
           const Block& block = blockState->owner();
           if (auto* container = dynamic_cast<const ILiquidContainer*>(&block)) {
               // ILiquidContainer 特殊处理
               if (container->canContainFluid(world, pos, *blockState, state.getFluid())) {
                   container->receiveFluid(world, pos, blockState, state);
                   return;
               }
           }
           if (!blockState->isAir()) {
               beforeReplacingBlock(world, pos, blockState);
           }
       }
       // 设置方块...
   }
   ```

#### 3.2 LavaFluid 完善
**文件**: `src/common/world/fluid/fluids/LavaFluid.cpp`

需要实现：
1. **维度感知的 tick 延迟**
   ```cpp
   [[nodiscard]] i32 getTickDelay(IWorld& world) const override {
       // 主世界: 30 tick
       // 下界: 10 tick
       return world.dimension() == DimensionId::NETHER ? 10 : 30;
   }
   ```

2. **维度感知的流动距离和衰减**
   ```cpp
   [[nodiscard]] i32 getLevelDecrease(IWorld& world) const override {
       return world.dimension() == DimensionId::NETHER ? 1 : 2;
   }

   [[nodiscard]] i32 getSpreadDistance(IWorld& world) const override {
       return world.dimension() == DimensionId::NETHER ? 6 : 4;
   }
   ```

3. **randomTick() 火焰生成** - 放置火方块的逻辑

4. **checkForMixing()** - 岩浆遇水生成石头/黑曜石

#### 3.3 WaterFluid 完善
**文件**: `src/common/world/fluid/fluids/WaterFluid.cpp`

需要实现：
1. **beforeReplacingBlock()** - 掉落方块物品

---

### 阶段4：Tick调度系统

#### 4.1 流体Tick调度器
**新建文件**: `src/common/world/tick/FluidTickList.hpp/cpp`

```cpp
/**
 * @brief 流体Tick调度器
 *
 * 管理 Fluid 的延迟 tick 调度。
 * 参考 MC 1.16.5 ITickList<Fluid>
 */
class FluidTickList {
public:
    /**
     * @brief 调度流体tick
     * @param pos 位置
     * @param fluid 流体类型
     * @param delay 延迟tick数
     * @param priority 优先级
     */
    void scheduleTick(const BlockPos& pos, fluid::Fluid& fluid,
                      i32 delay, TickPriority priority = TickPriority::Normal);

    /**
     * @brief 检查是否有待执行的tick
     */
    [[nodiscard]] bool hasScheduledTick(const BlockPos& pos, const fluid::Fluid& fluid) const;

    /**
     * @brief 执行所有到期的tick
     */
    void tick(IWorld& world);

    // 序列化支持
    // NBT 读写...

private:
    struct ScheduledFluidTick {
        BlockPos pos;
        fluid::Fluid* fluid;
        u64 targetTick;
        TickPriority priority;
    };

    std::vector<ScheduledFluidTick> m_pendingTicks;
    std::unordered_set<std::pair<BlockPos, fluid::Fluid*>> m_scheduledPositions;
};
```

#### 4.2 IWorld 接口更新
**文件**: `src/common/world/IWorld.hpp`

需要添加：
```cpp
/**
 * @brief 获取流体Tick调度器
 */
[[nodiscard]] virtual FluidTickList& getPendingFluidTicks() = 0;
[[nodiscard]] virtual const FluidTickList& getPendingFluidTicks() const = 0;
```

---

### 阶段5：方块属性完善

#### 5.1 BlockStateProperties 扩展
**文件**: `src/common/util/property/Properties.hpp`

确保 `LEVEL_0_15` 属性存在：
```cpp
/**
 * @brief 方块等级属性 (0-15)
 *
 * 用于流体方块的水位、农作物生长阶段等。
 */
static IntegerProperty& LEVEL_0_15();
```

#### 5.2 FluidProperties 确保
**文件**: `src/common/util/property/FluidProperties.hpp`

确保以下属性已正确定义：
```cpp
/**
 * @brief 流体等级属性 (1-8)
 *
 * 流动流体的等级，8为源头，1为最远端。
 */
static IntegerProperty& LEVEL_1_8();

/**
 * @brief 流体下落属性
 *
 * 标记流体是否正在下落。
 */
static BooleanProperty& FALLING();
```

---

### 阶段6：流体与方块交互

#### 6.1 方块掉落系统
**新建文件**: `src/common/world/block/BlockDrops.hpp/cpp`

```cpp
/**
 * @brief 方块掉落工具函数
 */
namespace BlockDrops {
    /**
     * @brief 方块被破坏时掉落物品
     */
    void dropBlockAsItem(IWorld& world, const BlockPos& pos,
                         const BlockState& state, i32 fortune);

    /**
     * @brief 掉落物品实体
     */
    void spawnItemEntity(IWorld& world, const Vector3& pos,
                        const ItemStack& item);
}
```

#### 6.2 火方块实现
**新建文件**: `src/common/world/block/blocks/FireBlock.hpp/cpp`

岩浆的 `randomTick` 需要放置火方块。

---

### 阶段7：性能优化

#### 7.1 碰撞形状缓存
**文件**: `src/common/world/fluid/FlowingFluid.hpp`

```cpp
// 在 FlowingFluid 类中添加
private:
    mutable std::unordered_map<u32, CollisionShape> m_shapeCache;
```

#### 7.2 方块渲染侧面缓存
参考 MC 1.16.5 的 `ThreadLocal<Object2ByteLinkedOpenHashMap<Block.RenderSideCacheKey>>`

---

### 阶段8：客户端功能（可选，后续）

#### 8.1 流体渲染相关
- `shouldRenderSides()` - 判断哪些面需要渲染
- `getDripParticleData()` - 滴落粒子
- `animateTick()` - 客户端动画效果

#### 8.2 桶装物品
- `getFilledBucket()` - 获取桶装物品

---

## 实现优先级

### P0 - 核心功能（必须先完成）
1. **Material 完善** - 添加 ICE, PORTAL, isFlammable 等
2. **BlockState 扩展** - getCollisionShape, isSolidSide, isSolid
3. **CollisionShape** - 确保 fullBlock() 和 box() 正确
4. **ILiquidContainer** - 确保接口可用

### P1 - 流体方块
5. **LiquidBlock** - 流体方块基类
6. **VanillaBlocks** - 添加 WATER, LAVA 方块

### P2 - 流动逻辑
7. **FlowingFluid** - 完善 doesSideHaveHoles, isBlocked
8. **LavaFluid** - 维度感知参数、火焰生成、水交互
9. **WaterFluid** - 方块掉落

### P3 - Tick系统
10. **FluidTickList** - 流体tick调度器
11. **IWorld 更新** - 添加 getPendingFluidTicks

### P4 - 其他
12. **BlockDrops** - 方块掉落系统
13. **FireBlock** - 火方块
14. **性能优化** - 形状缓存

---

## 目录结构规划

```
src/common/world/
├── fluid/                      # 流体子系统
│   ├── Fluid.hpp/cpp          # 流体基类
│   ├── FluidState.hpp         # 流体状态（在Fluid.hpp中）
│   ├── FlowingFluid.hpp/cpp   # 流动流体
│   ├── FluidRegistry.hpp/cpp  # 流体注册表
│   ├── FluidTags.hpp/cpp      # 流体标签
│   └── fluids/                # 具体流体
│       ├── EmptyFluid.hpp/cpp
│       ├── WaterFluid.hpp/cpp
│       └── LavaFluid.hpp/cpp
├── block/                      # 方块子系统
│   ├── Block.hpp/cpp          # 方块基类
│   ├── BlockState.hpp/cpp     # 方块状态
│   ├── BlockRegistry.hpp/cpp  # 方块注册表
│   ├── Material.hpp/cpp       # 材质定义
│   ├── ILiquidContainer.hpp   # 液体容器接口
│   ├── LiquidBlock.hpp/cpp    # 流体方块基类（新增）
│   ├── VanillaBlocks.hpp/cpp  # 原版方块
│   └── blocks/                # 具体方块
│       ├── FireBlock.hpp/cpp  # 火方块（新增）
│       └── ...
├── tick/                       # Tick调度系统
│   ├── TickPriority.hpp       # 优先级枚举
│   ├── BlockTickList.hpp/cpp  # 方块tick（新增）
│   └── FluidTickList.hpp/cpp  # 流体tick（新增）
└── IWorld.hpp/cpp             # 世界接口
```

---

## 单元测试需求

每个新增类需要配套单元测试，覆盖率 95%+：

1. **FluidStateTest** - 流体状态创建、属性访问、状态转换
2. **FluidRegistryTest** - 注册、查找、遍历
3. **FlowingFluidTest** - 流动逻辑、源头形成、衰减计算
4. **WaterFluidTest** - 水流动、无限源形成
5. **LavaFluidTest** - 岩浆流动、火焰生成、水交互
6. **FluidTickListTest** - 调度、执行、优先级
7. **LiquidBlockTest** - 方块-流体映射、放置、移除

---

## 预计工作量

| 阶段 | 预计时间 | 依赖 |
|------|----------|------|
| P0 核心依赖 | 2-3天 | 无 |
| P1 流体方块 | 1-2天 | P0 |
| P2 流动逻辑 | 2-3天 | P1 |
| P3 Tick系统 | 1天 | P2 |
| P4 其他 | 1-2天 | P3 |
| **总计** | **7-11天** | |

---

## 注意事项

1. **命名空间** - 所有代码使用 `mc::fluid` 命名空间
2. **注释规范** - 每个公共方法必须有 Doxygen 注释
3. **MC兼容性** - 行为必须与 MC 1.16.5 一致
4. **性能考虑** - 流体计算频繁，需要缓存优化
5. **线程安全** - Tick调度器可能在多线程环境使用
