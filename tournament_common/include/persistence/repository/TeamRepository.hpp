//
// Created by tomas on 8/24/25.
//

#ifndef RESTAPI_TEAMREPOSITORY_HPP
#define RESTAPI_TEAMREPOSITORY_HPP
#include <string>
#include <memory>
#include <nlohmann/json.hpp>


#include "persistence/configuration/IDbConnectionProvider.hpp"
#include "persistence/configuration/PostgresConnection.hpp"
#include "IRepository.hpp"
#include "domain/Team.hpp"
#include "domain/Utilities.hpp"


class TeamRepository : public IRepository<domain::Team, std::string> {
    std::shared_ptr<IDbConnectionProvider> connectionProvider;
public:

    explicit TeamRepository(std::shared_ptr<IDbConnectionProvider> connectionProvider) : connectionProvider(std::move(connectionProvider)){}

    std::vector<std::shared_ptr<domain::Team>> ReadAll() override {
        std::vector<std::shared_ptr<domain::Team>> teams;

        auto pooled = connectionProvider->Connection();
        auto connection = dynamic_cast<PostgresConnection*>(&*pooled);
        
        pqxx::work tx(*(connection->connection));
        pqxx::result result{tx.exec("select id, document->>'name' as name from teams")};
        tx.commit();

        for(auto row : result){
            teams.push_back(std::make_shared<domain::Team>(domain::Team{row["id"].c_str(), row["name"].c_str()}));
        }

        return teams;
    }

    std::shared_ptr<domain::Team> ReadById(std::string id) override {
        auto pooled = connectionProvider->Connection();
        auto connection = dynamic_cast<PostgresConnection*>(&*pooled);

        pqxx::work tx(*(connection->connection));
        pqxx::result result = tx.exec(pqxx::prepped{"select_team_by_id"}, id.data());
        tx.commit();

        if (result.empty()) {
            throw domain::NotFoundException();
        }

        auto team = std::make_shared<domain::Team>( nlohmann::json::parse(result[0]["document"].c_str()));
        team->Id = result[0]["id"].c_str();

        return team;
    }

    std::string Create(const domain::Team &entity) override {
        auto pooled = connectionProvider->Connection();
        auto connection = dynamic_cast<PostgresConnection*>(&*pooled);
        nlohmann::json teamBody = entity;

        try {
            pqxx::work tx(*(connection->connection));
            pqxx::result result = tx.exec(pqxx::prepped{"insert_team"}, teamBody.dump());
            tx.commit();

            return result[0]["id"].as<std::string>();

        } catch (const pqxx::unique_violation &e) {
            throw domain::DuplicateEntryException();
        }
    }

    std::string Update(const domain::Team &entity) override {
        auto pooled = connectionProvider->Connection();
        auto connection = dynamic_cast<PostgresConnection*>(&*pooled);

        try {
            pqxx::work tx(*(connection->connection));
            pqxx::result result = tx.exec(pqxx::prepped{"update_team_name"}, pqxx::params{entity.Name, entity.Id});
            tx.commit();

            // Si el resultado está vacío, significa que no se encontró el ID.
            if (result.empty()) {
                throw domain::NotFoundException();
            }

            return result[0]["id"].as<std::string>();

        } catch (const pqxx::unique_violation &e) {
            throw domain::DuplicateEntryException();
        } catch (const pqxx::sql_error &e) {
            throw;
        }
    }


    void Delete(std::string id) override{
        auto pooled = connectionProvider->Connection();
        auto connection = dynamic_cast<PostgresConnection*>(&*pooled);

        pqxx::work tx(*(connection->connection));
        pqxx::result result = tx.exec(pqxx::prepped{"delete_team"}, id);
        tx.commit();

        if (result.affected_rows() == 0) {
            throw domain::NotFoundException();
        }
    }

    virtual ~TeamRepository() = default;
};


#endif //RESTAPI_TEAMREPOSITORY_HPP