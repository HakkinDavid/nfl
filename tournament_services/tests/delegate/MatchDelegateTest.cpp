#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <memory>
#include <vector>
#include <expected>
#include <string>

#include "persistence/repository/IMatchRepository.hpp"
#include "cms/IQueueMessageProducer.hpp"
#include "domain/Tournament.hpp"
#include "domain/Match.hpp"
#include "domain/Utilities.hpp"
#include "delegate/MatchDelegate.hpp"

class TournamentRepositoryMock : public IRepository<domain::Tournament, std::string> {
public:
    MOCK_METHOD((std::shared_ptr<domain::Tournament>), ReadById, (std::string id), (override));
    MOCK_METHOD(std::string, Create, (const domain::Tournament& entity), (override));
    MOCK_METHOD(std::string, Update, (const domain::Tournament& entity), (override));
    MOCK_METHOD(void, Delete, (std::string id), (override));
    MOCK_METHOD((std::vector<std::shared_ptr<domain::Tournament>>), ReadAll, (), (override));
};

class MatchRepositoryMock : public IMatchRepository {
public:
    // Métodos de IRepository
    MOCK_METHOD((std::shared_ptr<domain::Match>), ReadById, (std::string id), (override));
    MOCK_METHOD(std::string, Create, (const domain::Match& entity), (override));
    MOCK_METHOD(std::string, Update, (const domain::Match& entity), (override));
    MOCK_METHOD(void, Delete, (std::string id), (override));
    MOCK_METHOD((std::vector<std::shared_ptr<domain::Match>>), ReadAll, (), (override));

    // Métodos de IMatchRepository
    MOCK_METHOD((std::shared_ptr<domain::Match>), FindLastOpenMatch, (std::string_view tournamentId), (override));
    MOCK_METHOD((std::vector<std::shared_ptr<domain::Match>>), FindMatchesByTournamentAndRound, (std::string_view tournamentId, std::string_view round), (override));
    MOCK_METHOD((std::vector<std::shared_ptr<domain::Match>>), FindAllByTournamentId, (std::string_view tournamentId), (override));
    MOCK_METHOD((std::shared_ptr<domain::Match>), FindByIdAndTournamentId, (std::string_view matchId, std::string_view tournamentId), (override));
    MOCK_METHOD(std::string, UpdateScore, (std::string_view matchId, std::string_view tournamentId, const domain::Score& score), (override));
};

class QueueMessageProducerMock : public IQueueMessageProducer {
public:
    MOCK_METHOD(void, SendMessage, (const std::string_view& message, const std::string_view& routingKey), (override));
};

// Test Fixture
class MatchDelegateTest : public ::testing::Test {
protected:
    std::shared_ptr<TournamentRepositoryMock> tournamentRepoMock;
    std::shared_ptr<MatchRepositoryMock> matchRepoMock;
    std::shared_ptr<QueueMessageProducerMock> producerMock;
    std::shared_ptr<MatchDelegate> matchDelegate;

    const std::string TOURNAMENT_ID = "tour-123";
    const std::string MATCH_ID = "match-abc";

    void SetUp() override {
        tournamentRepoMock = std::make_shared<TournamentRepositoryMock>();
        matchRepoMock = std::make_shared<MatchRepositoryMock>();
        producerMock = std::make_shared<QueueMessageProducerMock>();

        matchDelegate = std::make_shared<MatchDelegate>(
            tournamentRepoMock,
            matchRepoMock,
            producerMock
        );
    }
};

// Pruebas para GetMatches
TEST_F(MatchDelegateTest, GetMatches_FailsWhenTournamentNotFound) {
    EXPECT_CALL(*tournamentRepoMock, ReadById(TOURNAMENT_ID))
        .WillOnce(testing::Return(nullptr));

    auto result = matchDelegate->GetMatches(TOURNAMENT_ID, "");

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), "Tournament not found.");
}

TEST_F(MatchDelegateTest, GetMatches_Success_NoFilter) {
    auto matchPtr1 = std::make_shared<domain::Match>();
    auto matchPtr2 = std::make_shared<domain::Match>();
    std::vector<std::shared_ptr<domain::Match>> matches = {matchPtr1, matchPtr2};

    EXPECT_CALL(*tournamentRepoMock, ReadById(TOURNAMENT_ID))
        .WillOnce(testing::Return(std::make_shared<domain::Tournament>()));
    EXPECT_CALL(*matchRepoMock, FindAllByTournamentId(TOURNAMENT_ID))
        .WillOnce(testing::Return(matches));

    auto result = matchDelegate->GetMatches(TOURNAMENT_ID, "");

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value().size(), 2);
}

