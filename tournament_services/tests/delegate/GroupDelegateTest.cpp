//
// Created by root on 10/13/25.
//

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <memory>
#include <vector>
#include <expected>
#include <string>

#include "persistence/repository/TournamentRepository.hpp"
#include "persistence/repository/GroupRepository.hpp"
#include "persistence/repository/TeamRepository.hpp"
#include "cms/IQueueMessageProducer.hpp"
#include "domain/Tournament.hpp"
#include "domain/Group.hpp"
#include "domain/Team.hpp"
#include "domain/Utilities.hpp"
#include "delegate/GroupDelegate.hpp"

// Mocks para todas las dependencias del Delegate
class TournamentRepositoryMock : public IRepository<domain::Tournament, std::string> {
public:
    MOCK_METHOD((std::shared_ptr<domain::Tournament>), ReadById, (std::string id), (override));
    MOCK_METHOD(std::string, Create, (const domain::Tournament& entity), (override));
    MOCK_METHOD(std::string, Update, (const domain::Tournament& entity), (override));
    MOCK_METHOD(void, Delete, (std::string id), (override));
    MOCK_METHOD(std::vector<std::shared_ptr<domain::Tournament>>, ReadAll, (), (override));
};

class GroupRepositoryMock : public IGroupRepository {
public:
    MOCK_METHOD((std::shared_ptr<domain::Group>), ReadById, (std::string id), (override));
    MOCK_METHOD(std::string, Create, (const domain::Group& entity), (override));
    MOCK_METHOD(std::string, Update, (const domain::Group& entity), (override));
    MOCK_METHOD(void, Delete, (std::string id), (override));
    MOCK_METHOD((std::vector<std::shared_ptr<domain::Group>>), ReadAll, (), (override));
    MOCK_METHOD((std::vector<std::shared_ptr<domain::Group>>), FindByTournamentId, (const std::string_view& tournamentId), (override));
    MOCK_METHOD((std::shared_ptr<domain::Group>), FindByTournamentIdAndGroupId, (const std::string_view& tournamentId, const std::string_view& groupId), (override));
    MOCK_METHOD((std::shared_ptr<domain::Group>), FindByTournamentIdAndTeamId, (const std::string_view& tournamentId, const std::string_view& teamId), (override));
    MOCK_METHOD(void, UpdateGroupAddTeam, (std::string_view groupId, const domain::Team& team), (override));
};

class TeamRepositoryMock : public IRepository<domain::Team, std::string> {
public:
    MOCK_METHOD(std::shared_ptr<domain::Team>, ReadById, (std::string id), (override));
    MOCK_METHOD(std::string, Create, (const domain::Team& entity), (override));
    MOCK_METHOD(std::string, Update, (const domain::Team& entity), (override));
    MOCK_METHOD(void, Delete, (std::string id), (override));
    MOCK_METHOD(std::vector<std::shared_ptr<domain::Team>>, ReadAll, (), (override));
};

class QueueMessageProducerMock : public IQueueMessageProducer {
public:
    MOCK_METHOD(void, SendMessage, (const std::string_view& message, const std::string_view& routingKey), (override));
};

class GroupDelegateTest : public ::testing::Test {
protected:
    std::shared_ptr<TournamentRepositoryMock> tournamentRepoMock;
    std::shared_ptr<GroupRepositoryMock> groupRepoMock;
    std::shared_ptr<TeamRepositoryMock> teamRepoMock;
    std::shared_ptr<QueueMessageProducerMock> producerMock;
    std::shared_ptr<GroupDelegate> groupDelegate;

    void SetUp() override {
        tournamentRepoMock = std::make_shared<TournamentRepositoryMock>();
        groupRepoMock = std::make_shared<GroupRepositoryMock>();
        teamRepoMock = std::make_shared<TeamRepositoryMock>();
        producerMock = std::make_shared<QueueMessageProducerMock>();

        groupDelegate = std::make_shared<GroupDelegate>(
            tournamentRepoMock,
            groupRepoMock,
            teamRepoMock,
            producerMock
        );
    }
};

// Pruebas para CreateGroup

