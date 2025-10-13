#ifndef SERVICE_GROUP_DELEGATE_HPP
#define SERVICE_GROUP_DELEGATE_HPP

#include "delegate/IGroupDelegate.hpp"
#include <memory>

class TournamentRepository;
class IGroupRepository;
class TeamRepository;
class IQueueMessageProducer;

class GroupDelegate : public IGroupDelegate {
    std::shared_ptr<TournamentRepository> tournamentRepository;
    std::shared_ptr<IGroupRepository> groupRepository;
    std::shared_ptr<TeamRepository> teamRepository;
    std::shared_ptr<IQueueMessageProducer> producer;

    static constexpr int MAX_GROUPS_PER_TOURNAMENT = 8;
    static constexpr int MAX_TEAMS_PER_GROUP = 4;

    void checkAndPublishTournamentReadyEvent(std::string_view tournamentId);

public:
    GroupDelegate(std::shared_ptr<TournamentRepository> tournamentRepo,
                  std::shared_ptr<IGroupRepository> groupRepo,
                  std::shared_ptr<TeamRepository> teamRepo,
                  std::shared_ptr<IQueueMessageProducer> producer);

    std::expected<std::vector<domain::Group>, std::string> GetGroups(std::string tournamentId) override;
    std::expected<domain::Group, std::string> GetGroup(std::string tournamentId, std::string_view groupId) override;
    std::expected<std::string, std::string> CreateGroup(std::string tournamentId, domain::Group& group) override;
    std::expected<void, std::string> AddTeamToGroup(std::string_view tournamentId, std::string_view groupId, const domain::Team& team) override;
    std::expected<void, std::string> UpdateGroupName(std::string_view tournamentId, std::string_view groupId, const domain::Group& groupUpdatePayload) override;
    std::expected<void, std::string> DeleteGroup(std::string_view tournamentId, std::string groupId) override;
};

#endif // SERVICE_GROUP_DELEGATE_HPP
