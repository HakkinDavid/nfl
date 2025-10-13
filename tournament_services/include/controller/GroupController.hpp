#ifndef SERVICE_GROUP_CONTROLLER_HPP
#define SERVICE_GROUP_CONTROLLER_HPP

#define JSON_CONTENT_TYPE "application/json"
#define CONTENT_TYPE_HEADER "content-type"

#include <vector>
#include <string>
#include <memory>
#include <crow.h>
#include <nlohmann/json.hpp>

#include "configuration/RouteDefinition.hpp"
#include "delegate/IGroupDelegate.hpp"
#include "domain/Group.hpp"
#include "domain/Utilities.hpp"


class IGroupDelegate;

class GroupController {
    std::shared_ptr<IGroupDelegate> groupDelegate;

public:
    // El constructor toma la interfaz, lo que facilita la inyección de dependencias.
    explicit GroupController(std::shared_ptr<IGroupDelegate> delegate);

    // --- GET /tournaments/{id}/groups ---
    [[nodiscard]] crow::response GetGroups(const std::string& tournamentId) const;

    // --- GET /tournaments/{id}/groups/{id} ---
    [[nodiscard]] crow::response GetGroup(const std::string& tournamentId, const std::string& groupId) const;

    // --- POST /tournaments/{id}/groups ---
    [[nodiscard]] crow::response CreateGroup(const crow::request& req, const std::string& tournamentId) const;

    // --- POST /tournaments/{id}/groups/{id}/teams ---
    [[nodiscard]] crow::response AddTeamToGroup(const crow::request& req, const std::string& tournamentId, const std::string& groupId) const;

    // --- PATCH /tournaments/{id}/groups/{id} ---
    [[nodiscard]] crow::response UpdateGroupName(const crow::request& req, const std::string& tournamentId, const std::string& groupId) const;

    // --- DELETE /tournaments/{id}/groups/{id} ---
    [[nodiscard]] crow::response DeleteGroup(const std::string& tournamentId, const std::string& groupId) const;
};

#endif // SERVICE_GROUP_CONTROLLER_HPP
