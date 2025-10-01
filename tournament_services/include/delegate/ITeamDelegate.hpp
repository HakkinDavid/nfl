#ifndef ITEAM_DELEGATE_HPP
#define ITEAM_DELEGATE_HPP

#include <string_view>
#include <memory>
#include <expected>

#include "domain/Team.hpp"

class ITeamDelegate {
    public:
    virtual ~ITeamDelegate() = default;
    virtual std::shared_ptr<domain::Team> GetTeam(std::string id) = 0;
    virtual std::vector<std::shared_ptr<domain::Team>> GetAllTeams() = 0;
    virtual std::expected<std::string, std::string> SaveTeam(const domain::Team& team) = 0;
    virtual std::expected<std::string, std::string> UpdateTeam(const std::string& teamId, const domain::Team& team) = 0;
};

#endif /* ITEAM_DELEGATE_HPP */
