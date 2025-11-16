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
    std::vector<std::shared_ptr<domain::Match>> firstRoundMatches;
    const std::vector<int> firstRoundInfo = {0, 4, 3, 6, 1, 5, 8, 2, 2, 6, 6, 5, 3, 7, 2, 4, 
                                             0, 8, 2, 10, 1, 9, 0, 3, 2, 10, 9, 10, 3, 11, 7, 3, 
                                             0, 12, 7, 5, 1, 13, 2, 0, 2, 14, 5, 2, 3, 15, 4, 7,
                                             0, 16, 8, 0, 1, 17, 6, 8, 2, 18, 6, 6, 3, 19, 6, 6,
                                             0, 20, 3, 2, 1, 21, 10, 2, 2, 22, 3, 1, 3, 23, 2, 0,
                                             0, 24, 10, 10, 1, 25, 5, 8, 2, 26, 9, 1, 3, 27, 9, 4, 
                                             0, 28, 10, 10, 1, 29, 3, 8, 2, 30, 1, 6, 3, 31, 4, 6, 
                                             4, 8, 4, 7, 5, 9, 10, 4, 6, 10, 1, 9, 7, 11, 1, 5, 
                                             4, 12, 10, 9, 5, 13, 6, 5, 6, 14, 1, 8, 7, 15, 0, 4, 
                                             4, 16, 0, 6, 5, 17, 4, 4, 6, 18, 8, 1, 7, 19, 9, 1, 
                                             4, 20, 4, 2, 5, 21, 4, 7, 6, 22, 0, 3, 7, 23, 2, 4, 
                                             4, 24, 8, 3, 5, 25, 3, 5, 6, 26, 5, 3, 7, 27, 0, 1, 
                                             4, 28, 0, 9, 5, 29, 8, 0, 6, 30, 7, 10, 7, 31, 6, 1, 
                                             8, 12, 0, 10, 9, 13, 4, 5, 10, 14, 4, 9, 11, 15, 2, 6, 
                                             8, 16, 3, 5, 9, 17, 7, 4, 10, 18, 1, 1, 11, 19, 5, 5, 
                                             8, 20, 1, 5, 9, 21, 10, 2, 10, 22, 7, 3, 11, 23, 7, 6, 
                                             8, 24, 3, 7, 9, 25, 2, 0, 10, 26, 1, 9, 11, 27, 10, 5, 
                                             8, 28, 8, 5, 9, 29, 6, 4, 10, 30, 1, 8, 11, 31, 4, 1, 
                                             12, 16, 10, 7, 13, 17, 7, 2, 14, 18, 0, 0, 15, 19, 1, 9, 
                                             12, 20, 8, 5, 13, 21, 0, 10, 14, 22, 10, 6, 15, 23, 4, 1, 
                                             12, 24, 10, 9, 13, 25, 0, 5, 14, 26, 8, 3, 15, 27, 5, 1, 
                                             12, 28, 6, 7, 13, 29, 5, 6, 14, 30, 4, 0, 15, 31, 6, 4, 
                                             16, 20, 8, 2, 17, 21, 1, 2, 18, 22, 1, 3, 19, 23, 9, 5, 
                                             16, 24, 0, 3, 17, 25, 8, 2, 18, 26, 2, 10, 19, 27, 1, 4, 
                                             16, 28, 1, 9, 17, 29, 8, 7, 18, 30, 3, 4, 19, 31, 9, 8, 
                                             20, 24, 3, 9, 21, 25, 7, 5, 22, 26, 6, 2, 23, 27, 3, 10, 
                                             20, 28, 2, 7, 21, 29, 9, 3, 22, 30, 4, 2, 23, 31, 4, 8, 
                                             24, 28, 3, 2, 25, 29, 1, 4, 26, 30, 8, 2, 27, 31, 3, 3, 
                                             0, 1, 6, 8, 0, 2, 4, 8, 0, 3, 9, 8, 1, 2, 10, 7, 1, 3, 10, 6, 2, 3, 0, 0, 
                                             4, 5, 10, 5, 4, 6, 5, 6, 4, 7, 7, 6, 5, 6, 9, 10, 5, 7, 5, 3, 6, 7, 6, 2, 
                                             8, 9, 0, 2, 8, 10, 4, 1, 8, 11, 7, 4, 9, 10, 2, 4, 9, 11, 2, 8, 10, 11, 7, 3, 
                                             12, 13, 2, 6, 12, 14, 5, 2, 12, 15, 3, 2, 13, 14, 9, 10, 13, 15, 2, 2, 14, 15, 4, 2, 
                                             16, 17, 0, 9, 16, 18, 10, 7, 16, 19, 4, 4, 17, 18, 8, 0, 17, 19, 4, 3, 18, 19, 6, 9, 
                                             20, 21, 8, 10, 20, 22, 2, 8, 20, 23, 9, 1, 21, 22, 2, 4, 21, 23, 0, 5, 22, 23, 10, 8, 
                                             24, 25, 2, 1, 24, 26, 9, 7, 24, 27, 10, 1, 25, 26, 6, 8, 25, 27, 5, 10, 26, 27, 7, 4, 
                                             28, 29, 6, 6, 28, 30, 2, 3, 28, 31, 9, 8, 29, 30, 7, 4, 29, 31, 6, 1, 30, 31, 7, 10};
    const std::vector<int> teamIndexes = {0, 4, 8, 12, 16, 20, 24, 112, 113, 114, 
                                          1, 5, 9, 13, 17, 21, 25, 112, 115, 116, 
                                          2, 6, 10, 14, 18, 22, 26, 113, 115, 117, 
                                          3, 7, 11, 15, 19, 23, 27, 114, 116, 117, 
                                          0, 28, 32, 36, 40, 44, 48, 118, 119, 120, 
                                          1, 29, 33, 37, 41, 45, 49, 118, 121, 122, 
                                          2, 30, 34, 38, 42, 46, 50, 119, 121, 123, 
                                          3, 31, 35, 39, 43, 47, 51, 120, 121, 123, 
                                          4, 28, 52, 56, 60, 64, 68, 124, 125, 126, 
                                          5, 29, 53, 57, 61, 65, 69, 124, 127, 128, 
                                          6, 30, 54, 58, 62, 66, 70, 125, 127, 129, 
                                          7, 31, 55, 59, 63, 67, 71, 126, 128, 129, 
                                          8, 32, 52, 72, 76, 80, 84, 130, 131, 132, 
                                          9, 33, 53, 73, 77, 81, 85, 130, 133, 134, 
                                          10, 34, 54, 74, 78, 82, 86, 131, 133, 135, 
                                          11, 35, 55, 75, 79, 83, 87, 132, 134, 135, 
                                          12, 36, 56, 72, 88, 92, 96, 136, 137, 138, 
                                          13, 37, 57, 73, 89, 93, 97, 136, 139, 140, 
                                          14, 38, 58, 74, 90, 94, 98, 137, 139, 141, 
                                          15, 39, 59, 75, 91, 95, 99, 138, 140, 141, 
                                          16, 40, 60, 76, 88, 100, 104, 142, 143, 144, 
                                          17, 41, 61, 77, 89, 101, 105, 142, 145, 146, 
                                          18, 42, 62, 78, 90, 102, 106, 143, 145, 147, 
                                          19, 43, 63, 79, 91, 103, 107, 144, 146, 147, 
                                          20, 44, 64, 80, 92, 100, 108, 148, 149, 150, 
                                          21, 45, 65, 81, 93, 101, 109, 148, 151, 152, 
                                          22, 46, 66, 82, 94, 102, 110, 149, 151, 153, 
                                          23, 47, 67, 83, 95, 103, 111, 150, 152, 153, 
                                          24, 48, 68, 84, 96, 104, 108, 154, 155, 156, 
                                          25, 49, 69, 85, 97, 105, 109, 154, 157, 158, 
                                          26, 50, 70, 86, 98, 106, 110, 155, 157, 159, 
                                          27, 51, 71, 87, 99, 107, 111, 156, 158, 159};

    void SetUp() override {
        matchRepoMock = std::make_shared<MatchRepositoryMock>();
        groupRepoMock = std::make_shared<GroupRepositoryMock>();
        matchDelegate = std::make_shared<MatchDelegate>(matchRepoMock, groupRepoMock);
        setUpMatches();
    }

    void setUpMatches() {
        for (int i=0; i < 160; ++i) {
            std::string home = std::to_string(firstRoundInfo[4*i]);
            std::string visitor = std::to_string(firstRoundInfo[(4*i)+1]);
            int homeScore = firstRoundInfo[(4*i)+2];
            int visitorScore = firstRoundInfo[(4*i)+3];

            auto match = std::make_shared<domain::Match>();
            match->Id() = "match-" + std::to_string(i);
            match->TournamentId() = TOURNAMENT_ID;
            match->Home() = domain::Team{"team-" + home, "Team " + home};
            match->Visitor() = domain::Team{"team-" + visitor, "Team " + visitor};
            match->Round() = "First Round";
            match->Score() = domain::Score{homeScore, visitorScore};

            firstRoundMatches.push_back(match);
        }
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

    std::shared_ptr<domain::Match> createMockMatch(const std::string& id, const std::string& homeId, const std::string& visitorId,
                                                   const std::string& round, int homeScore = -1, int visitorScore = -1) {
        auto match = std::make_shared<domain::Match>();
        match->Id() = id;
        match->Home() = domain::Team{"team-" + homeId, "Team " + homeId};
        match->Visitor() = domain::Team{"team-" + visitorId, "Team " + visitorId};
        match->Round() = round;
        match->TournamentId() = TOURNAMENT_ID;

        if (homeScore >= 0 && homeScore <= 10 && visitorScore >= 0 && visitorScore <= 10) {
            if (round == "First Round" || homeScore != visitorScore) {
                match->Score() = domain::Score{homeScore, visitorScore};
            }
        }

        return match;
    }
};

