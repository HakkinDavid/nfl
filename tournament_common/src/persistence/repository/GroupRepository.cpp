//
// Created by root on 9/27/25.
//

#include "domain/Utilities.hpp"
#include  "persistence/repository/GroupRepository.hpp"

GroupRepository::GroupRepository(const std::shared_ptr<IDbConnectionProvider>& connectionProvider) : connectionProvider(std::move(connectionProvider)) {}

std::shared_ptr<domain::Group> GroupRepository::ReadById(std::string id) {
    auto pooled = connectionProvider->Connection();
    auto connection = dynamic_cast<PostgresConnection*>(&*pooled);

    pqxx::work tx(*(connection->connection));
    pqxx::result result = tx.exec(pqxx::prepped{"select_group_by_id"}, pqxx::params{id});
    tx.commit();

    if (result.empty()) {
        return nullptr;
    }

    auto row = result[0];
    auto group = std::make_shared<domain::Group>(nlohmann::json::parse(row["document"].c_str()));
    group->Id() = row["id"].as<std::string>();

    return group;
}

std::string GroupRepository::Create (const domain::Group & entity) {
    auto pooled = connectionProvider->Connection();
    auto connection = dynamic_cast<PostgresConnection*>(&*pooled);
    nlohmann::json groupBody = entity;

    pqxx::work tx(*(connection->connection));
    pqxx::result result = tx.exec(pqxx::prepped{"insert_group"}, pqxx::params{entity.TournamentId(), groupBody.dump()});

    tx.commit();

    return result[0]["id"].as<std::string>();
}

std::string GroupRepository::Update (const domain::Group & entity) {
    auto pooled = connectionProvider->Connection();
    auto connection = dynamic_cast<PostgresConnection*>(&*pooled);

    try {
        pqxx::work tx(*(connection->connection));
        pqxx::result result = tx.exec(pqxx::prepped{"update_group_name"},
                                     pqxx::params{entity.Name(), entity.Id(), entity.TournamentId()});
        tx.commit();
        if (result.empty()) {
            throw domain::NotFoundException();
        }
        return result[0]["id"].as<std::string>();
    } catch (const pqxx::unique_violation& e) {
        throw domain::DuplicateEntryException();
    }
}

void GroupRepository::Delete(std::string id) {
    auto pooled = connectionProvider->Connection();
    auto connection = dynamic_cast<PostgresConnection*>(&*pooled);
    pqxx::work tx(*(connection->connection));

    pqxx::result result = tx.exec(pqxx::prepped{"delete_group"}, pqxx::params{id});
    tx.commit();

    if (result.affected_rows() == 0) {
        throw domain::NotFoundException();
    }
}

std::vector<std::shared_ptr<domain::Group>> GroupRepository::ReadAll() {
    std::vector<std::shared_ptr<domain::Group>> groups;
    auto pooled = connectionProvider->Connection();
    auto connection = dynamic_cast<PostgresConnection*>(&*pooled);

    pqxx::work tx(*(connection->connection));
    pqxx::result result{tx.exec("SELECT id, document FROM groups")};
    tx.commit();

    for(auto row : result){
        auto group = std::make_shared<domain::Group>(nlohmann::json::parse(row["document"].c_str()));
        group->Id() = row["id"].as<std::string>();
        groups.push_back(group);
    }
    return groups;
}

std::vector<std::shared_ptr<domain::Group>> GroupRepository::FindByTournamentId(const std::string_view& tournamentId) {
    auto pooled = connectionProvider->Connection();
    auto connection = dynamic_cast<PostgresConnection*>(&*pooled);
    pqxx::work tx(*(connection->connection));
    pqxx::result result = tx.exec(pqxx::prepped{"select_groups_by_tournament"}, pqxx::params{tournamentId});
    tx.commit();

    std::vector<std::shared_ptr<domain::Group>> groups;
    for(auto row : result){
        auto group = std::make_shared<domain::Group>(nlohmann::json::parse(row["document"].c_str()));
        group->Id() = row["id"].as<std::string>();
        groups.push_back(group);
    }
    return groups;
}

std::shared_ptr<domain::Group> GroupRepository::FindByTournamentIdAndGroupId(const std::string_view& tournamentId, const std::string_view& groupId) {
    auto pooled = connectionProvider->Connection();
    auto connection = dynamic_cast<PostgresConnection*>(&*pooled);
    pqxx::work tx(*(connection->connection));
    pqxx::result result = tx.exec(pqxx::prepped{"select_group_by_tournamentid_groupid"}, pqxx::params{tournamentId, groupId});
    tx.commit();

    if (result.empty()) {
        return nullptr;
    }

    auto row = result[0];
    auto group = std::make_shared<domain::Group>(nlohmann::json::parse(row["document"].c_str()));
    group->Id() = row["id"].as<std::string>();
    return group;
}

std::shared_ptr<domain::Group> GroupRepository::FindByTournamentIdAndTeamId(const std::string_view& tournamentId, const std::string_view& teamId) {
    auto pooled = connectionProvider->Connection();
    const auto connection = dynamic_cast<PostgresConnection*>(&*pooled);

    pqxx::work tx(*(connection->connection));
    const pqxx::result result = tx.exec(pqxx::prepped{"select_group_in_tournament"}, pqxx::params{tournamentId, teamId});
    tx.commit();
    if (result.empty()) {
        return nullptr;
    }
    nlohmann::json groupDocument = nlohmann::json::parse(result[0]["document"].c_str());
    std::shared_ptr<domain::Group> group = std::make_shared<domain::Group>(groupDocument);
    group->Id() = result[0]["id"].as<std::string>();

    return group;
}

void GroupRepository::UpdateGroupAddTeam(std::string_view groupId, const domain::Team& team) {
    nlohmann::json teamDocument = team;

    auto pooled = connectionProvider->Connection();
    auto connection = dynamic_cast<PostgresConnection*>(&*pooled);
    pqxx::work tx(*(connection->connection));
    tx.exec(pqxx::prepped{"update_group_add_team"}, pqxx::params{groupId, teamDocument.dump()});
    tx.commit();
}