//
// Created by root on 9/27/25.
//

#define JSON_CONTENT_TYPE "application/json"
#define CONTENT_TYPE_HEADER "content-type"

#include "configuration/RouteDefinition.hpp"
#include "controller/TeamController.hpp"
#include "domain/Utilities.hpp"


TeamController::TeamController(const std::shared_ptr<ITeamDelegate>& teamDelegate) : teamDelegate(teamDelegate) {}

crow::response TeamController::getTeam(const std::string& teamId) const {
    if(!std::regex_match(teamId, UUID_REGEX)) {
        return crow::response{crow::BAD_REQUEST, "Invalid ID format"};
    }

    try {
        if(auto team = teamDelegate->GetTeam(teamId); team != nullptr) {
            nlohmann::json body = team;
            auto response = crow::response{crow::OK, body.dump()};
            response.add_header(CONTENT_TYPE_HEADER, JSON_CONTENT_TYPE);
            return response;
        }
        return crow::response{crow::NOT_FOUND, "Team not found"};
    } catch (const domain::NotFoundException& e) {
        return crow::response{crow::NOT_FOUND, "Team not found"};
    } catch (const std::exception& e) {
        return crow::response{crow::INTERNAL_SERVER_ERROR, "An internal error occurred."};
    }
}

crow::response TeamController::getAllTeams() const {

    nlohmann::json body = teamDelegate->GetAllTeams();
    crow::response response{200, body.dump()};
    response.add_header(CONTENT_TYPE_HEADER, JSON_CONTENT_TYPE);
    return response;
}

crow::response TeamController::SaveTeam(const crow::request& request) const {
    crow::response response;

    if(!nlohmann::json::accept(request.body)) {
        response.code = crow::BAD_REQUEST;
        return response;
    }
    try {
        auto requestBody = nlohmann::json::parse(request.body);
        domain::Team team = requestBody;

        auto createdId = teamDelegate->SaveTeam(team);
        response.code = crow::CREATED; // 201
        response.add_header("location", createdId.data());

    } catch (const domain::DuplicateEntryException& e) {
        response.code = crow::CONFLICT; // 409
        response.body = "Team with that name already exists.";

    } catch (const std::exception& e) {
        response.code = crow::INTERNAL_SERVER_ERROR; // 500
        response.body = std::string("An internal error occurred: ") + e.what();
    }

    return response;
}

crow::response TeamController::UpdateTeam(const crow::request& request, const std::string& teamId) const {
    // Validar el formato del ID
    if(!std::regex_match(teamId, UUID_REGEX)) {
        return crow::response{crow::BAD_REQUEST, "Invalid ID format"};
    }

    // Validar y parsear el cuerpo JSON
    if(!nlohmann::json::accept(request.body)) {
        return crow::response{crow::BAD_REQUEST, "Invalid JSON body"};
    }

    auto requestBody = nlohmann::json::parse(request.body);
    domain::Team team = requestBody;

    try {
        teamDelegate->UpdateTeam(teamId, team);
        return crow::response{crow::NO_CONTENT}; // HTTP 204
    } catch (const domain::NotFoundException& e) {
        // Si atrapamos NotFoundException, devolvemos HTTP 404.
        return crow::response{crow::NOT_FOUND, "Team not found"};
    } catch (const domain::DuplicateEntryException& e) {
        // Si el nombre ya existe, devolvemos un Conflict (409).
        return crow::response{crow::CONFLICT, "Team with that name already exists."};

    } catch (const std::exception& e) {
        // Cualquier otro error como 500.
        return crow::response{crow::INTERNAL_SERVER_ERROR, "An internal error occurred."};
    }
}

REGISTER_ROUTE(TeamController, getTeam, "/teams/<string>", "GET"_method)
REGISTER_ROUTE(TeamController, getAllTeams, "/teams", "GET"_method)
REGISTER_ROUTE(TeamController, SaveTeam, "/teams", "POST"_method)
REGISTER_ROUTE(TeamController, UpdateTeam, "/teams/<string>", "PATCH"_method)