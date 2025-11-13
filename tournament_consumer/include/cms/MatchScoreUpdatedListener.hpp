#ifndef LISTENER_MATCHSCOREUPDATED_LISTENER_HPP
#define LISTENER_MATCHSCOREUPDATED_LISTENER_HPP

#include "QueueMessageListener.hpp"
#include "delegate/MatchDelegate.hpp"

class MatchScoreUpdatedListener : public QueueMessageListener {
    std::shared_ptr<MatchDelegate> matchDelegate;

    void processMessage(const std::string& message) override;
public:
    MatchScoreUpdatedListener(const std::shared_ptr<ConnectionManager>& connectionManager,
                              const std::shared_ptr<MatchDelegate>& matchDelegate)
        : QueueMessageListener(connectionManager),
          matchDelegate(matchDelegate) {}
    ~MatchScoreUpdatedListener() override { Stop(); }
};

inline void MatchScoreUpdatedListener::processMessage(const std::string& message) {
    try {
        std::println("MatchScoreUpdatedListener received message: {}", message);
        auto json = nlohmann::json::parse(message);
        std::string matchId = json.at("match_id");
        std::string tournamentId = json.at("tournament_id");

        auto result = matchDelegate->generateNextRound(matchId, tournamentId);
        if (!result.has_value()) {
            std::println("MatchScoreUpdatedListener error: {}", result.error());
        }
    } catch (const nlohmann::json::exception& e) {
        std::println("JSON parse error in MatchScoreUpdatedListener: {} - Message was: {}", e.what(), message);
    } catch (const std::exception& e) {
        std::println("Error in MatchScoreUpdatedListener: {} - Message was: {}", e.what(), message);
    }
}

#endif // LISTENER_MATCHSCOREUPDATED_LISTENER_HPP
