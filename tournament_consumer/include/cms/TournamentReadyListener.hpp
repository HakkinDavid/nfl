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
    TournamentReadyListener(const std::shared_ptr<ConnectionManager> &connectionManager);
    ~TournamentReadyListener() override;

};

inline TournamentReadyListener::TournamentReadyListener(const std::shared_ptr<ConnectionManager> &connectionManager)
    : QueueMessageListener(connectionManager) {
}

inline TournamentReadyListener::~TournamentReadyListener() {
    Stop();
}

inline void TournamentReadyListener::processMessage(const std::string &message) {
    try {
        auto json = nlohmann::json::parse(message);
        std::string tournamentId = json.at("tournament_id");

        matchDelegate->CreateFirstRoundMatches(tournamentId);
    } catch (const std::exception& e) {
        std::println("TournamentReadyListener error: {}", e.what());
    }
}


#endif //LISTENER_TOURNAMENTREADY_LISTENER_HPP