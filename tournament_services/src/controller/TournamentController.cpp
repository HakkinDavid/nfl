//
// Created by tsuny on 8/31/25.
//

#define JSON_CONTENT_TYPE "application/json"
#define CONTENT_TYPE_HEADER "content-type"

#include "configuration/RouteDefinition.hpp"
#include "controller/TournamentController.hpp"

#include <string>
#include <exception>
#include <utility>
#include <expected>
#include "domain/Tournament.hpp"
#include "domain/Utilities.hpp"

TournamentController::TournamentController(std::shared_ptr<ITournamentDelegate> delegate) : tournamentDelegate(std::move(delegate)) {}

crow::response TournamentController::CreateTournament(const crow::request &request) const
{
    nlohmann::json body = nlohmann::json::parse(request.body);
    const std::shared_ptr<domain::Tournament> tournament = std::make_shared<domain::Tournament>(body);

    crow::response response;
    auto createdIdResult = tournamentDelegate->CreateTournament(tournament);
    if (createdIdResult.has_value()) {
        response.code = crow::CREATED; // 201
        response.add_header("location", createdIdResult.value());
    } else {
        const std::string& errorMessage = createdIdResult.error();
        if (errorMessage.find("already exists") != std::string::npos) {
            response.code = crow::CONFLICT; // 409
            response.body = errorMessage;
        } else {
            response.code = crow::INTERNAL_SERVER_ERROR; // 500
            response.body = errorMessage;
        }
    }
    return response;
}

crow::response TournamentController::UpdateTournament(const crow::request &request) const
{
    nlohmann::json body = nlohmann::json::parse(request.body);
    const std::shared_ptr<domain::Tournament> tournament = std::make_shared<domain::Tournament>(body);

    crow::response response;
    auto updateResult = tournamentDelegate->UpdateTournament(tournament);
    if (updateResult.has_value()) {
        response.code = crow::NO_CONTENT; // 204
    } else {
        const std::string& errorMessage = updateResult.error();
        if (errorMessage.find("not found") != std::string::npos) {
            response.code = crow::NOT_FOUND; // 404
            response.body = errorMessage;
        } else if (errorMessage.find("already exists") != std::string::npos) {
            response.code = crow::CONFLICT; // 409
            response.body = errorMessage;
        } else {
            response.code = crow::INTERNAL_SERVER_ERROR; // 500
            response.body = errorMessage;
        }
    }
    return response;
}

crow::response TournamentController::GetTournament(const std::string &tournamentId) const
{
    if (!std::regex_match(tournamentId, ID_VALUE))
    {
        return crow::response{crow::BAD_REQUEST, "Invalid ID format"};
    }

    if (auto tournament = tournamentDelegate->GetTournament(tournamentId); tournament != nullptr)
    {
        nlohmann::json body = tournament;
        auto response = crow::response{crow::OK, body.dump()};
        response.add_header("Content-Type", "application/json");
        return response;
    }
    return crow::response{crow::NOT_FOUND, "tournament not found"};
}

crow::response TournamentController::DeleteTournament(const std::string &tournamentId) const
{
    if (!std::regex_match(tournamentId, UUID_REGEX))
    {
        return crow::response{crow::BAD_REQUEST, "Invalid ID format"};
    }

    auto deleteResult = tournamentDelegate->DeleteTournament(tournamentId);

    if (deleteResult.has_value())
    {
        // El tournamento fue borrado. El estándar para DELETE es 204 No Content.
        return crow::response{crow::NO_CONTENT};
    }
    else
    {
        // Si no se encontró el id.
        return crow::response{crow::NOT_FOUND, deleteResult.error()};
    }
}

crow::response TournamentController::ReadAll() const
{
    nlohmann::json body = tournamentDelegate->ReadAll();
    crow::response response;
    response.code = crow::OK;
    response.body = body.dump();
    response.add_header(CONTENT_TYPE_HEADER, JSON_CONTENT_TYPE);

    return response;
}

REGISTER_ROUTE(TournamentController, CreateTournament, "/tournaments", "POST"_method)
REGISTER_ROUTE(TournamentController, UpdateTournament, "/tournaments", "PATCH"_method)
REGISTER_ROUTE(TournamentController, GetTournament, "/tournaments/<string>", "GET"_method)
REGISTER_ROUTE(TournamentController, DeleteTournament, "/tournaments/<string>", "DELETE"_method)
REGISTER_ROUTE(TournamentController, ReadAll, "/tournaments", "GET"_method)