TEST_F(GroupDelegateTest, CreateGroup_Success) {
    const std::string tournamentId = "tour-123";
    domain::Group groupPayload;
    groupPayload.Name() = "Group A";
    const std::string newGroupId = "group-abc";

    auto mockTournament = std::make_shared<domain::Tournament>();
    mockTournament->Id() = tournamentId;
    mockTournament->Name() = "Mock Tournament";

    EXPECT_CALL(*tournamentRepoMock, ReadById(tournamentId))
        .WillOnce(testing::Return(mockTournament));
    EXPECT_CALL(*groupRepoMock, FindByTournamentId(tournamentId))
        .Times(2)
        .WillRepeatedly(testing::Return(std::vector<std::shared_ptr<domain::Group>>{}));
    EXPECT_CALL(*groupRepoMock, Create(::testing::_))
        .WillOnce(testing::Return(newGroupId));

    auto result = groupDelegate->CreateGroup(tournamentId, groupPayload);

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(newGroupId, result.value());
}

TEST_F(GroupDelegateTest, CreateGroup_FailsWhenTournamentNotFound) {
    EXPECT_CALL(*tournamentRepoMock, ReadById(::testing::_))
        .WillOnce(testing::Return(nullptr));

    domain::Group dummyGroup;
    auto result = groupDelegate->CreateGroup("tour-123", dummyGroup);

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ("Tournament not found.", result.error());
}

TEST_F(GroupDelegateTest, CreateGroup_FailsWhenMaxGroupsReached) {
    const std::string tournamentId = "tour-123";
    std::vector<std::shared_ptr<domain::Group>> fullGroups(8);

    EXPECT_CALL(*tournamentRepoMock, ReadById(tournamentId))
        .WillOnce(testing::Return(std::make_shared<domain::Tournament>()));
    EXPECT_CALL(*groupRepoMock, FindByTournamentId(tournamentId))
        .WillOnce(testing::Return(fullGroups));

    domain::Group dummyGroup;
    auto result = groupDelegate->CreateGroup(tournamentId, dummyGroup);

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ("Maximum number of groups for this tournament has been reached.", result.error());
}

TEST_F(GroupDelegateTest, CreateGroup_FailsOnDuplicateName) {
    const std::string tournamentId = "tour-123";
    domain::Group groupPayload;
    groupPayload.Name() = "Group A";

    EXPECT_CALL(*tournamentRepoMock, ReadById(tournamentId))
        .WillOnce(testing::Return(std::make_shared<domain::Tournament>()));
    EXPECT_CALL(*groupRepoMock, FindByTournamentId(tournamentId))
        .WillOnce(testing::Return(std::vector<std::shared_ptr<domain::Group>>{}));
    // Simulamos que el repositorio falla por clave duplicada
    EXPECT_CALL(*groupRepoMock, Create(::testing::_))
        .WillOnce(testing::Throw(domain::DuplicateEntryException()));

    auto result = groupDelegate->CreateGroup(tournamentId, groupPayload);

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ("Entry already exists.", result.error());
}

TEST_F(GroupDelegateTest, CreateGroup_FailsWithTooManyTeamsInPayload) {
    const std::string tournamentId = "tour-123";
    domain::Group groupPayload;
    groupPayload.Name() = "Group A";
    groupPayload.Teams().resize(5); // Payload con 5 equipos (límite es 4)

    EXPECT_CALL(*tournamentRepoMock, ReadById(tournamentId))
        .WillOnce(testing::Return(std::make_shared<domain::Tournament>()));
    EXPECT_CALL(*groupRepoMock, FindByTournamentId(tournamentId))
        .WillOnce(testing::Return(std::vector<std::shared_ptr<domain::Group>>{}));

    auto result = groupDelegate->CreateGroup(tournamentId, groupPayload);

    ASSERT_FALSE(result.has_value());
    EXPECT_NE(std::string::npos, result.error().find("cannot have more than"));
}

// Pruebas para GetGroup

