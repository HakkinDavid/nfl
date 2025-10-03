//
// Created by tomas on 8/22/25.
//

#ifndef RESTAPI_TESTDELEGATE_HPP
#define RESTAPI_TESTDELEGATE_HPP
#include <memory>

#include "persistence/repository/TeamRepository.hpp" // why not just use the TeamRepository
#include "domain/Team.hpp"
#include "ITeamDelegate.hpp"

class TeamDelegate : public ITeamDelegate {
    std::shared_ptr<IRepository<domain::Team, std::string>> teamRepository;
    public:
    explicit TeamDelegate(std::shared_ptr<IRepository<domain::Team, std::string>> repository);
    std::shared_ptr<domain::Team> GetTeam(std::string id) override;
    std::vector<std::shared_ptr<domain::Team>> GetAllTeams() override;
    std::expected<std::string, std::string> SaveTeam( const domain::Team& team) override;
    std::expected<std::string, std::string> UpdateTeam(const std::string& teamId, const domain::Team& team) override;
    std::expected<void, std::string> DeleteTeam(const std::string& teamId) override;
};


#endif //RESTAPI_TESTDELEGATE_HPP