//
// Created by developer on 10/14/25.
//

#ifndef TOURNAMENTS_IMATCHREPOSITORY_HPP
#define TOURNAMENTS_IMATCHREPOSITORY_HPP

#include <string_view>
#include <vector>
#include <memory>

#include "domain/Match.hpp"
#include "persistence/repository/IRepository.hpp"

class IMatchRepository : public IRepository<domain::Match, std::string> {
public:
    //Find match with only one team to be added
    virtual std::shared_ptr<domain::Match> FindLastOpenMatch(std::string_view tournamentId) = 0;
    virtual std::vector<std::shared_ptr<domain::Match>> FindMatchesByTournamentAndRound(std::string_view tournamentId, std::string_view round) = 0;
    virtual std::vector<std::shared_ptr<domain::Match>> FindAllByTournamentId(std::string_view tournamentId) = 0;
    virtual std::shared_ptr<domain::Match> FindByIdAndTournamentId(std::string_view matchId, std::string_view tournamentId) = 0;
    virtual std::string UpdateScore(std::string_view matchId, std::string_view tournamentId, const domain::Score& score) = 0;
};
#endif //TOURNAMENTS_IMATCHREPOSITORY_HPP