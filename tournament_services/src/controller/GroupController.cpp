//
// Created by root on 10/13/25.
//

#include "controller/GroupController.hpp"

#include <nlohmann/json.hpp>
#include "delegate/IGroupDelegate.hpp"
#include "domain/Group.hpp"
#include "domain/Team.hpp"
#include "common/Constants.hpp"
#include "configuration/RouteDefinition.hpp"

#define JSON_CONTENT_TYPE "application/json"
#define CONTENT_TYPE_HEADER "content-type"

GroupController::GroupController(std::shared_ptr<IGroupDelegate> delegate) : groupDelegate(std::move(delegate)) {}

crow::response GroupController::GetGroups(const std::string& tournamentId) const {
    if (!std::regex_match(tournamentId, UUID_REGEX)) {
        return crow::response(crow::BAD_REQUEST, "Invalid Tournament ID format.");
    }

    auto result = groupDelegate->GetGroups(tournamentId);

    if (result.has_value()) {
        nlohmann::json body = result.value();
        crow::response res(crow::OK, body.dump());
        res.add_header(CONTENT_TYPE_HEADER, JSON_CONTENT_TYPE);
        return res;
    }

    return crow::response(crow::NOT_FOUND, result.error());
}

crow::response GroupController::GetGroup(const std::string& tournamentId, const std::string& groupId) const {
    if (!std::regex_match(tournamentId, UUID_REGEX) || !std::regex_match(groupId, UUID_REGEX)) {
        return crow::response(crow::BAD_REQUEST, "Invalid ID format.");
    }

    auto result = groupDelegate->GetGroup(tournamentId, groupId);

    if (result.has_value()) {
        nlohmann::json body = result.value();
        crow::response res(crow::OK, body.dump());
        res.add_header(CONTENT_TYPE_HEADER, JSON_CONTENT_TYPE);
        return res;
    }

    return crow::response(crow::NOT_FOUND, result.error());
}

crow::response GroupController::CreateGroup(const crow::request& req, const std::string& tournamentId) const {
    if (!std::regex_match(tournamentId, UUID_REGEX)) {
        return crow::response(crow::BAD_REQUEST, "Invalid Tournament ID format.");
    }
    if (!nlohmann::json::accept(req.body)) {
        return crow::response(crow::BAD_REQUEST, "Invalid JSON body.");
    }

    domain::Group group = nlohmann::json::parse(req.body);
    auto result = groupDelegate->CreateGroup(tournamentId, group);

    if (result.has_value()) {
        crow::response res(crow::CREATED);
        res.add_header("Location", result.value());
        return res;
    }

    return crow::response(422, result.error());
}

crow::response GroupController::AddTeamToGroup(const crow::request& req, const std::string& tournamentId, const std::string& groupId) const {
    if (!std::regex_match(tournamentId, UUID_REGEX) || !std::regex_match(groupId, UUID_REGEX)) {
        return crow::response(crow::BAD_REQUEST, "Invalid ID format.");
    }
    if (!nlohmann::json::accept(req.body)) {
        return crow::response(crow::BAD_REQUEST, "Invalid JSON body.");
    }

    domain::Team team = nlohmann::json::parse(req.body);
    auto result = groupDelegate->AddTeamToGroup(tournamentId, groupId, team);

    if (result.has_value()) {
        return crow::response(crow::NO_CONTENT);
    }

    return crow::response(422, result.error());
}

crow::response GroupController::UpdateGroupName(const crow::request& req, const std::string& tournamentId, const std::string& groupId) const {
    if (!std::regex_match(tournamentId, UUID_REGEX) || !std::regex_match(groupId, UUID_REGEX)) {
        return crow::response(crow::BAD_REQUEST, "Invalid ID format.");
    }
    if (!nlohmann::json::accept(req.body)) {
        return crow::response(crow::BAD_REQUEST, "Invalid JSON body.");
    }

    domain::Group groupPayload = nlohmann::json::parse(req.body);
    auto result = groupDelegate->UpdateGroupName(tournamentId, groupId, groupPayload);

    if (result.has_value()) {
        return crow::response(crow::NO_CONTENT);
    }

    const auto& error = result.error();
    if (error.find("not found") != std::string::npos) {
        return crow::response(crow::NOT_FOUND, error);
    }
    if (error.find("already exists") != std::string::npos) {
        return crow::response(crow::CONFLICT, error);
    }

    return crow::response(422, error);
}

crow::response GroupController::DeleteGroup(const std::string& tournamentId, const std::string& groupId) const {
    if (!std::regex_match(tournamentId, UUID_REGEX) || !std::regex_match(groupId, UUID_REGEX)) {
        return crow::response(crow::BAD_REQUEST, "Invalid ID format.");
    }

    auto result = groupDelegate->DeleteGroup(tournamentId, groupId);

    if (result.has_value()) {
        return crow::response(crow::NO_CONTENT);
    }

    return crow::response(crow::NOT_FOUND, result.error());
}

// REGISTRO DE RUTAS
REGISTER_ROUTE(GroupController, GetGroups,       "/tournaments/<string>/groups",           "GET"_method);
REGISTER_ROUTE(GroupController, GetGroup,        "/tournaments/<string>/groups/<string>",  "GET"_method);
REGISTER_ROUTE(GroupController, CreateGroup,     "/tournaments/<string>/groups",           "POST"_method);
REGISTER_ROUTE(GroupController, AddTeamToGroup,  "/tournaments/<string>/groups/<string>/teams", "POST"_method);
REGISTER_ROUTE(GroupController, UpdateGroupName, "/tournaments/<string>/groups/<string>",  "PATCH"_method);
REGISTER_ROUTE(GroupController, DeleteGroup,     "/tournaments/<string>/groups/<string>",  "DELETE"_method);