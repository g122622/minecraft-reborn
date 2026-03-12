#include "LocalServerConnection.hpp"
#include <spdlog/spdlog.h>

namespace mr::network {

LocalServerConnection::LocalServerConnection(LocalEndpoint* endpoint)
    : m_endpoint(endpoint)
    , m_connected(endpoint != nullptr && endpoint->isConnected())
    , m_id(++s_nextId)
{
}

void LocalServerConnection::send(const u8* data, size_t size) {
    if (m_endpoint && m_connected) {
        m_endpoint->send(data, size);
    }
}

void LocalServerConnection::disconnect(const String& reason) {
    if (m_endpoint && m_connected) {
        m_endpoint->disconnect();
        m_connected = false;
        if (!reason.empty()) {
            spdlog::debug("LocalServerConnection {} disconnected: {}", m_id, reason);
        }
    }
}

bool LocalServerConnection::isConnected() const {
    return m_endpoint && m_connected && m_endpoint->isConnected();
}

String LocalServerConnection::identifier() const {
    return "Local:" + std::to_string(m_id);
}

ConnectionType LocalServerConnection::type() const {
    return ConnectionType::Local;
}

} // namespace mr::network
