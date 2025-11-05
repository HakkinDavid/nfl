#ifndef DOMAIN_MATCH_HPP
#define DOMAIN_MATCH_HPP

#include <string>
#include <optional>
#include <vector>
#include "domain/Team.hpp"

namespace domain {
    enum class Winner { HOME, VISITOR };

    struct Score {
        int homeTeamScore;
        int visitorTeamScore;

        [[nodiscard]] Winner GetWinner() const {
            if (visitorTeamScore < homeTeamScore) {
                return Winner::HOME;
            }
            return Winner::VISITOR;
        }
    };

    class Match {
        std::string id;
        std::string tournamentId;
        Team home;
        Team visitor;
        std::string round;
        std::optional<domain::Score> score;

    public:
        // Getters y Setters
        [[nodiscard]] std::string Id() const { return id; }
        std::string& Id() { return id; }

        [[nodiscard]] std::string TournamentId() const { return tournamentId; }
        std::string& TournamentId() { return tournamentId; }

        [[nodiscard]] Team Home() const { return home; }
        Team& Home() { return home; }

        [[nodiscard]] Team Visitor() const { return visitor; }
        Team& Visitor() { return visitor; }

        [[nodiscard]] std::string Round() const { return round; }
        std::string& Round() { return round; }

        [[nodiscard]] std::optional<domain::Score> Score() const { return score; }
        std::optional<domain::Score>& Score() { return score; }
    };
}

#endif