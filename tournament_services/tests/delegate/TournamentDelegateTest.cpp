#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <memory>
#include <vector>
#include <expected>
#include <string>

#include "persistence/repository/IRepository.hpp"
#include "domain/Tournament.hpp"
#include "domain/Utilities.hpp"
#include "delegate/TournamentDelegate.hpp"
#include "cms/QueueMessageProducer.hpp"
#include "cms/IQueueMessageProducer.hpp"


// Mock for TournamentRepository
class TournamentRepositoryMock : public IRepository<domain::Tournament, std::string> {
public:
    MOCK_METHOD((std::shared_ptr<domain::Tournament>), ReadById, (std::string id), (override));
    MOCK_METHOD(std::string, Create, (const domain::Tournament& entity), (override));
    MOCK_METHOD(std::string, Update, (const domain::Tournament& entity), (override));
    MOCK_METHOD(void, Delete, (std::string id), (override));
    MOCK_METHOD(std::vector<std::shared_ptr<domain::Tournament>>, ReadAll, (), (override));
};

class QueueMessageProducerMock : public IQueueMessageProducer {
public:
    MOCK_METHOD(void, SendMessage, (const std::string_view& message, const std::string_view& routingKey), (override));
};

class TournamentDelegateTest : public ::testing::Test {
protected:
    std::shared_ptr<TournamentRepositoryMock> tournamentRepositoryMock;
    std::shared_ptr<IQueueMessageProducer> queueProducerMock;          // Interface pointer
    std::shared_ptr<QueueMessageProducerMock> queueProducerMockConcrete; // Concrete mock pointer
    std::shared_ptr<TournamentDelegate> tournamentDelegate;

    void SetUp() override {
        tournamentRepositoryMock = std::make_shared<TournamentRepositoryMock>();
        queueProducerMockConcrete = std::make_shared<QueueMessageProducerMock>();
        queueProducerMock = queueProducerMockConcrete;  // Both reference same object
        tournamentDelegate = std::make_shared<TournamentDelegate>(tournamentRepositoryMock, queueProducerMock);
    }
};

/*class TournamentDelegateTest : public ::testing::Test {
protected:
    std::shared_ptr<TournamentRepositoryMock> tournamentRepositoryMock;
    std::shared_ptr<QueueMessageProducerMock> queueProducerMock;
    std::shared_ptr<TournamentDelegate> tournamentDelegate;

    void SetUp() override {
        tournamentRepositoryM Old Tournament Delegate DeletagateTest class ,k keeping just in case = std::make_shared<TournamentRepositoryMock>();
        queueProducerMock = std::make_shared<QueueMessageProducerMock>();
        tournamentDelegate = std::make_shared<TournamentDelegate>(tournamentRepositoryMock, queueProducerMock);
    }
};*/
TEST_F(TournamentDelegateTest, CreateTournament_Success) {
    auto tournamentToCreate = std::make_shared<domain::Tournament>();
    tournamentToCreate->Name() = "New Tournament";
    std::string generatedId = "new-tournament-uuid";
    domain::Tournament capturedTournament;

    EXPECT_CALL(*tournamentRepositoryMock, Create(::testing::_))
        .WillOnce(testing::DoAll(
            testing::SaveArg<0>(&capturedTournament),
            testing::Return(generatedId)
        ));

    EXPECT_CALL(*queueProducerMockConcrete, SendMessage(generatedId, "tournament.created"))
        .Times(1);

    auto result = tournamentDelegate->CreateTournament(tournamentToCreate);

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(generatedId, result.value());
    EXPECT_EQ(tournamentToCreate->Name(), capturedTournament.Name());
}

TEST_F(TournamentDelegateTest, CreateTournament_FailsOnDuplicate) {
    auto tournamentToCreate = std::make_shared<domain::Tournament>();
    tournamentToCreate->Name() = "Existing Tournament";

    EXPECT_CALL(*tournamentRepositoryMock, Create(::testing::_))
    .WillOnce(testing::Throw(domain::DuplicateEntryException()));

    EXPECT_CALL(*queueProducerMockConcrete, SendMessage(::testing::_, ::testing::_))
        .Times(0);

    auto result = tournamentDelegate->CreateTournament(tournamentToCreate);

    ASSERT_FALSE(result.has_value());
    EXPECT_NE(std::string::npos, result.error().find("Entry already exists"));
}

// Tests for GetTournament - ALREADY CORRECT
TEST_F(TournamentDelegateTest, GetTournament_Success) {
    std::string tournamentId = "existing-uuid";
    auto expectedTournament = std::make_shared<domain::Tournament>();
    expectedTournament->Id() = tournamentId;
    expectedTournament->Name() = "Found Tournament";

    EXPECT_CALL(*tournamentRepositoryMock, ReadById(tournamentId))
        .WillOnce(testing::Return(expectedTournament));

    std::shared_ptr<domain::Tournament> result = tournamentDelegate->GetTournament(tournamentId);

    ASSERT_NE(nullptr, result);
    EXPECT_EQ(expectedTournament->Id(), result->Id());
    EXPECT_EQ(expectedTournament->Name(), result->Name());
}

