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
    auto json = nlohmann::json::parse(message);
    std::string tournamentId = json.at("tournament_id");

    auto result = matchDelegate->createFirstRoundMatches(tournamentId);
    
    if (!result.has_value()) {
        std::println("TournamentReadyListener error: {}", result.error());
    }
}


#endif //LISTENER_TOURNAMENTREADY_LISTENER_HPP