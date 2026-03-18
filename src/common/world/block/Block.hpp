#pragma once

#include "../../core/Types.hpp"
#include "../../resource/ResourceLocation.hpp"
#include "../../physics/collision/CollisionShape.hpp"
#include "../../util/property/StateHolder.hpp"
#include "../../util/property/StateContainer.hpp"
#include "../../util/Direction.hpp"
#include "Material.hpp"
#include <memory>
#include <vector>
#include <unordered_map>
#include <functional>

namespace mc {

// Forward declarations
class Block;
class BlockState;
class BlockRegistry;
class IWorld;
class BlockPos;
class IRandom;

namespace fluid {
class FluidState;
} // namespace fluid

/**
 * @brief VoxelShape工具类
 *
 * 提供常用碰撞形状的静态实例。
 */
class VoxelShapes {
public:
    /**
     * @brief 获取空形状
     */
    static const CollisionShape& empty();

    /**
     * @brief 获取完整方块形状
     */
    static const CollisionShape& fullCube();

    /**
     * @brief 创建方块形状
     * @param x1, y1, z1 起始坐标 (像素，0-16)
     * @param x2, y2, z2 结束坐标 (像素，0-16)
     */
    static CollisionShape cube(f32 x1, f32 y1, f32 z1, f32 x2, f32 y2, f32 z2);
};

/**
 * @brief 方块状态
 *
 * 不可变的方块状态对象，包含方块的所有属性值。
 * 继承自StateHolder以支持O(1)的状态转换。
 *
 * 参考: net.minecraft.block.BlockState
 */
class BlockState : public StateHolder<Block, BlockState> {
public:
    /**
     * @brief 构造方块状态
     */
    BlockState(const Block& block,
               std::unordered_map<const IProperty*, size_t> values,
               u32 stateId);

    /**
     * @brief 是否为空气
     */
    [[nodiscard]] bool isAir() const;

    /**
     * @brief 是否为固体
     */
    [[nodiscard]] bool isSolid() const { return m_isSolid; }

    /**
     * @brief 是否不透明
     */
    [[nodiscard]] bool isOpaque() const { return m_isOpaque; }

    /**
     * @brief 是否透明
     */
    [[nodiscard]] bool isTransparent() const { return !m_isOpaque; }

    /**
     * @brief 是否阻挡移动
     */
    [[nodiscard]] bool blocksMovement() const { return m_blocksMovement; }

    /**
     * @brief 是否为液体
     */
    [[nodiscard]] bool isLiquid() const { return m_isLiquid; }

    /**
     * @brief 是否可燃
     */
    [[nodiscard]] bool isFlammable() const { return m_isFlammable; }

    /**
     * @brief 获取光照等级
     */
    [[nodiscard]] u8 lightLevel() const { return m_lightLevel; }

    /**
     * @brief 检查此状态是否属于指定方块
     * @param block 方块指针
     * @return 如果此状态的方块与给定方块相同则返回true
     */
    [[nodiscard]] bool is(const Block* block) const {
        return block != nullptr && &owner() == block;
    }

    /**
     * @brief 获取光照透明度 (0-15)
     *
     * 参考: net.minecraft.block.AbstractBlock.AbstractBlockState#getOpacity
     */
    [[nodiscard]] i32 getOpacity() const { return m_opacity; }

    /**
     * @brief 检查是否传播天空光向下
     */
    [[nodiscard]] bool propagatesSkylightDown() const { return m_propagatesSkylightDown; }

    /**
     * @brief 获取硬度
     */
    [[nodiscard]] f32 hardness() const { return m_hardness; }

    /**
     * @brief 获取抗性
     */
    [[nodiscard]] f32 resistance() const { return m_resistance; }

    /**
     * @brief 获取方块ID
     */
    [[nodiscard]] u32 blockId() const { return m_blockId; }

    /**
     * @brief 获取碰撞形状
     */
    [[nodiscard]] const CollisionShape& getCollisionShape() const;

    /**
     * @brief 获取渲染形状
     */
    [[nodiscard]] const CollisionShape& getShape() const;

    /**
     * @brief 获取遮挡形状
     */
    [[nodiscard]] const CollisionShape& getOcclusionShape() const;

    /**
     * @brief 获取方块资源位置
     */
    [[nodiscard]] const ResourceLocation& blockLocation() const;

