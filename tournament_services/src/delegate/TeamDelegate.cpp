//
// Created by tomas on 8/22/25.
//

#include "delegate/TeamDelegate.hpp"

#include <utility>

TeamDelegate::TeamDelegate(std::shared_ptr<IRepository<domain::Team, std::string> > repository) : teamRepository(std::move(repository)) {
}

std::vector<std::shared_ptr<domain::Team>> TeamDelegate::GetAllTeams() {
    return teamRepository->ReadAll();
}

std::shared_ptr<domain::Team> TeamDelegate::GetTeam(std::string id) {
    return teamRepository->ReadById(id.data());
}

std::expected<std::string, std::string> TeamDelegate::SaveTeam(const domain::Team& team){
    try {
        return teamRepository->Create(team);
    } catch (const domain::DuplicateEntryException& e) {
        return std::unexpected(e.what());
    }
}

std::expected<std::string, std::string> TeamDelegate::UpdateTeam(const std::string& teamId, const domain::Team& team) {
    try {
        domain::Team teamToUpdate{teamId, team.Name};
        return teamRepository->Update(teamToUpdate);
    } catch (const domain::NotFoundException& e) {
        return std::unexpected(e.what());
    } catch (const domain::DuplicateEntryException& e) {
        return std::unexpected(e.what());
    }
}


