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

    auto requestBody = nlohmann::json::parse(request.body);
    domain::Team team = requestBody;

    // Devuelve un std::expected
    auto createdIdResult = teamDelegate->SaveTeam(team);

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

crow::response TeamController::UpdateTeam(const crow::request& request, const std::string& teamId) const {
    crow::response response;

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

    auto updateResult = teamDelegate->UpdateTeam(teamId, team);

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

crow::response TeamController::DeleteTeam(const std::string& teamId) const {
    if(!std::regex_match(teamId, UUID_REGEX)) {
        return crow::response{crow::BAD_REQUEST, "Invalid ID format"};
    }

    auto deleteResult = teamDelegate->DeleteTeam(teamId);

    if (deleteResult.has_value()) {
        // El equipo fue borrado. El estándar para DELETE es 204 No Content.
        return crow::response{crow::NO_CONTENT};
    } else {
        // Si no se encontró el id.
        return crow::response{crow::NOT_FOUND, deleteResult.error()};
    }
}

REGISTER_ROUTE(TeamController, getTeam, "/teams/<string>", "GET"_method)
REGISTER_ROUTE(TeamController, getAllTeams, "/teams", "GET"_method)
REGISTER_ROUTE(TeamController, SaveTeam, "/teams", "POST"_method)
REGISTER_ROUTE(TeamController, UpdateTeam, "/teams/<string>", "PATCH"_method)
REGISTER_ROUTE(TeamController, DeleteTeam, "/teams/<string>", "DELETE"_method)