TEST_F(GroupDelegateTest, GetGroup_Success) {
    const std::string tournamentId = "tour-123";
    const std::string groupId = "group-abc";
    auto expectedGroup = std::make_shared<domain::Group>();
    expectedGroup->Id() = groupId;
    expectedGroup->Name() = "Group A";

    EXPECT_CALL(*tournamentRepoMock, ReadById(tournamentId))
        .WillOnce(testing::Return(std::make_shared<domain::Tournament>()));
    EXPECT_CALL(*groupRepoMock, FindByTournamentIdAndGroupId(tournamentId, groupId))
        .WillOnce(testing::Return(expectedGroup));

    auto result = groupDelegate->GetGroup(tournamentId, groupId);

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(groupId, result.value().Id());
    EXPECT_EQ("Group A", result.value().Name());
}

TEST_F(GroupDelegateTest, GetGroup_FailsWhenNotFound) {
    const std::string tournamentId = "tour-123";
    const std::string groupId = "group-abc";

    EXPECT_CALL(*tournamentRepoMock, ReadById(tournamentId))
        .WillOnce(testing::Return(std::make_shared<domain::Tournament>()));
    EXPECT_CALL(*groupRepoMock, FindByTournamentIdAndGroupId(tournamentId, groupId))
        .WillOnce(testing::Return(nullptr)); // Simulamos que el grupo no se encuentra

    auto result = groupDelegate->GetGroup(tournamentId, groupId);

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ("Group not found in this tournament.", result.error());
}

// Pruebas para AddTeamToGroup

TEST_F(GroupDelegateTest, AddTeamToGroup_Success) {
    const std::string tournamentId = "tour-123";
    const std::string groupId = "group-abc";
    domain::Team teamToAdd{"team-xyz", "Team XYZ"};

    auto group = std::make_shared<domain::Group>(); // Un grupo que no está lleno

    EXPECT_CALL(*groupRepoMock, FindByTournamentIdAndGroupId(tournamentId, groupId))
        .WillOnce(testing::Return(group));
    EXPECT_CALL(*teamRepoMock, ReadById(teamToAdd.Id))
        .WillOnce(testing::Return(std::make_shared<domain::Team>()));
    EXPECT_CALL(*groupRepoMock, FindByTournamentIdAndTeamId(tournamentId, teamToAdd.Id))
        .WillOnce(testing::Return(nullptr)); // El equipo no está en otro grupo
    EXPECT_CALL(*groupRepoMock, UpdateGroupAddTeam(groupId, ::testing::_))
        .Times(1);
    EXPECT_CALL(*groupRepoMock, FindByTournamentId(tournamentId))
        .WillOnce(testing::Return(std::vector<std::shared_ptr<domain::Group>>{}));

    auto result = groupDelegate->AddTeamToGroup(tournamentId, groupId, teamToAdd);

    EXPECT_TRUE(result.has_value());
}

TEST_F(GroupDelegateTest, AddTeamToGroup_FailsWhenGroupIsFull) {
    const std::string tournamentId = "tour-123";
    const std::string groupId = "group-abc";
    domain::Team dummyTeam{"team-xyz", "Team XYZ"};

    auto fullGroup = std::make_shared<domain::Group>();
    fullGroup->Teams().resize(4); // Simulamos un grupo con 4 equipos

    EXPECT_CALL(*groupRepoMock, FindByTournamentIdAndGroupId(tournamentId, groupId))
        .WillOnce(testing::Return(fullGroup));

    auto result = groupDelegate->AddTeamToGroup(tournamentId, groupId, dummyTeam);

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ("Group is already full.", result.error());
}

TEST_F(GroupDelegateTest, AddTeamToGroup_FailsWhenTeamNotFound) {
    const std::string tournamentId = "tour-123", groupId = "group-abc";
    domain::Team teamToAdd{"non-existent-team-id", "Ghost Team"};

    EXPECT_CALL(*groupRepoMock, FindByTournamentIdAndGroupId(tournamentId, groupId))
        .WillOnce(testing::Return(std::make_shared<domain::Group>()));
    // Simulamos que el equipo a añadir no existe en la base de datos
    EXPECT_CALL(*teamRepoMock, ReadById(teamToAdd.Id))
        .WillOnce(testing::Return(nullptr));

    auto result = groupDelegate->AddTeamToGroup(tournamentId, groupId, teamToAdd);

    ASSERT_FALSE(result.has_value());
    EXPECT_NE(std::string::npos, result.error().find("does not exist"));
}

// Pruebas para Lógica de Eventos

