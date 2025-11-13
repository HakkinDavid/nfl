//
// Created by tomas on 9/6/25.
//
#include <activemq/library/ActiveMQCPP.h>
#include <thread>
#include <iostream>

#include "configuration/ContainerSetup.hpp"

int main() {
    std::cout << "Starting tournament_consumer..." << std::endl;
    std::cout << "Initializing ActiveMQ library..." << std::endl;
    std::cout.flush();
    activemq::library::ActiveMQCPP::initializeLibrary();
    std::cout << "ActiveMQ initialized successfully" << std::endl;
    {
        std::println("before container");
        auto container = config::containerSetup();
        std::println("after container");

        std::cout << "Creating tournamentReadyThread..." << std::endl;
        std::thread tournamentReadyThread([container] {
            std::cout << "[Thread1] Starting, resolving TournamentReadyListener..." << std::endl;
            auto listener = container->resolve<TournamentReadyListener>();
            std::cout << "[Thread1] Resolved, calling Start()..." << std::endl;
            listener->Start("tournament.ready");
            std::cout << "[Thread1] Start() completed" << std::endl;
        });

        std::cout << "Creating matchScoreUpdatedThread..." << std::endl;
        std::thread matchScoreUpdatedThread([container] {
            std::cout << "[Thread2] Starting, resolving MatchScoreUpdatedListener..." << std::endl;
            auto listener = container->resolve<MatchScoreUpdatedListener>();
            std::cout << "[Thread2] Resolved, calling Start()..." << std::endl;
            listener->Start("match.score.updated");
            std::cout << "[Thread2] Start() completed" << std::endl;
        });
        //crear otro thread aqui

        std::cout << "Joining threads..." << std::endl;
        tournamentReadyThread.join();
        matchScoreUpdatedThread.join();
        //join de otro thread aqui
        // while (true) {
        //     std::this_thread::sleep_for(std::chrono::seconds(5));
        // }
    }
    activemq::library::ActiveMQCPP::shutdownLibrary();
    return 0;
}