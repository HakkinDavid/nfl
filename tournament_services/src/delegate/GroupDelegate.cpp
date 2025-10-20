//
// Created by root on 10/13/25.
//

#include "delegate/GroupDelegate.hpp"

#include "domain/Utilities.hpp"
#include "persistence/repository/TournamentRepository.hpp"
#include "persistence/repository/IGroupRepository.hpp"
#include "persistence/repository/TeamRepository.hpp"
#include "cms/IQueueMessageProducer.hpp"
#include <utility>
#include <format>

GroupDelegate::GroupDelegate(std::shared_ptr<IRepository<domain::Tournament, std::string>> tournamentRepo,
                             std::shared_ptr<IGroupRepository> groupRepo,
                             std::shared_ptr<IRepository<domain::Team, std::string>> teamRepo,
                             std::shared_ptr<IQueueMessageProducer> producer)
    : tournamentRepository(std::move(tournamentRepo)),
      groupRepository(std::move(groupRepo)),
      teamRepository(std::move(teamRepo)),
      producer(std::move(producer)) {}

std::expected<std::vector<domain::Group>, std::string> GroupDelegate::GetGroups(std::string tournamentId) {
    if (!tournamentRepository->ReadById(tournamentId)) {
        return std::unexpected("Tournament not found.");
    }

    auto groupsAsPtrs = groupRepository->FindByTournamentId(tournamentId);
    std::vector<domain::Group> groupsAsValues;
    groupsAsValues.reserve(groupsAsPtrs.size());

    for (const auto& groupPtr : groupsAsPtrs) {
        if (groupPtr) {
            groupsAsValues.push_back(*groupPtr);
        }
    }

    return groupsAsValues;
}

std::expected<domain::Group, std::string> GroupDelegate::GetGroup(std::string tournamentId, std::string_view groupId) {
    if (!tournamentRepository->ReadById(tournamentId)) {
        return std::unexpected("Tournament not found.");
    }
    auto group = groupRepository->FindByTournamentIdAndGroupId(tournamentId, groupId);
    if (!group) {
        return std::unexpected("Group not found in this tournament.");
    }
    return *group;
}

std::expected<std::string, std::string> GroupDelegate::CreateGroup(const std::string tournamentId, domain::Group& group) {
    std::shared_ptr<domain::Tournament> tournament = tournamentRepository->ReadById(tournamentId);
    if (!tournament) {
        return std::unexpected("Tournament not found.");
    }

    auto existingGroups = groupRepository->FindByTournamentId(tournamentId);
    if (existingGroups.size() >= MAX_GROUPS_PER_TOURNAMENT) {
        return std::unexpected("Maximum number of groups for this tournament has been reached.");
    }

    if (group.Teams().size() > MAX_TEAMS_PER_GROUP) {
        return std::unexpected(std::format("A group cannot have more than {} teams.", MAX_TEAMS_PER_GROUP));
    }

    for (const auto& team : group.Teams()) {
        if (!teamRepository->ReadById(team.Id)) {
            return std::unexpected(std::format("Team with ID {} does not exist.", team.Id));
        }
        if (groupRepository->FindByTournamentIdAndTeamId(tournamentId, team.Id)) {
            return std::unexpected(std::format("Team {} is already in another group in this tournament.", team.Name));
        }
    }

    group.TournamentId() = tournamentId;
    try {
        std::string newGroupId = groupRepository->Create(group);
        checkAndPublishTournamentReadyEvent(tournamentId);
        return newGroupId;
    } catch (const domain::DuplicateEntryException& e) {
        return std::unexpected(e.what());
    }
}

std::expected<void, std::string> GroupDelegate::AddTeamToGroup(std::string_view tournamentId, std::string_view groupId, const domain::Team& team) {
    auto group = groupRepository->FindByTournamentIdAndGroupId(tournamentId, groupId);
    if (!group) {
        return std::unexpected("Group not found in this tournament.");
    }

    if (group->Teams().size() >= MAX_TEAMS_PER_GROUP) {
        return std::unexpected("Group is already full.");
    }

    if (!teamRepository->ReadById(team.Id)) {
        return std::unexpected(std::format("Team with ID {} does not exist.", team.Id));
    }

    if (groupRepository->FindByTournamentIdAndTeamId(tournamentId, team.Id)) {
        return std::unexpected(std::format("Team {} is already in a group in this tournament.", team.Name));
    }

    groupRepository->UpdateGroupAddTeam(groupId, team);
    checkAndPublishTournamentReadyEvent(tournamentId);
    return {};
}

std::expected<void, std::string> GroupDelegate::UpdateGroupName(std::string_view tournamentId, std::string_view groupId, const domain::Group& groupUpdatePayload) {
    auto group = groupRepository->FindByTournamentIdAndGroupId(tournamentId, groupId);
    if (!group) {
        return std::unexpected("Group not found in this tournament.");
    }

    group->Name() = groupUpdatePayload.Name();
    group->TournamentId() = tournamentId;

    try {
        groupRepository->Update(*group);
        return {};
    } catch (const domain::NotFoundException& e) {
        return std::unexpected(e.what());
    } catch (const domain::DuplicateEntryException& e) {
        return std::unexpected(e.what());
    }
}

std::expected<void, std::string> GroupDelegate::DeleteGroup(std::string_view tournamentId, std::string groupId) {
    if (!groupRepository->FindByTournamentIdAndGroupId(tournamentId, groupId)) {
        return std::unexpected("Group not found in this tournament.");
    }

    try {
        groupRepository->Delete(groupId);
        return {};
    } catch (const domain::NotFoundException& e) {
        return std::unexpected(e.what());
    }
}

// Lógica del Evento
void GroupDelegate::checkAndPublishTournamentReadyEvent(std::string_view tournamentId) {
    auto groups = groupRepository->FindByTournamentId(tournamentId);

    if (groups.size() != MAX_GROUPS_PER_TOURNAMENT) {
        return;
    }

    for (const auto& group : groups) {
        if (group->Teams().size() != MAX_TEAMS_PER_GROUP) {
            return;
        }
    }

    producer->SendMessage(tournamentId, "tournament.ready");
}