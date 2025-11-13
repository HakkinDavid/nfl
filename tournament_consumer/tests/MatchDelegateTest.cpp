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
#include "../include/delegate/MatchDelegate.hpp"

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

    std::shared_ptr<domain::Match> createMockMatch(const std::string& id,
                                              const std::string& homeId, const std::string& homeName,
                                              const std::string& visitorId, const std::string& visitorName,
                                              const std::string& round, int homeScore = 0, int visitorScore = 0) {
        auto match = std::make_shared<domain::Match>();
        match->Id() = id;
        match->Home() = domain::Team{homeId, homeName};
        match->Visitor() = domain::Team{visitorId, visitorName};
        match->Round() = round;
        match->TournamentId() = TOURNAMENT_ID;

        if (homeScore > 0 || visitorScore > 0) {
            match->Score() = domain::Score{homeScore, visitorScore};
        }

        return match;
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

TEST_F(MatchDelegateTest, GenerateNextRound_RoundNotComplete_NoAction) {
    auto mockMatch = std::make_shared<domain::Match>();
    mockMatch->Round() = "First Round";

    // 159 / 160 matches
    std::vector<std::shared_ptr<domain::Match>> firstRoundMatches(159);
    for (auto& match : firstRoundMatches) {
        match = std::make_shared<domain::Match>();
        match->Score() = domain::Score{1, 0};
    }

    EXPECT_CALL(*matchRepoMock, FindByIdAndTournamentId(MATCH_ID, TOURNAMENT_ID))
        .WillOnce(testing::Return(mockMatch));

    EXPECT_CALL(*matchRepoMock, FindMatchesByTournamentAndRound(TOURNAMENT_ID, "First Round"))
        .WillOnce(testing::Return(firstRoundMatches));

    // No deberia haber creacion de matches
    EXPECT_CALL(*matchRepoMock, Create(::testing::_))
        .Times(0);

    auto result = matchDelegate->generateNextRound(MATCH_ID, TOURNAMENT_ID);

    ASSERT_TRUE(result.has_value());
}

TEST_F(MatchDelegateTest, GenerateNextRound_FinalRound_NoAction) {
    auto mockMatch = std::make_shared<domain::Match>();
    mockMatch->Round() = "Finals";

    EXPECT_CALL(*matchRepoMock, FindByIdAndTournamentId(MATCH_ID, TOURNAMENT_ID))
        .WillOnce(testing::Return(mockMatch));

    // No se esperan mas llamadas para Finals
    EXPECT_CALL(*matchRepoMock, FindMatchesByTournamentAndRound(::testing::_, ::testing::_))
        .Times(0);
    EXPECT_CALL(*matchRepoMock, Create(::testing::_))
        .Times(0);

    auto result = matchDelegate->generateNextRound(MATCH_ID, TOURNAMENT_ID);

    ASSERT_TRUE(result.has_value());
}

TEST_F(MatchDelegateTest, GenerateNextRound_MatchNotFound_ReturnsError) {
    EXPECT_CALL(*matchRepoMock, FindByIdAndTournamentId(MATCH_ID, TOURNAMENT_ID))
        .WillOnce(testing::Return(nullptr));

    auto result = matchDelegate->generateNextRound(MATCH_ID, TOURNAMENT_ID);

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), "Match not found.");
}

TEST_F(MatchDelegateTest, GenerateNextRound_RepositoryThrows_ReturnsError) {
    EXPECT_CALL(*matchRepoMock, FindByIdAndTournamentId(MATCH_ID, TOURNAMENT_ID))
        .WillOnce(testing::Throw(std::runtime_error("Database connection failed")));

    auto result = matchDelegate->generateNextRound(MATCH_ID, TOURNAMENT_ID);

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), "Database connection failed");
}
TEST_F(MatchDelegateTest, GenerateNextRound_GroupToConference_CreatesCorrectMatches) {
    auto triggerMatch = std::make_shared<domain::Match>();
    triggerMatch->Round() = "Group";
    triggerMatch->Id() = "match-170";

    std::vector<std::shared_ptr<domain::Match>> groupMatches = {
        createMockMatch("match-167", "team-14", "Team 14", "team-2", "Team 2", "Group", 7, 5),
        createMockMatch("match-168", "team-9", "Team 9", "team-4", "Team 4", "Group", 8, 4),
        createMockMatch("match-169", "team-24", "Team 24", "team-19", "Team 19", "Group", 10, 6),
        createMockMatch("match-170", "team-17", "Team 17", "team-21", "Team 21", "Group", 3, 9)
    };

    auto mockGroups = createMockGroups(8);

    EXPECT_CALL(*matchRepoMock, FindByIdAndTournamentId(MATCH_ID, TOURNAMENT_ID))
        .WillOnce(testing::Return(triggerMatch));

    EXPECT_CALL(*matchRepoMock, FindMatchesByTournamentAndRound(TOURNAMENT_ID, "Group"))
        .Times(2)
        .WillRepeatedly(testing::Return(groupMatches));

    EXPECT_CALL(*groupRepoMock, FindByTournamentId(TOURNAMENT_ID))
        .WillOnce(testing::Return(mockGroups));

    EXPECT_CALL(*matchRepoMock, Create(::testing::_))
        .Times(2)
        .WillRepeatedly(testing::Return("new-match-id"));

    auto result = matchDelegate->generateNextRound(MATCH_ID, TOURNAMENT_ID);
    ASSERT_TRUE(result.has_value());
}

TEST_F(MatchDelegateTest, GenerateNextRound_ConferenceToFinals_CreatesCorrectMatch) {
    auto triggerMatch = std::make_shared<domain::Match>();
    triggerMatch->Round() = "Conference";
    triggerMatch->Id() = "match-172";

    std::vector<std::shared_ptr<domain::Match>> conferenceMatches = {
        createMockMatch("match-171", "team-14", "Team 14", "team-9", "Team 9", "Conference", 3, 4),  // team-9 wins
        createMockMatch("match-172", "team-24", "Team 24", "team-21", "Team 21", "Conference", 8, 2)  // team-24 wins
    };

    EXPECT_CALL(*matchRepoMock, FindByIdAndTournamentId(MATCH_ID, TOURNAMENT_ID))
        .WillOnce(testing::Return(triggerMatch));

    EXPECT_CALL(*matchRepoMock, FindMatchesByTournamentAndRound(TOURNAMENT_ID, "Conference"))
        .Times(2)
        .WillRepeatedly(testing::Return(conferenceMatches));

    EXPECT_CALL(*matchRepoMock, Create(::testing::_))
        .Times(1)
        .WillOnce(testing::Invoke([](const domain::Match& match) {
            EXPECT_EQ(match.Round(), "Finals");
            EXPECT_EQ(match.Home().Id, "team-9");
            EXPECT_EQ(match.Home().Name, "Team 9");
            EXPECT_EQ(match.Visitor().Id, "team-24");
            EXPECT_EQ(match.Visitor().Name, "Team 24");
            return "match-173";
        }));

    auto result = matchDelegate->generateNextRound(MATCH_ID, TOURNAMENT_ID);
    ASSERT_TRUE(result.has_value());
}