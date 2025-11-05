//
// Created by root on 11/4/25.
//

#ifndef NFL_IMATCHDELEGATE_HPP
#define NFL_IMATCHDELEGATE_HPP

#include <string>
#include <string_view>
#include <vector>
#include <memory>
#include <expected>
#include "domain/Match.hpp"

class IMatchDelegate {
public:
    virtual ~IMatchDelegate() = default;

    // Para los endpoints de la API
    virtual std::expected<std::vector<domain::Match>, std::string> GetMatches(std::string_view tournamentId, std::string_view filter) = 0;
    virtual std::expected<domain::Match, std::string> GetMatch(std::string_view tournamentId, std::string_view matchId) = 0;
    virtual std::expected<void, std::string> UpdateMatchScore(std::string_view tournamentId, std::string_view matchId, const domain::Score& score) = 0;

    // Para uso interno
    virtual std::expected<std::string, std::string> CreateMatch(domain::Match& match) = 0;
    virtual std::expected<domain::Match, std::string> GetNextOpenMatch(std::string_view tournamentId) = 0;
    virtual std::expected<std::vector<domain::Match>, std::string> GetMatchesByRound(std::string_view tournamentId, std::string_view round) = 0;
};

#endif //NFL_IMATCHDELEGATE_HPP