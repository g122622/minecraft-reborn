#include "CommandRegistry.hpp"
#include "commands/GameModeCommand.hpp"
#include "commands/TimeCommand.hpp"
#include "commands/KillCommand.hpp"
#include "commands/ListCommand.hpp"
#include "commands/HelpCommand.hpp"
#include "commands/SeedCommand.hpp"
#include "commands/TeleportCommand.hpp"
#include "commands/GiveCommand.hpp"
#include "commands/ClearCommand.hpp"

namespace mr {
namespace command {

CommandRegistry::CommandRegistry()
    : m_dispatcher()
{
}

Result<i32> CommandRegistry::execute(const String& input, ServerCommandSource& source) {
    auto result = m_dispatcher.execute(input, source);
    if (result.success()) {
        return result.value().result();
    }
    return Error(ErrorCode::Unknown, "Command execution failed");
}

void CommandRegistry::registerDefaults() {
    // 注册核心命令
    GameModeCommand::registerTo(m_dispatcher);
    TimeCommand::registerTo(m_dispatcher);
    KillCommand::registerTo(m_dispatcher);
    ListCommand::registerTo(m_dispatcher);
    HelpCommand::registerTo(m_dispatcher);
    SeedCommand::registerTo(m_dispatcher);
    TeleportCommand::registerTo(m_dispatcher);
    GiveCommand::registerTo(m_dispatcher);
    ClearCommand::registerTo(m_dispatcher);

    // 记录命令名称
    m_commandNames = {
        "gamemode", "time", "kill", "list", "help", "seed", "tp", "give", "clear"
    };
}

std::vector<String> CommandRegistry::getCommandNames() const {
    return m_commandNames;
}

bool CommandRegistry::hasCommand(const String& name) const {
    return std::find(m_commandNames.begin(), m_commandNames.end(), name) != m_commandNames.end();
}

} // namespace command
} // namespace mr
