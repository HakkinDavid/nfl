//
// Created by developer on 10/14/25.
//

#ifndef LISTENER_TOURNAMENTREADY_LISTENER_HPP
#define LISTENER_TOURNAMENTREADY_LISTENER_HPP
#include "QueueMessageListener.hpp"
#include "delegate/MatchDelegate.hpp"

class TournamentReadyListener : public QueueMessageListener{
    std::shared_ptr<MatchDelegate> matchDelegate;

    void processMessage(const std::string& message) override;
public:
    TournamentReadyListener(const std::shared_ptr<ConnectionManager>& connectionManager,
                            const std::shared_ptr<MatchDelegate>& matchDelegate) 
        : QueueMessageListener(connectionManager),
          matchDelegate(matchDelegate) {}
    ~TournamentReadyListener() override { Stop(); }
};

inline void TournamentReadyListener::processMessage(const std::string &message) {
    try {
        std::println("TournamentReadyListener received message: {}", message);
        // tournament_services sends plain string (tournament ID), not JSON
        std::string tournamentId = message;

        std::println("Creating first round matches for tournament: {}", tournamentId);
        auto result = matchDelegate->createFirstRoundMatches(tournamentId);

        if (!result.has_value()) {
            std::println("TournamentReadyListener error: {}", result.error());
        } else {
            std::println("First round matches created successfully for tournament: {}", tournamentId);
        }
    } catch (const std::exception& e) {
        std::println("Error in TournamentReadyListener: {} - Message was: {}", e.what(), message);
    }
}


#endif //LISTENER_TOURNAMENTREADY_LISTENER_HPP