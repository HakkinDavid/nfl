#ifndef SERVICE_IGROUP_DELEGATE_HPP
#define SERVICE_IGROUP_DELEGATE_HPP

#include <string>
#include <string_view>
#include <vector>
#include <expected>

#include "domain/Group.hpp"

class IGroupDelegate{
public:
    virtual ~IGroupDelegate() = default;
    // GET /tournaments/{id}/groups
    virtual std::expected<std::vector<domain::Group>, std::string> GetGroups(std::string tournamentId) = 0;

    // GET /tournaments/{id}/groups/{id}
    virtual std::expected<domain::Group, std::string> GetGroup(std::string tournamentId, std::string_view groupId) = 0;

    // POST /tournaments/{id}/groups
    virtual std::expected<std::string, std::string> CreateGroup(std::string tournamentId, domain::Group& group) = 0;

    // POST /tournaments/{id}/groups/{id}/teams
    virtual std::expected<void, std::string> AddTeamToGroup(std::string_view tournamentId, std::string_view groupId, const domain::Team& team) = 0;

    // PATCH /tournaments/{id}/groups/{id}
    virtual std::expected<void, std::string> UpdateGroupName(std::string_view tournamentId, std::string_view groupId, const domain::Group& groupUpdatePayload) = 0;

    // DELETE /tournaments/{id}/groups/{id}
    virtual std::expected<void, std::string> DeleteGroup(std::string_view tournamentId, std::string groupId) = 0;
};

#endif /* SERVICE_IGROUP_DELEGATE_HPP */
