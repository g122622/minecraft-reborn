#pragma once

#include "Goal.hpp"
#include <memory>

namespace mc::entity::ai {

/**
 * @brief 带优先级的目标包装器
 *
 * 包装一个 Goal 并添加优先级信息。
 * 优先级数值越小，优先级越高。
 *
 * 参考 MC 1.16.5 PrioritizedGoal
 */
class PrioritizedGoal : public Goal {
public:
    /**
     * @brief 构造函数
     * @param priority 优先级（数值越小优先级越高）
     * @param goal 被包装的目标
     */
    PrioritizedGoal(int priority, std::unique_ptr<Goal> goal)
        : m_priority(priority)
        , m_inner(std::move(goal))
        , m_running(false)
    {}

    /**
     * @brief 构造函数（从原始指针）
     * @param priority 优先级
     * @param goal 被包装的目标（获取所有权）
     */
    PrioritizedGoal(int priority, Goal* goal)
        : m_priority(priority)
        , m_inner(goal)
        , m_running(false)
    {}

    /**
     * @brief 是否可以被另一个目标抢占
     * @param other 要比较的目标
     * @return true 如果当前目标可以被抢占
     */
    [[nodiscard]] bool isPreemptedBy(const PrioritizedGoal& other) const {
        return isPreemptible() && other.m_priority < m_priority;
    }

    // ========== Goal 接口实现 ==========

    [[nodiscard]] bool shouldExecute() override {
        return m_inner->shouldExecute();
    }

    [[nodiscard]] bool shouldContinueExecuting() override {
        return m_inner->shouldContinueExecuting();
    }

    [[nodiscard]] bool isPreemptible() const override {
        return m_inner->isPreemptible();
    }

    void startExecuting() override {
        if (!m_running) {
            m_running = true;
            m_inner->startExecuting();
        }
    }

    void resetTask() override {
        if (m_running) {
            m_running = false;
            m_inner->resetTask();
        }
    }

    void tick() override {
        m_inner->tick();
    }

    void setMutexFlags(const EnumSet<GoalFlag>& flags) {
        m_inner->setMutexFlags(flags);
    }

    [[nodiscard]] const EnumSet<GoalFlag>& getMutexFlags() const {
        return m_inner->getMutexFlags();
    }

    [[nodiscard]] String getTypeName() const override {
        return m_inner->getTypeName();
    }

    // ========== PrioritizedGoal 特有方法 ==========

    /**
     * @brief 获取优先级
     */
    [[nodiscard]] int getPriority() const {
        return m_priority;
    }

    /**
     * @brief 是否正在运行
     */
    [[nodiscard]] bool isRunning() const {
        return m_running;
    }

    /**
     * @brief 获取内部目标
     */
    [[nodiscard]] Goal* getGoal() {
        return m_inner.get();
    }

    /**
     * @brief 获取内部目标（const版本）
     */
    [[nodiscard]] const Goal* getGoal() const {
        return m_inner.get();
    }

    /**
     * @brief 相等比较
     */
    [[nodiscard]] bool operator==(const PrioritizedGoal& other) const {
        return m_inner.get() == other.m_inner.get();
    }

    /**
     * @brief 不等比较
     */
    [[nodiscard]] bool operator!=(const PrioritizedGoal& other) const {
        return !(*this == other);
    }

private:
    int m_priority;                  // 优先级
    std::unique_ptr<Goal> m_inner;   // 内部目标
    bool m_running;                  // 是否正在运行
};

} // namespace mc::entity::ai
