#pragma once

namespace mc::client::ui::kagero::tpl {

/**
 * @brief 初始化模板系统
 *
 * 注册所有内置Widget工厂、属性设置器和事件绑定器。
 * 在程序启动时调用一次。
 *
 * @note 此函数是幂等的，多次调用不会有副作用
 */
void initializeTemplateSystem();

/**
 * @brief 关闭模板系统
 *
 * 清理模板系统资源。
 * 在程序退出时调用。
 */
void shutdownTemplateSystem();

/**
 * @brief 检查模板系统是否已初始化
 *
 * @return 是否已初始化
 */
[[nodiscard]] bool isTemplateSystemInitialized();

} // namespace mc::client::ui::kagero::tpl
