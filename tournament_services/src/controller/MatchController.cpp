//
// Created by root on 11/4/25.
//

#include "controller/MatchController.hpp"

#include <nlohmann/json.hpp>
#include <utility>

#include "delegate/IMatchDelegate.hpp"
#include "domain/Match.hpp"
#include "domain/Utilities.hpp"
#include "common/Constants.hpp"
#include "configuration/RouteDefinition.hpp"

#define JSON_CONTENT_TYPE "application/json"
#define CONTENT_TYPE_HEADER "content-type"

MatchController::MatchController(std::shared_ptr<IMatchDelegate> delegate) : delegate(std::move(delegate)) {}

crow::response MatchController::GetMatches(const crow::request& req, const std::string& tournamentId) const {
    if (!std::regex_match(tournamentId, UUID_REGEX)) {
        return crow::response(crow::BAD_REQUEST, "Invalid Tournament ID format.");
    }

    const char* filter_param = req.url_params.get("showMatches");
    std::string filter = (filter_param ? filter_param : "");

    auto result = delegate->GetMatches(tournamentId, filter);

    if (result.has_value()) {
        nlohmann::json body = result.value();
        crow::response res(crow::OK, body.dump());
        res.add_header(CONTENT_TYPE_HEADER, JSON_CONTENT_TYPE);
        return std::move(res);
    }

    if (result.error().find("not found") != std::string::npos) {
        return crow::response(crow::NOT_FOUND, result.error());
    }

    return crow::response(crow::INTERNAL_SERVER_ERROR, result.error());
}

crow::response MatchController::GetMatch(const std::string& tournamentId, const std::string& matchId) const {
    if (!std::regex_match(tournamentId, UUID_REGEX) || !std::regex_match(matchId, UUID_REGEX)) {
        return crow::response(crow::BAD_REQUEST, "Invalid ID format.");
    }

    auto result = delegate->GetMatch(tournamentId, matchId);

    if (result.has_value()) {
        nlohmann::json body = result.value();
        crow::response res(crow::OK, body.dump());
        res.add_header(CONTENT_TYPE_HEADER, JSON_CONTENT_TYPE);
        return std::move(res);
    }

    const auto& error = result.error();
    if (error.find("not found") != std::string::npos) {
        return crow::response(crow::NOT_FOUND, error);
    }
    // Cualquier otro error se considera un 500
    return crow::response(crow::INTERNAL_SERVER_ERROR, error);
}

crow::response MatchController::UpdateMatchScore(const crow::request& req, const std::string& tournamentId, const std::string& matchId) const {
    if (!std::regex_match(tournamentId, UUID_REGEX) || !std::regex_match(matchId, UUID_REGEX)) {
        return crow::response(crow::BAD_REQUEST, "Invalid ID format.");
    }
    if (!nlohmann::json::accept(req.body)) {
        return crow::response(crow::BAD_REQUEST, "Invalid JSON body.");
    }

    domain::Score score;
    try {
        nlohmann::json::parse(req.body).at("score").get_to(score);
    } catch (const std::exception& e) {
        return crow::response(crow::BAD_REQUEST, std::format("Invalid request body: {}", e.what()));
    }

    auto result = delegate->UpdateMatchScore(tournamentId, matchId, score);

    if (result.has_value()) {
        return crow::response(crow::NO_CONTENT);
    }

    const auto& error = result.error();
    if (error.find("not found") != std::string::npos) {
        return crow::response(crow::NOT_FOUND, error);
    }
    if (error.find("Score cannot be negative") != std::string::npos) {
        return crow::response(422, error);
    }

    return crow::response(crow::INTERNAL_SERVER_ERROR, error);
}


// --- REGISTRO DE RUTAS ---
REGISTER_ROUTE(MatchController, GetMatches,       "/tournaments/<string>/matches",           "GET"_method);
REGISTER_ROUTE(MatchController, GetMatch,         "/tournaments/<string>/matches/<string>",  "GET"_method);
REGISTER_ROUTE(MatchController, UpdateMatchScore, "/tournaments/<string>/matches/<string>",  "PATCH"_method);