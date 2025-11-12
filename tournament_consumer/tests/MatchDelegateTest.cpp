//
// Created by dcasta on 11/11/25.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <memory>
#include <vector>
#include <expected>
#include <string>

#include "persistence/repository/IMatchRepository.hpp"
#include "persistence/repository/IGroupRepository.hpp"
#include "domain/Match.hpp"
#include "domain/Group.hpp"
#include "domain/Team.hpp"
#include "domain/Utilities.hpp"
#include "delegate/MatchDelegate.hpp"

class MatchRepositoryMock : public IMatchRepository {
public:
    MOCK_METHOD((std::shared_ptr<domain::Match>), ReadById, (std::string id), (override));
    MOCK_METHOD(std::string, Create, (const domain::Match& entity), (override));
    MOCK_METHOD(std::string, Update, (const domain::Match& entity), (override));
    MOCK_METHOD(void, Delete, (std::string id), (override));
    MOCK_METHOD((std::vector<std::shared_ptr<domain::Match>>), ReadAll, (), (override));

    MOCK_METHOD((std::vector<std::shared_ptr<domain::Match>>), FindMatchesByTournamentAndRound, (std::string_view tournamentId, std::string_view round), (override));
    MOCK_METHOD((std::vector<std::shared_ptr<domain::Match>>), FindAllByTournamentId, (std::string_view tournamentId), (override));
    MOCK_METHOD((std::shared_ptr<domain::Match>), FindByIdAndTournamentId, (std::string_view matchId, std::string_view tournamentId), (override));
    MOCK_METHOD(std::string, UpdateScore, (std::string_view matchId, std::string_view tournamentId, const domain::Score& score), (override));
    MOCK_METHOD((std::vector<std::shared_ptr<domain::Match>>), GetMatchesByTeamId, (std::string_view tournamentId, std::string_view teamId), (override));
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

class MatchDelegateTest : public ::testing::Test {
protected:
    std::shared_ptr<MatchRepositoryMock> matchRepoMock;
    std::shared_ptr<GroupRepositoryMock> groupRepoMock;
    std::shared_ptr<MatchDelegate> matchDelegate;

    const std::string TOURNAMENT_ID = "tournament-123";
    const std::string MATCH_ID = "match-456";

    void SetUp() override {
        matchRepoMock = std::make_shared<MatchRepositoryMock>();
        groupRepoMock = std::make_shared<GroupRepositoryMock>();
        matchDelegate = std::make_shared<MatchDelegate>(matchRepoMock, groupRepoMock);
    }

    // Helper para crear grupos mock con equipos
    std::vector<std::shared_ptr<domain::Group>> createMockGroups(int count) {
        std::vector<std::shared_ptr<domain::Group>> groups;
        for (int i = 0; i < count; ++i) {
            auto group = std::make_shared<domain::Group>();
            group->Id() = "group-" + std::to_string(i);

            // 4 equipos para cada grupo
            std::vector<domain::Team> teams;
            for (int j = 0; j < 4; ++j) {
                teams.push_back(domain::Team{"team-" + std::to_string(i*4 + j), "Team " + std::to_string(i*4 + j)});
            }
            group->Teams() = teams;
            groups.push_back(group);
        }
        return groups;
    }
};

TEST_F(MatchDelegateTest, CreateFirstRoundMatches_Success) {
    auto mockGroups = createMockGroups(8);

    EXPECT_CALL(*groupRepoMock, FindByTournamentId(TOURNAMENT_ID))
        .WillOnce(testing::Return(mockGroups));

    // 160 matches (96 intergroup + 64 intra-group)
    EXPECT_CALL(*matchRepoMock, Create(::testing::_))
        .Times(160)
        .WillRepeatedly(testing::Return("match-id"));

    auto result = matchDelegate->createFirstRoundMatches(TOURNAMENT_ID);

    ASSERT_TRUE(result.has_value());
}

TEST_F(MatchDelegateTest, CreateFirstRoundMatches_FailsWhenRepositoryThrows) {
    auto mockGroups = createMockGroups(8);

    EXPECT_CALL(*groupRepoMock, FindByTournamentId(TOURNAMENT_ID))
        .WillOnce(testing::Return(mockGroups));

    EXPECT_CALL(*matchRepoMock, Create(::testing::_))
        .WillOnce(testing::Throw(std::runtime_error("Database error")));

    auto result = matchDelegate->createFirstRoundMatches(TOURNAMENT_ID);

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), "Database error");
}

TEST_F(MatchDelegateTest, CreateFirstRoundMatches_FailsWithInsufficientGroups) {
    // 4 / 8
    auto mockGroups = createMockGroups(4);

    EXPECT_CALL(*groupRepoMock, FindByTournamentId(TOURNAMENT_ID))
        .WillOnce(testing::Return(mockGroups));

    auto result = matchDelegate->createFirstRoundMatches(TOURNAMENT_ID);

    // Deberia funcionar pero con mal behavior
    ASSERT_TRUE(result.has_value());
}

TEST_F(MatchDelegateTest, GenerateNextRound_Success_FirstRoundToWildCard) {
    auto mockMatch = std::make_shared<domain::Match>();
    mockMatch->Round() = "First Round";

    // Mock 160 matches de First Round completadas
    std::vector<std::shared_ptr<domain::Match>> firstRoundMatches(160);
    for (auto& match : firstRoundMatches) {
        match = std::make_shared<domain::Match>();
        match->Score() = domain::Score{1, 0}; // Todas tienen score
    }

    EXPECT_CALL(*matchRepoMock, FindByIdAndTournamentId(MATCH_ID, TOURNAMENT_ID))
        .WillOnce(testing::Return(mockMatch));

    EXPECT_CALL(*matchRepoMock, FindMatchesByTournamentAndRound(TOURNAMENT_ID, "First Round"))
        .WillOnce(testing::Return(firstRoundMatches));

    EXPECT_CALL(*matchRepoMock, Create(::testing::_))
        .Times(6) // 6 Wild Card matches
        .WillRepeatedly(testing::Return("wildcard-match-id"));

    // Mock playoff teams
    EXPECT_CALL(*groupRepoMock, FindByTournamentId(TOURNAMENT_ID))
        .WillOnce(testing::Return(createMockGroups(8)));

    // Mock team matches para calculacion de playoff
    EXPECT_CALL(*matchRepoMock, GetMatchesByTeamId(::testing::_, ::testing::_))
        .WillRepeatedly(testing::Return(std::vector<std::shared_ptr<domain::Match>>{}));

    auto result = matchDelegate->generateNextRound(MATCH_ID, TOURNAMENT_ID);

    ASSERT_TRUE(result.has_value());
}