    /**
     * @brief 获取流体状态
     *
     * 委托到方块的 getFluidState 方法
     */
    [[nodiscard]] const fluid::FluidState* getFluidState() const;

    /**
     * @brief 转换为模型键（用于查找模型变体）
     * @return 格式: "axis=y,facing=north" 或 "" (无属性时)
     */
    [[nodiscard]] String toModelKey() const;

protected:
    /**
     * @brief 获取拥有者名称
     */
    [[nodiscard]] String ownerName() const override;

private:
    friend class Block;
    friend class BlockRegistry;

    /**
     * @brief 缓存方块属性
     */
    void cacheProperties();

    // 缓存的属性
    bool m_isSolid = true;
    bool m_isOpaque = true;
    bool m_blocksMovement = false;
    bool m_isLiquid = false;
    bool m_isFlammable = false;
    bool m_propagatesSkylightDown = false;
    u8 m_lightLevel = 0;
    i32 m_opacity = 15;  // 默认完全不透明
    f32 m_hardness = 0.0f;
    f32 m_resistance = 0.0f;
    u32 m_blockId = 0;
};

/**
 * @brief 方块属性构建器
 *
 * 用于构建方块属性的流畅接口。
 *
 * 参考: net.minecraft.block.AbstractBlock.Properties
 *
 * 用法示例:
 * @code
 * auto properties = BlockProperties(Material::ROCK)
 *     .hardness(1.5f)
 *     .resistance(6.0f)
 *     .requiresTool();
 * @endcode
 */
class BlockProperties {
public:
    /**
     * @brief 构造方块属性
     * @param material 材质
     */
    explicit BlockProperties(const Material& material);

    /**
     * @brief 设置硬度
     */
    BlockProperties& hardness(f32 value);

    /**
     * @brief 设置抗性
     */
    BlockProperties& resistance(f32 value);

    /**
     * @brief 设置光照等级
     */
    BlockProperties& lightLevel(u8 level);

    /**
     * @brief 设置无碰撞
     */
    BlockProperties& noCollision();

    /**
     * @brief 设置非固体
     */
    BlockProperties& notSolid();

    /**
     * @brief 设置需要工具
     */
    BlockProperties& requiresTool();

    /**
     * @brief 设置可燃性
     */
    BlockProperties& flammable(bool value = true);

    /**
     * @brief 设置可替换
     */
    BlockProperties& replaceable();

    /**
     * @brief 设置强度（同时设置硬度和抗性）
     */
    BlockProperties& strength(f32 value);

    /**
     * @brief 设置光照透明度
     *
     * @param value 透明度值 (0-15)
     *   - 0: 完全透明，光线无衰减通过
     *   - 1-14: 部分透明，光线衰减指定等级
     *   - 15: 完全不透明，阻挡所有光线
     *
     * 参考: net.minecraft.block.AbstractBlock.Properties#opacity
     */
    BlockProperties& opacity(i32 value);

    /**
     * @brief 设置是否传播天空光向下
     *
     * 某些方块（如树叶、冰、水）会使天空光衰减1级后传播，
     * 而不是完全阻挡或无衰减传播。
     *
     * 参考: net.minecraft.block.AbstractBlock.Properties#propagatesSkylightDown
     */
    BlockProperties& propagatesSkylightDown(bool value = true);

    // Getters
    [[nodiscard]] const Material& material() const { return m_material; }
    [[nodiscard]] f32 hardness() const { return m_hardness; }
    [[nodiscard]] f32 resistance() const { return m_resistance; }
    [[nodiscard]] u8 lightLevel() const { return m_lightLevel; }
    [[nodiscard]] bool hasCollision() const { return m_hasCollision; }
    [[nodiscard]] bool isSolid() const { return m_isSolid; }
    [[nodiscard]] bool isFlammable() const { return m_isFlammable; }
    [[nodiscard]] bool requiresTool() const { return m_requiresTool; }
    [[nodiscard]] bool isReplaceable() const { return m_isReplaceable; }
    [[nodiscard]] i32 opacity() const { return m_opacity; }
    [[nodiscard]] bool doesPropagateSkylightDown() const { return m_propagatesSkylightDown; }

private:
    friend class Block;
    friend class BlockRegistry;

