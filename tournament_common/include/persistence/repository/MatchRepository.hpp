//
// Created by root on 11/4/25.
//

#ifndef NFL_MATCHREPOSITORY_HPP
#define NFL_MATCHREPOSITORY_HPP

#include "persistence/repository/IMatchRepository.hpp"
#include "persistence/configuration/IDbConnectionProvider.hpp"

class MatchRepository : public IMatchRepository {
    std::shared_ptr<IDbConnectionProvider> connectionProvider;
public:
    explicit MatchRepository(std::shared_ptr<IDbConnectionProvider> provider);

    // Implementaciones de IRepository
    std::shared_ptr<domain::Match> ReadById(std::string id) override;
    std::string Create(const domain::Match& entity) override;
    std::string Update(const domain::Match& entity) override; // Usado para lógica interna, no para PATCH
    void Delete(std::string id) override;
    std::vector<std::shared_ptr<domain::Match>> ReadAll() override;

    //  Implementaciones de IMatchRepository
    std::vector<std::shared_ptr<domain::Match>> FindAllByTournamentId(std::string_view tournamentId) override;
    std::shared_ptr<domain::Match> FindByIdAndTournamentId(std::string_view matchId, std::string_view tournamentId) override;
    std::string UpdateScore(std::string_view matchId, std::string_view tournamentId, const domain::Score& score) override;
    std::vector<std::shared_ptr<domain::Match>> GetMatchesByTeamId(std::string_view tournamentId, std::string_view teamId) override;
    std::vector<std::shared_ptr<domain::Match>> FindMatchesByTournamentAndRound(std::string_view tournamentId, std::string_view round) override;
};

#endif //NFL_MATCHREPOSITORY_HPP