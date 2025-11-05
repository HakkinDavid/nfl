//
// Created by root on 11/4/25.
//

#include "persistence/repository/MatchRepository.hpp"

#include "persistence/configuration/PostgresConnection.hpp"
#include "domain/Utilities.hpp"
#include <nlohmann/json.hpp>
#include <pqxx/pqxx>

MatchRepository::MatchRepository(std::shared_ptr<IDbConnectionProvider> provider) : connectionProvider(std::move(provider)) {}

std::string MatchRepository::Create(const domain::Match& entity) {
    auto pooled = connectionProvider->Connection();
    auto connection = dynamic_cast<PostgresConnection*>(&*pooled);
    nlohmann::json matchDocument = entity;

    pqxx::work tx(*(connection->connection));
    pqxx::result res = tx.exec(pqxx::prepped{"create_match"}, pqxx::params{entity.TournamentId(), matchDocument.dump()});
    tx.commit();
    return res[0]["id"].as<std::string>();
}

std::vector<std::shared_ptr<domain::Match>> MatchRepository::FindAllByTournamentId(std::string_view tournamentId) {
    auto pooled = connectionProvider->Connection();
    auto connection = dynamic_cast<PostgresConnection*>(&*pooled);
    pqxx::work tx(*(connection->connection));

    pqxx::result res = tx.exec(pqxx::prepped{"get_matches_by_tournament"}, pqxx::params{tournamentId});
    tx.commit();

    std::vector<std::shared_ptr<domain::Match>> matches;
    for (auto row : res) {
        auto jsonDocument = nlohmann::json::parse(row["document"].as<std::string>());
        auto match = std::make_shared<domain::Match>();
        jsonDocument.get_to(*match);

        match->Id() = row["id"].as<std::string>();
        matches.push_back(match);
    }
    return matches;
}

std::shared_ptr<domain::Match> MatchRepository::FindByIdAndTournamentId(std::string_view matchId, std::string_view tournamentId) {
    auto pooled = connectionProvider->Connection();
    auto connection = dynamic_cast<PostgresConnection*>(&*pooled);
    pqxx::work tx(*(connection->connection));

    pqxx::result res = tx.exec(pqxx::prepped{"get_match_by_id_and_tournament"}, pqxx::params{matchId, tournamentId});
    tx.commit();

    if (res.empty()) {
        return nullptr;
    }

    auto jsonDocument = nlohmann::json::parse(res[0]["document"].as<std::string>());
    auto match = std::make_shared<domain::Match>();
    jsonDocument.get_to(*match);

    match->Id() = res[0]["id"].as<std::string>();
    return match;
}

std::string MatchRepository::UpdateScore(std::string_view matchId, std::string_view tournamentId, const domain::Score& score) {
    auto pooled = connectionProvider->Connection();
    auto connection = dynamic_cast<PostgresConnection*>(&*pooled);
    nlohmann::json scoreDocument = score;

    pqxx::work tx(*(connection->connection));
    pqxx::result res = tx.exec(pqxx::prepped{"update_match_score"}, pqxx::params{scoreDocument.dump(), matchId, tournamentId});
    tx.commit();

    if (res.empty()) {
        throw domain::NotFoundException();
    }
    return res[0]["id"].as<std::string>();
}

std::shared_ptr<domain::Match> MatchRepository::FindLastOpenMatch(std::string_view tournamentId) {
    auto pooled = connectionProvider->Connection();
    auto connection = dynamic_cast<PostgresConnection*>(&*pooled);
    pqxx::work tx(*(connection->connection));

    pqxx::result res = tx.exec(pqxx::prepped{"find_last_open_match"}, pqxx::params{tournamentId});
    tx.commit();

    if (res.empty()) {
        return nullptr; // No se encontró ningún partido abierto
    }

    auto jsonDocument = nlohmann::json::parse(res[0]["document"].as<std::string>());
    auto match = std::make_shared<domain::Match>();
    jsonDocument.get_to(*match);
    match->Id() = res[0]["id"].as<std::string>();

    return match;
}

std::vector<std::shared_ptr<domain::Match>> MatchRepository::FindMatchesByTournamentAndRound(std::string_view tournamentId, std::string_view round) {
    auto pooled = connectionProvider->Connection();
    auto connection = dynamic_cast<PostgresConnection*>(&*pooled);
    pqxx::work tx(*(connection->connection));

    pqxx::result res = tx.exec(pqxx::prepped{"get_matches_by_round"}, pqxx::params{tournamentId, round});
    tx.commit();

    std::vector<std::shared_ptr<domain::Match>> matches;
    for (auto row : res) {
        auto jsonDocument = nlohmann::json::parse(row["document"].as<std::string>());
        auto match = std::make_shared<domain::Match>();
        jsonDocument.get_to(*match);
        match->Id() = row["id"].as<std::string>();
        matches.push_back(match);
    }

    return matches;
}


// Métodos de IRepository no implementados
std::shared_ptr<domain::Match> MatchRepository::ReadById(std::string id) { return nullptr; }
std::string MatchRepository::Update(const domain::Match& entity) { return ""; }
void MatchRepository::Delete(std::string id) {}
std::vector<std::shared_ptr<domain::Match>> MatchRepository::ReadAll() { return {}; }