    Material m_material;
    f32 m_hardness;
    f32 m_resistance;
    u8 m_lightLevel;
    bool m_hasCollision;
    bool m_isSolid;
    bool m_isFlammable;
    bool m_requiresTool;
    bool m_isReplaceable;
    i32 m_opacity = 15;  // 默认完全不透明
    bool m_propagatesSkylightDown = false;
};

/**
 * @brief 方块基类
 *
 * 所有方块类型的基类。方块通过BlockRegistry注册，
 * 每个方块有一个或多个BlockState表示不同状态。
 *
 * 参考: net.minecraft.block.Block
 *
 * 用法示例:
 * @code
 * class StoneBlock : public Block {
 * public:
 *     StoneBlock() : Block(BlockProperties(Material::ROCK).hardness(1.5f)) {
 *         auto container = StateContainer<Block, BlockState>::Builder(*this)
 *             .create([](const Block& block, auto values, u32 id) {
 *                 return std::make_unique<BlockState>(block, values, id);
 *             });
 *         createBlockState(std::move(container));
 *     }
 * };
 * @endcode
 */
class Block {
public:
    virtual ~Block() = default;

    // ========================================================================
    // 静态方法
    // ========================================================================

    /**
     * @brief 根据方块ID获取方块
     */
    [[nodiscard]] static Block* getBlock(u32 blockId);

    /**
     * @brief 根据资源位置获取方块
     */
    [[nodiscard]] static Block* getBlock(const ResourceLocation& id);

    /**
     * @brief 根据状态ID获取方块状态
     */
    [[nodiscard]] static BlockState* getBlockState(u32 stateId);

    /**
     * @brief 遍历所有方块
     */
    static void forEachBlock(std::function<void(Block&)> callback);

    /**
     * @brief 遍历所有方块状态
     */
    static void forEachBlockState(std::function<void(const BlockState&)> callback);

    // ========================================================================
    // 方块属性
    // ========================================================================

    /**
     * @brief 获取方块资源位置
     */
    [[nodiscard]] const ResourceLocation& blockLocation() const { return m_blockLocation; }

    /**
     * @brief 获取方块ID
     */
    [[nodiscard]] u32 blockId() const { return m_blockId; }

    /**
     * @brief 获取材质
     */
    [[nodiscard]] const Material& material() const { return m_material; }

    /**
     * @brief 获取状态容器
     */
    [[nodiscard]] const StateContainer<Block, BlockState>& stateContainer() const { return *m_stateContainer; }

    /**
     * @brief 获取默认状态
     */
    [[nodiscard]] const BlockState& defaultState() const { return *m_defaultState; }

    /**
     * @brief 获取硬度
     */
    [[nodiscard]] f32 hardness() const { return m_hardness; }

    /**
     * @brief 获取抗性
     */
    [[nodiscard]] f32 resistance() const { return m_resistance; }

    /**
     * @brief 获取光照等级
     */
    [[nodiscard]] u8 lightLevel() const { return m_lightLevel; }

    /**
     * @brief 获取光照透明度 (0-15)
     */
    [[nodiscard]] i32 opacity() const { return m_opacity; }

    /**
     * @brief 检查是否传播天空光向下
     */
    [[nodiscard]] bool doesPropagateSkylightDown() const { return m_propagatesSkylightDown; }

    // ========================================================================
    // 虚方法
    // ========================================================================

    /**
     * @brief 获取渲染形状
     * @param state 方块状态
     * @return 形状引用
     */
    [[nodiscard]] virtual const CollisionShape& getShape(const BlockState& state) const;

    /**
     * @brief 获取碰撞形状
     * @param state 方块状态
     * @return 形状引用
     */
    [[nodiscard]] virtual const CollisionShape& getCollisionShape(const BlockState& state) const;

    /**
     * @brief 获取遮挡形状
     * @param state 方块状态
     * @return 形状引用
     */
    [[nodiscard]] virtual const CollisionShape& getOcclusionShape(const BlockState& state) const;

    /**
     * @brief 是否为空气
     * @param state 方块状态
     */
    [[nodiscard]] virtual bool isAir(const BlockState& state) const;

    /**
     * @brief 是否为固体
     * @param state 方块状态
     */
    [[nodiscard]] virtual bool isSolid(const BlockState& state) const;