TEST_F(MatchDelegateTest, CreateFirstRoundMatches_Success) {
    auto mockGroups = createMockGroups(8);

    EXPECT_CALL(*groupRepoMock, FindByTournamentId(TOURNAMENT_ID))
        .WillOnce(testing::Return(mockGroups));

    
    std::array<domain::Match, 160> capturedMatches;
    {
        testing::InSequence seq;

        // 160 matches (112 intergroup + 48 intra-group)
        for (int i=0; i < 160; ++i) {
            EXPECT_CALL(*matchRepoMock, Create(::testing::_))
                .WillOnce(testing::DoAll(
                    testing::SaveArg<0>(&capturedMatches[i]),
                    testing::Return("match-" + std::to_string(i))
                ))
                .RetiresOnSaturation();
        }
    }

    auto result = matchDelegate->createFirstRoundMatches(TOURNAMENT_ID);

    ASSERT_TRUE(result.has_value());

    // Revisar que las matches sean iguales a las de firstRoundMatches
    for (int i=0; i < 160; ++i) {
        ASSERT_EQ(capturedMatches[i].Home().Id, firstRoundMatches[i]->Home().Id);
        ASSERT_EQ(capturedMatches[i].Visitor().Id, firstRoundMatches[i]->Visitor().Id);
    }
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


TEST_F(MatchDelegateTest, GenerateNextRound_FirstRoundToWildCard_CreatesCorrectMatches) {
    auto mockMatch = std::make_shared<domain::Match>();
    mockMatch->Round() = "First Round";

    std::vector<std::shared_ptr<domain::Match>> wildCardMatches = {
        createMockMatch("match-161", "1", "2", "Wild Card"),
        createMockMatch("match-162", "9", "12", "Wild Card"),
        createMockMatch("match-163", "4", "15", "Wild Card"),
        createMockMatch("match-164", "22", "19", "Wild Card"),
        createMockMatch("match-165", "17", "29", "Wild Card"),
        createMockMatch("match-166", "28", "21", "Wild Card")
    };

    EXPECT_CALL(*matchRepoMock, FindByIdAndTournamentId(MATCH_ID, TOURNAMENT_ID))
        .WillOnce(testing::Return(mockMatch));

    EXPECT_CALL(*matchRepoMock, FindMatchesByTournamentAndRound(TOURNAMENT_ID, "First Round"))
        .WillOnce(testing::Return(firstRoundMatches));

    // Mock playoff teams
    EXPECT_CALL(*groupRepoMock, FindByTournamentId(TOURNAMENT_ID))
        .WillOnce(testing::Return(createMockGroups(8)));

    std::array<domain::Match, 6> capturedMatches;
    {
        testing::InSequence seq;

        for (int i=0; i < 32; ++i) {
            std::vector<std::shared_ptr<domain::Match>> matches;
            for (int j=0; j < 10; ++j) {
                matches.push_back(firstRoundMatches[teamIndexes[i*10+j]]);
            }

            EXPECT_CALL(*matchRepoMock, GetMatchesByTeamId(TOURNAMENT_ID, "team-" + std::to_string(i)))
                .WillOnce(testing::Return(matches))
                .RetiresOnSaturation();
        }

        for (int i=0; i < 6; ++i) {
            EXPECT_CALL(*matchRepoMock, Create(::testing::_))
                .WillOnce(testing::DoAll(
                    testing::SaveArg<0>(&capturedMatches[i]),
                    testing::Return(MATCH_ID)
                ))
                .RetiresOnSaturation();
        }
    }

    auto result = matchDelegate->generateNextRound(MATCH_ID, TOURNAMENT_ID);
    ASSERT_TRUE(result.has_value());

    // Checando que los matches creados con los que ya estan estipulados
    for (int i=0; i < 6; ++i) {
        ASSERT_EQ(capturedMatches[i].Home().Id, wildCardMatches[i]->Home().Id);
        ASSERT_EQ(capturedMatches[i].Visitor().Id, wildCardMatches[i]->Visitor().Id);
    }
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

TEST_F(MatchDelegateTest, GenerateNextRound_WildCardToGroup_CreatesCorrectMatches) {
    auto triggerMatch = std::make_shared<domain::Match>();
    triggerMatch->Round() = "Wild Card";
    triggerMatch->Id() = "match-166";

    std::vector<std::shared_ptr<domain::Match>> wildCardMatches = {
        createMockMatch("match-161", "1", "2", "Wild Card", 4, 7),
        createMockMatch("match-162", "9", "12", "Wild Card", 5, 3),
        createMockMatch("match-163", "4", "15", "Wild Card", 8, 3),
        createMockMatch("match-164", "22", "19", "Wild Card", 6, 10),
        createMockMatch("match-165", "17", "29", "Wild Card", 2, 1),
        createMockMatch("match-166", "28", "21", "Wild Card", 2, 9)
    };

    std::vector<std::shared_ptr<domain::Match>> groupMatches = {
        createMockMatch("match-167", "14", "2", "Group"),
        createMockMatch("match-168", "9", "4", "Group"),
        createMockMatch("match-169", "24", "19", "Group"),
        createMockMatch("match-170", "17", "21", "Group")
    };

    auto mockGroups = createMockGroups(8);

    EXPECT_CALL(*matchRepoMock, FindByIdAndTournamentId(MATCH_ID, TOURNAMENT_ID))
        .WillOnce(testing::Return(triggerMatch));

    EXPECT_CALL(*matchRepoMock, FindMatchesByTournamentAndRound(TOURNAMENT_ID, "Group"))
        .Times(2)
        .WillRepeatedly(testing::Return(wildCardMatches));

    EXPECT_CALL(*groupRepoMock, FindByTournamentId(TOURNAMENT_ID))
        .WillOnce(testing::Return(mockGroups));

    std::array<domain::Match, 4> capturedMatches;
    {
        testing::InSequence seq;

        for (int i=0; i < 32; ++i) {
            std::vector<std::shared_ptr<domain::Match>> matches;
            for (int j=0; j < 10; ++j) {
                matches.push_back(firstRoundMatches[teamIndexes[i*10+j]]);
            }

            EXPECT_CALL(*matchRepoMock, GetMatchesByTeamId(TOURNAMENT_ID, "team-" + std::to_string(i)))
                .WillOnce(testing::Return(matches))
                .RetiresOnSaturation();
        }

        for (int i=0; i < 4; ++i) {
            EXPECT_CALL(*matchRepoMock, Create(::testing::_))
                .WillOnce(testing::DoAll(
                    testing::SaveArg<0>(&capturedMatches[i]),
                    testing::Return(MATCH_ID)
                ))
                .RetiresOnSaturation();
        }
    }

    auto result = matchDelegate->generateNextRound(MATCH_ID, TOURNAMENT_ID);
    ASSERT_TRUE(result.has_value());

    // Checando que los matches creados con los que ya estan estipulados
    for (int i=0; i < 4; ++i) {
        ASSERT_EQ(capturedMatches[i].Home().Id, groupMatches[i]->Home().Id);
        ASSERT_EQ(capturedMatches[i].Visitor().Id, groupMatches[i]->Visitor().Id);
    }
}

TEST_F(MatchDelegateTest, GenerateNextRound_GroupToConference_CreatesCorrectMatches) {
    auto triggerMatch = std::make_shared<domain::Match>();
    triggerMatch->Round() = "Group";
    triggerMatch->Id() = "match-170";

    std::vector<std::shared_ptr<domain::Match>> groupMatches = {
        createMockMatch("match-167", "14", "2", "Group", 7, 5),
        createMockMatch("match-168", "9", "4", "Group", 8, 4),
        createMockMatch("match-169", "24", "19", "Group", 10, 6),
        createMockMatch("match-170", "17", "21", "Group", 3, 9)
    };

    std::vector<std::shared_ptr<domain::Match>> conferenceMatches = {
        createMockMatch("match-171", "14", "9", "Conference"),
        createMockMatch("match-172", "24", "21", "Conference")
    };

    auto mockGroups = createMockGroups(8);

    EXPECT_CALL(*matchRepoMock, FindByIdAndTournamentId(MATCH_ID, TOURNAMENT_ID))
        .WillOnce(testing::Return(triggerMatch));

    EXPECT_CALL(*matchRepoMock, FindMatchesByTournamentAndRound(TOURNAMENT_ID, "Group"))
        .Times(2)
        .WillRepeatedly(testing::Return(groupMatches));

    EXPECT_CALL(*groupRepoMock, FindByTournamentId(TOURNAMENT_ID))
        .WillOnce(testing::Return(mockGroups));

    std::array<domain::Match, 2> capturedMatches;
    {
        testing::InSequence seq;

        for (int i=0; i < 2; ++i) {
            EXPECT_CALL(*matchRepoMock, Create(::testing::_))
                .WillOnce(testing::DoAll(
                    testing::SaveArg<0>(&capturedMatches[i]),
                    testing::Return(MATCH_ID)
                ))
                .RetiresOnSaturation();
        }
    }

    auto result = matchDelegate->generateNextRound(MATCH_ID, TOURNAMENT_ID);
    ASSERT_TRUE(result.has_value());

    // Checando que los matches creados con los que ya estan estipulados
    for (int i=0; i < 2; ++i) {
        ASSERT_EQ(capturedMatches[i].Home().Id, conferenceMatches[i]->Home().Id);
        ASSERT_EQ(capturedMatches[i].Visitor().Id, conferenceMatches[i]->Visitor().Id);
    }
}

TEST_F(MatchDelegateTest, GenerateNextRound_ConferenceToFinals_CreatesCorrectMatch) {
    auto triggerMatch = std::make_shared<domain::Match>();
    triggerMatch->Round() = "Conference";
    triggerMatch->Id() = "match-172";

    std::vector<std::shared_ptr<domain::Match>> conferenceMatches = {
        createMockMatch("match-171", "14", "9", "Conference", 3, 4),  // team-9 wins
        createMockMatch("match-172", "24", "21", "Conference", 8, 2)  // team-24 wins
    };

    std::shared_ptr<domain::Match> finalMatch = createMockMatch("match-173", "9", "24", "Finals");

    EXPECT_CALL(*matchRepoMock, FindByIdAndTournamentId(MATCH_ID, TOURNAMENT_ID))
        .WillOnce(testing::Return(triggerMatch));

    EXPECT_CALL(*matchRepoMock, FindMatchesByTournamentAndRound(TOURNAMENT_ID, "Conference"))
        .Times(2)
        .WillRepeatedly(testing::Return(conferenceMatches));

    domain::Match capturedMatch;
    EXPECT_CALL(*matchRepoMock, Create(::testing::_))
        .WillOnce(testing::DoAll(
            testing::SaveArg<0>(&capturedMatch), 
            testing::Return(MATCH_ID)
        ));

    auto result = matchDelegate->generateNextRound(MATCH_ID, TOURNAMENT_ID);
    ASSERT_TRUE(result.has_value());

    ASSERT_EQ(capturedMatch.Home().Id, finalMatch->Home().Id);
    ASSERT_EQ(capturedMatch.Visitor().Id, finalMatch->Visitor().Id);
}