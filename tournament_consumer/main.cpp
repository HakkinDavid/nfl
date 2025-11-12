//
// Created by tomas on 9/6/25.
//
#include <activemq/library/ActiveMQCPP.h>
#include <thread>

#include "configuration/ContainerSetup.hpp"

int main() {
    activemq::library::ActiveMQCPP::initializeLibrary();
    {
        std::println("before container");
        const auto container = config::containerSetup();
        std::println("after container");

        std::thread tournamentReadyThread([&] {
            auto listener = container->resolve<TournamentReadyListener>();
            listener->Start("tournament.ready");
        });
        std::thread matchScoreUpdatedThread([&] {
            auto listener = container->resolve<MatchScoreUpdatedListener>();
            listener->Start("match.score.updated");
        });
        //crear otro thread aqui

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