    /**
     * @brief 是否不透明
     * @param state 方块状态
     */
    [[nodiscard]] virtual bool isOpaque(const BlockState& state) const;

    /**
     * @brief 获取光照透明度 (0-15)
     *
     * 返回方块阻挡光线的程度：
     * - 0: 完全透明，光线无衰减通过
     * - 1-14: 部分透明，光线衰减指定等级
     * - 15: 完全不透明，阻挡所有光线
     *
     * 对于透明方块（如玻璃），返回0但仍然阻挡天空光传播。
     * 对于树叶、冰、水等，返回1-2使光线衰减。
     *
     * 参考: net.minecraft.block.BlockState#getOpacity
     *
     * @param state 方块状态
     * @param world 世界（可选，用于上下文相关透明度）
     * @param pos 位置（可选）
     * @return 光照透明度 (0-15)
     */
    [[nodiscard]] virtual i32 getOpacity(const BlockState& state,
                                          IWorld* world = nullptr,
                                          const BlockPos* pos = nullptr) const;

    /**
     * @brief 检查是否传播天空光向下
     *
     * 某些方块（如树叶、冰、水）会使天空光衰减1级后传播，
     * 而不是完全阻挡或无衰减传播。
     *
     * 参考: net.minecraft.block.BlockState#propagatesSkylightDown
     *
     * @param state 方块状态
     * @param world 世界（可选）
     * @param pos 位置（可选）
     * @return 如果天空光可以传播返回true
     */
    [[nodiscard]] virtual bool propagatesSkylightDown(const BlockState& state,
                                                       IWorld* world = nullptr,
                                                       const BlockPos* pos = nullptr) const;

    /**
     * @brief 获取流体状态
     *
     * 默认返回空流体。液体方块（LiquidBlock）会重写此方法返回对应的流体。
     *
     * @param state 方块状态
     * @return 流体状态指针
     */
    [[nodiscard]] virtual const fluid::FluidState* getFluidState(const BlockState& state) const;

    // ========================================================================
    // Tick方法
    // ========================================================================

    /**
     * @brief 执行方块计划刻
     *
     * 当方块的计划刻到期时调用。默认实现为空。
     * 需要tick行为的方块（如活塞、红石元件、农作物等）应重写此方法。
     *
     * @param world 世界引用
     * @param pos 方块位置
     * @param state 方块状态
     */
    virtual void tick(IWorld& world, const BlockPos& pos, BlockState& state);

    /**
     * @brief 执行随机刻
     *
     * 在随机刻中被调用。默认实现为空。
     * 需要随机tick行为的方块（如农作物生长、铜氧化等）应重写此方法。
     *
     * @param world 世界引用
     * @param pos 方块位置
     * @param state 方块状态
     * @param random 随机数生成器
     */
    virtual void randomTick(IWorld& world, const BlockPos& pos, BlockState& state, IRandom& random);

    /**
     * @brief 是否响应随机刻
     *
     * 返回true时，该方块会被随机刻系统选中执行randomTick。
     * 默认返回false。
     *
     * @return 是否响应随机刻
     */
    [[nodiscard]] virtual bool ticksRandomly() const { return false; }

    /**
     * @brief 转换为字符串
     */
    [[nodiscard]] virtual String toString() const {
        return m_blockLocation.toString();
    }

protected:
    friend class BlockRegistry;
    friend class BlockState;

    /**
     * @brief 构造方块
     */
    explicit Block(BlockProperties properties);

    /**
     * @brief 创建方块状态容器
     */
    void createBlockState(std::unique_ptr<StateContainer<Block, BlockState>> container);

    /**
     * @brief 设置默认状态
     */
    void setDefaultState(const BlockState& state);

    // 由BlockRegistry设置
    ResourceLocation m_blockLocation;
    u32 m_blockId = 0;

    // 由构造函数设置
    Material m_material;
    f32 m_hardness = 0.0f;
    f32 m_resistance = 0.0f;
    u8 m_lightLevel = 0;
    i32 m_opacity = 15;  // 默认完全不透明
    bool m_hasCollision = true;
    bool m_isFlammable = false;
    bool m_propagatesSkylightDown = false;

    // 由createBlockState设置
    std::unique_ptr<StateContainer<Block, BlockState>> m_stateContainer;
    const BlockState* m_defaultState = nullptr;
};

} // namespace mc