TEST_F(TournamentDelegateTest, GetTournament_NotFoundReturnsNull) {
    std::string tournamentId = "non-existent-uuid";

    EXPECT_CALL(*tournamentRepositoryMock, ReadById(tournamentId))
        .WillOnce(testing::Return(nullptr));

    std::shared_ptr<domain::Tournament> result = tournamentDelegate->GetTournament(tournamentId);

    EXPECT_EQ(nullptr, result);
}

TEST_F(TournamentDelegateTest, GetAllTournaments_ReturnsList) {
    auto tournament1 = std::make_shared<domain::Tournament>();
    tournament1->Id() = "id-1";
    tournament1->Name() = "Tournament 1";

    auto tournament2 = std::make_shared<domain::Tournament>();
    tournament2->Id() = "id-2";
    tournament2->Name() = "Tournament 2";

    std::vector<std::shared_ptr<domain::Tournament>> expectedList = {tournament1, tournament2};

    EXPECT_CALL(*tournamentRepositoryMock, ReadAll())
        .WillOnce(testing::Return(expectedList));

    auto result = tournamentDelegate->ReadAll();

    ASSERT_EQ(2, result.size());
    EXPECT_EQ("Tournament 1", result[0]->Name());
    EXPECT_EQ("Tournament 2", result[1]->Name());
}

TEST_F(TournamentDelegateTest, GetAllTournaments_ReturnsEmptyList) {
    std::vector<std::shared_ptr<domain::Tournament>> emptyList;

    EXPECT_CALL(*tournamentRepositoryMock, ReadAll())
        .WillOnce(testing::Return(emptyList));

    auto result = tournamentDelegate->ReadAll();

    EXPECT_TRUE(result.empty());
}

TEST_F(TournamentDelegateTest, UpdateTournament_Success) {
    std::string tournamentId = "existing-uuid";
    auto updatePayload = std::make_shared<domain::Tournament>();
    updatePayload->Name() = "Updated Name";
    updatePayload->Id() = tournamentId;

    domain::Tournament capturedTournament;

    EXPECT_CALL(*tournamentRepositoryMock, Update(::testing::_))
        .WillOnce(testing::DoAll(
            testing::SaveArg<0>(&capturedTournament),
            testing::Return(tournamentId)
        ));

    EXPECT_CALL(*queueProducerMockConcrete, SendMessage(tournamentId, "tournament.updated"))
        .Times(1);

    auto result = tournamentDelegate->UpdateTournament(updatePayload);

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(tournamentId, result.value());
    EXPECT_EQ("Updated Name", capturedTournament.Name());
    EXPECT_EQ(tournamentId, capturedTournament.Id());
}

TEST_F(TournamentDelegateTest, UpdateTournament_FailsOnNotFound) {
    auto updatePayload = std::make_shared<domain::Tournament>();
    updatePayload->Name() = "Updated Name";

    EXPECT_CALL(*tournamentRepositoryMock, Update(::testing::_))
    .WillOnce(testing::Throw(domain::NotFoundException()));

    EXPECT_CALL(*queueProducerMockConcrete, SendMessage(::testing::_, ::testing::_))
        .Times(0);

    auto result = tournamentDelegate->UpdateTournament(updatePayload);

    ASSERT_FALSE(result.has_value());
    EXPECT_NE(std::string::npos, result.error().find("Entry not found"));
}

TEST_F(TournamentDelegateTest, DeleteTournament_Success) {
    const std::string tournamentId = "tournament-to-delete";

    EXPECT_CALL(*tournamentRepositoryMock, Delete(tournamentId))
        .Times(1);

    EXPECT_CALL(*queueProducerMockConcrete, SendMessage(tournamentId, "tournament.deleted"))
        .Times(1);

    auto result = tournamentDelegate->DeleteTournament(tournamentId);

    EXPECT_TRUE(result.has_value());
}

TEST_F(TournamentDelegateTest, DeleteTournament_FailsOnNotFound) {
    const std::string tournamentId = "non-existent-tournament";

    EXPECT_CALL(*tournamentRepositoryMock, Delete(tournamentId))
    .WillOnce(testing::Throw(domain::NotFoundException()));

    EXPECT_CALL(*queueProducerMockConcrete, SendMessage(::testing::_, ::testing::_))
        .Times(0);

    auto result = tournamentDelegate->DeleteTournament(tournamentId);

    ASSERT_FALSE(result.has_value());
    EXPECT_NE(std::string::npos, result.error().find("Entry not found"));
}