TEST_F(MatchDelegateTest, GetMatches_Success_WithFilter) {
    auto playedMatch = std::make_shared<domain::Match>();
    playedMatch->Score() = domain::Score{1, 0}; // Este partido tiene marcador
    auto pendingMatch = std::make_shared<domain::Match>(); // Este no

    std::vector<std::shared_ptr<domain::Match>> matches = {playedMatch, pendingMatch};

    EXPECT_CALL(*tournamentRepoMock, ReadById(TOURNAMENT_ID))
        .WillOnce(testing::Return(std::make_shared<domain::Tournament>()));
    EXPECT_CALL(*matchRepoMock, FindAllByTournamentId(TOURNAMENT_ID))
        .WillOnce(testing::Return(matches));

    // Pedimos solo los "pendientes"
    auto result = matchDelegate->GetMatches(TOURNAMENT_ID, "pending");

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value().size(), 1); // Solo debe devolver 1 partido
    EXPECT_FALSE(result.value()[0].Score().has_value()); // Verifica que es el pendiente
}

//  Pruebas para GetMatch
TEST_F(MatchDelegateTest, GetMatch_Success) {
    auto match = std::make_shared<domain::Match>();
    match->Id() = MATCH_ID;

    EXPECT_CALL(*matchRepoMock, FindByIdAndTournamentId(MATCH_ID, TOURNAMENT_ID))
        .WillOnce(testing::Return(match));

    auto result = matchDelegate->GetMatch(TOURNAMENT_ID, MATCH_ID);

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value().Id(), MATCH_ID);
}

TEST_F(MatchDelegateTest, GetMatch_FailsWhenNotFound) {
    EXPECT_CALL(*matchRepoMock, FindByIdAndTournamentId(MATCH_ID, TOURNAMENT_ID))
        .WillOnce(testing::Return(nullptr)); // Simulamos que no se encuentra

    auto result = matchDelegate->GetMatch(TOURNAMENT_ID, MATCH_ID);

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), "Match not found in this tournament.");
}

// Pruebas para UpdateMatchScore

TEST_F(MatchDelegateTest, UpdateMatchScore_Success) {
    domain::Score score{10, 5};

    EXPECT_CALL(*matchRepoMock, UpdateScore(MATCH_ID, TOURNAMENT_ID, ::testing::_))
        .WillOnce(testing::Return(MATCH_ID)); // Devuelve el ID al tener éxito

    // Verificamos que se genera el evento
    EXPECT_CALL(*producerMock, SendMessage(::testing::_, "match.score.updated"))
        .Times(1);

    auto result = matchDelegate->UpdateMatchScore(TOURNAMENT_ID, MATCH_ID, score);

    ASSERT_TRUE(result.has_value());
}

TEST_F(MatchDelegateTest, UpdateMatchScore_FailsWithNegativeScore) {
    domain::Score score{-1, 5}; // Marcador inválido

    auto result = matchDelegate->UpdateMatchScore(TOURNAMENT_ID, MATCH_ID, score);

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), "Score cannot be negative.");
}

TEST_F(MatchDelegateTest, UpdateMatchScore_FailsWhenNotFound) {
    domain::Score score{10, 5};

    // Simulamos que el repositorio lanza NotFoundException
    EXPECT_CALL(*matchRepoMock, UpdateScore(MATCH_ID, TOURNAMENT_ID, ::testing::_))
        .WillOnce(testing::Throw(domain::NotFoundException()));

    // El productor de mensajes no debe ser llamado si la actualización falla
    EXPECT_CALL(*producerMock, SendMessage(::testing::_, ::testing::_))
        .Times(0);

    auto result = matchDelegate->UpdateMatchScore(TOURNAMENT_ID, MATCH_ID, score);

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), "Match not found.");
}

// Pruebas para funciones internas

TEST_F(MatchDelegateTest, CreateMatch_Success) {
    domain::Match match;
    std::string newId = "new-match-id";
    EXPECT_CALL(*matchRepoMock, Create(::testing::_))
        .WillOnce(testing::Return(newId));

    auto result = matchDelegate->CreateMatch(match);

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), newId);
}

TEST_F(MatchDelegateTest, GetNextOpenMatch_Success) {
    auto match = std::make_shared<domain::Match>();
    match->Id() = "open-match";

    EXPECT_CALL(*matchRepoMock, FindLastOpenMatch(TOURNAMENT_ID))
        .WillOnce(testing::Return(match));

    auto result = matchDelegate->GetNextOpenMatch(TOURNAMENT_ID);

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value().Id(), "open-match");
}

TEST_F(MatchDelegateTest, GetMatchesByRound_Success) {
    auto match = std::make_shared<domain::Match>();
    std::vector<std::shared_ptr<domain::Match>> matches = {match};

    EXPECT_CALL(*tournamentRepoMock, ReadById(TOURNAMENT_ID))
        .WillOnce(testing::Return(std::make_shared<domain::Tournament>()));
    EXPECT_CALL(*matchRepoMock, FindMatchesByTournamentAndRound(TOURNAMENT_ID, "regular"))
        .WillOnce(testing::Return(matches));

    auto result = matchDelegate->GetMatchesByRound(TOURNAMENT_ID, "regular");

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value().size(), 1);
}