TEST_F(GroupDelegateTest, EventIsPublished_WhenTournamentBecomesFull) {
    const std::string tournamentId = "tour-123";
    const std::string lastGroupId = "group-h";
    domain::Team finalTeam{"team-final", "The Final Team"};

    auto almostFullGroup = std::make_shared<domain::Group>();
    almostFullGroup->Teams().resize(3);

    std::vector<std::shared_ptr<domain::Group>> fullyPopulatedGroups(8);
    for(auto& group : fullyPopulatedGroups) {
        group = std::make_shared<domain::Group>();
        group->Teams().resize(4); // Ahora TODOS los grupos tienen 4 equipos.
    }

    EXPECT_CALL(*groupRepoMock, FindByTournamentIdAndGroupId(tournamentId, lastGroupId))
        .WillOnce(testing::Return(almostFullGroup));
    EXPECT_CALL(*teamRepoMock, ReadById(finalTeam.Id))
        .WillOnce(testing::Return(std::make_shared<domain::Team>()));
    EXPECT_CALL(*groupRepoMock, FindByTournamentIdAndTeamId(tournamentId, finalTeam.Id))
        .WillOnce(testing::Return(nullptr));
    EXPECT_CALL(*groupRepoMock, UpdateGroupAddTeam(lastGroupId, ::testing::_))
        .Times(1);

    // Verificación del Evento
    EXPECT_CALL(*groupRepoMock, FindByTournamentId(tournamentId))
        .WillOnce(testing::Return(fullyPopulatedGroups));
    EXPECT_CALL(*producerMock, SendMessage(tournamentId, "tournament.ready"))
        .Times(1);

    auto result = groupDelegate->AddTeamToGroup(tournamentId, lastGroupId, finalTeam);

    EXPECT_TRUE(result.has_value());
}

// Pruebas para Get, Update, Delete (éxito)

TEST_F(GroupDelegateTest, GetGroups_Success) {
    const std::string tournamentId = "tour-123";
    EXPECT_CALL(*tournamentRepoMock, ReadById(tournamentId))
        .WillOnce(testing::Return(std::make_shared<domain::Tournament>()));
    EXPECT_CALL(*groupRepoMock, FindByTournamentId(tournamentId))
        .WillOnce(testing::Return(std::vector<std::shared_ptr<domain::Group>>{}));

    auto result = groupDelegate->GetGroups(tournamentId);
    EXPECT_TRUE(result.has_value());
}

TEST_F(GroupDelegateTest, UpdateGroupName_Success) {
    const std::string tournamentId = "tour-123", groupId = "group-abc";
    domain::Group payload;
    payload.Name() = "New Name";

    EXPECT_CALL(*groupRepoMock, FindByTournamentIdAndGroupId(tournamentId, groupId))
        .WillOnce(testing::Return(std::make_shared<domain::Group>()));
    EXPECT_CALL(*groupRepoMock, Update(::testing::_))
        .WillOnce(testing::Return(groupId));

    auto result = groupDelegate->UpdateGroupName(tournamentId, groupId, payload);
    EXPECT_TRUE(result.has_value());
}

TEST_F(GroupDelegateTest, DeleteGroup_Success) {
    const std::string tournamentId = "tour-123", groupId = "group-abc";
    EXPECT_CALL(*groupRepoMock, FindByTournamentIdAndGroupId(tournamentId, groupId))
        .WillOnce(testing::Return(std::make_shared<domain::Group>()));
    EXPECT_CALL(*groupRepoMock, Delete(groupId))
        .WillOnce(testing::Return());

    auto result = groupDelegate->DeleteGroup(tournamentId, groupId);
    EXPECT_TRUE(result.has_value());
}

// Falla UpdateGroup
TEST_F(GroupDelegateTest, UpdateGroupName_FailsWhenNotFound) {
    const std::string tournamentId = "tour-123", groupId = "group-abc";
    domain::Group payload;
    payload.Name() = "New Name";

    // Simulamos que el grupo a actualizar no se encuentra
    EXPECT_CALL(*groupRepoMock, FindByTournamentIdAndGroupId(tournamentId, groupId))
        .WillOnce(testing::Return(nullptr));

    auto result = groupDelegate->UpdateGroupName(tournamentId, groupId, payload);

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ("Group not found in this tournament.", result.error());
}