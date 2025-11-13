//
// Created by tomas on 9/14/25.
//

#ifndef SERVICES_CONNECTION_MANAGER_HPP
#define SERVICES_CONNECTION_MANAGER_HPP

#include <cms/Connection.h>
#include <cms/Session.h>
#include <activemq/core/ActiveMQConnectionFactory.h>
#include <memory>
#include <mutex>
#include <stdexcept>

class ConnectionManager {
public:
    void initialize(const std::string_view& brokerURI) {
        std::lock_guard<std::mutex> lock(sessionMutex);
        if (connection) {
            return; // Already initialized
        }
        factory = std::make_unique<activemq::core::ActiveMQConnectionFactory>(brokerURI.data());
        connection = std::shared_ptr<cms::Connection>(factory->createConnection());

        connection->start();
    }

    [[nodiscard]] std::shared_ptr<cms::Connection> Connection() const { return connection; }

    [[nodiscard]] std::shared_ptr<cms::Session> CreateSession() {
        std::lock_guard<std::mutex> lock(sessionMutex);
        if (!connection) {
            throw std::runtime_error("ConnectionManager not initialized - connection is null");
        }
        return std::shared_ptr<cms::Session>(connection->createSession(cms::Session::AUTO_ACKNOWLEDGE));
    }

private:
    std::unique_ptr<activemq::core::ActiveMQConnectionFactory> factory;
    std::shared_ptr<cms::Connection> connection;
    std::mutex sessionMutex;
};

#endif //SERVICES_CONNECTION_MANAGER_HPP