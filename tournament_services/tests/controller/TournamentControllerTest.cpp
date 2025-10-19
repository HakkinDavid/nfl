#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <crow.h>

#include "domain/Team.hpp"
#include "delegate/ITournamentDelegate.hpp"
#include "controller/TournamentController.hpp"
#include "domain/Utilities.hpp"

class TournamentDelegateMock : public ITournamentDelegate {
    public:
    MOCK_METHOD(std::shared_ptr<domain::Tournament>, GetTournament, (const std::string_view id), (override));
    MOCK_METHOD(std::vector<std::shared_ptr<domain::Tournament>>, ReadAll, (), (override));
    MOCK_METHOD((std::expected<std::string, std::string>), CreateTournament, (const std::shared_ptr<domain::Tournament> tournament), (override));
    MOCK_METHOD((std::expected<std::string, std::string>), UpdateTournament, (const std::shared_ptr<domain::Tournament> tournament), (override));
    MOCK_METHOD((std::expected<void, std::string>), DeleteTournament, (const std::string& tournamentId), (override));
};

class TournamentControllerTest : public ::testing::Test{
protected:
    std::shared_ptr<TournamentDelegateMock> tournamentDelegateMock;
    std::shared_ptr<TournamentController> tournamentController;

    void SetUp() override {
        tournamentDelegateMock = std::make_shared<TournamentDelegateMock>();
        tournamentController = std::make_shared<TournamentController>(TournamentController(tournamentDelegateMock));
    }

    // TearDown() function
    void TearDown() override {
        // teardown code comes here
    }

};

// --- Pruebas para POST /tournaments ---

TEST_F(TournamentControllerTest, CreateTournament_Success201) {
    domain::Tournament capturedTournament;
    EXPECT_CALL(*tournamentDelegateMock, CreateTournament(::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedTournament),
                testing::Return(std::expected<std::string, std::string>{"new-id"})
            )
        );

    nlohmann::json tournamentRequestBody = {{"name", "new tournament"}};
    crow::request tournamentRequest;
    tournamentRequest.body = tournamentRequestBody.dump();

    crow::response response = tournamentController->CreateTournament(tournamentRequest);

    testing::Mock::VerifyAndClearExpectations(&tournamentDelegateMock);

    EXPECT_EQ(crow::CREATED, response.code);
    EXPECT_EQ("new-id", response.get_header_value("location"));
    EXPECT_EQ(tournamentRequestBody.at("name").get<std::string>(), capturedTournament.Name());
}

TEST_F(TournamentControllerTest, CreateTournament_Conflict409) {
    EXPECT_CALL(*tournamentDelegateMock, CreateTournament(::testing::_))
        .WillOnce(testing::Return(std::unexpected("Entry already exists.")));

    nlohmann::json tournamentRequestBody = {{"name", "Existing Tournament Name"}};
    crow::request tournamentRequest;
    tournamentRequest.body = tournamentRequestBody.dump();

    crow::response response = tournamentController->CreateTournament(tournamentRequest);

    EXPECT_EQ(crow::CONFLICT, response.code);
}

// --- Pruebas para GET /tournaments/{id} ---

TEST_F(TournamentControllerTest, GetTournamentById_OK200) {
    const std::string validUuid = "8f1b5b6a-7b8c-4a3e-9c1d-0b7a8e1f2a3b";
    auto expectedTournament = std::make_shared<domain::Tournament>();
    expectedTournament->Id() = validUuid;
    expectedTournament->Name() = "Tournament Name";
    // Mas datos sobre los grupos, etc podrian incluirse aqui

    EXPECT_CALL(*tournamentDelegateMock, GetTournament(testing::Eq(validUuid)))
        .WillOnce(testing::Return(expectedTournament));

    crow::response response = tournamentController->GetTournament(validUuid);
    auto jsonResponse = crow::json::load(response.body);

    EXPECT_EQ(crow::OK, response.code);
    EXPECT_EQ(expectedTournament->Id(), jsonResponse["id"]);
    EXPECT_EQ(expectedTournament->Name(), jsonResponse["name"]);
}

TEST_F(TournamentControllerTest, GetTournamentNotFound_nullptr404) {
    const std::string validUuid = "8f1b5b6a-7b8c-4a3e-9c1d-0b7a8e1f2a3b";
    EXPECT_CALL(*tournamentDelegateMock, GetTournament(testing::Eq(validUuid)))
        .WillOnce(testing::Return(nullptr));

    crow::response response = tournamentController->GetTournament(validUuid);

    EXPECT_EQ(crow::NOT_FOUND, response.code);
}

TEST_F(TournamentControllerTest, GetTournamentById_ErrorFormat400) {
    crow::response badRequest = tournamentController->GetTournament("");
    EXPECT_EQ(badRequest.code, crow::BAD_REQUEST);

    badRequest = tournamentController->GetTournament("mfasd#*");
    EXPECT_EQ(badRequest.code, crow::BAD_REQUEST);
}

// --- Pruebas para GET /tournaments ---

TEST_F(TournamentControllerTest, ReadAll_ReturnsListOfTournaments200) {
    auto tournament1 = std::make_shared<domain::Tournament>();
    auto tournament2 = std::make_shared<domain::Tournament>();
    tournament1->Id() = "id-1"; tournament1->Name() = "Tournament 1";
    tournament2->Id() = "id-2"; tournament2->Name() = "Tournament 2";
    std::vector<std::shared_ptr<domain::Tournament>> expectedTournaments = {tournament1, tournament2};

    EXPECT_CALL(*tournamentDelegateMock, ReadAll())
        .WillOnce(testing::Return(expectedTournaments));

    crow::response response = tournamentController->ReadAll();
    auto jsonResponse = nlohmann::json::parse(response.body);

    EXPECT_EQ(crow::OK, response.code);
    ASSERT_TRUE(jsonResponse.is_array());
    ASSERT_EQ(2, jsonResponse.size());
    EXPECT_EQ("id-1", jsonResponse[0]["id"]);
    EXPECT_EQ("Tournament 2", jsonResponse[1]["name"]);
}

TEST_F(TournamentControllerTest, ReadAll_ReturnsEmptyList200) {
    std::vector<std::shared_ptr<domain::Tournament>> emptyList;

    EXPECT_CALL(*tournamentDelegateMock, ReadAll())
        .WillOnce(testing::Return(emptyList));

    crow::response response = tournamentController->ReadAll();
    auto jsonResponse = nlohmann::json::parse(response.body);

    EXPECT_EQ(crow::OK, response.code);
    ASSERT_TRUE(jsonResponse.is_array());
    EXPECT_EQ(0, jsonResponse.size());
}

// --- Pruebas para PATCH /tournaments/{id} ---

TEST_F(TournamentControllerTest, UpdateTournament_Success204) {
    const std::string tournamentIdToUpdate = "8f1b5b6a-7b8c-4a3e-9c1d-0b7a8e1f2a3b";
    std::shared_ptr<domain::Tournament> capturedTournament;

    EXPECT_CALL(*tournamentDelegateMock, UpdateTournament(::testing::_))
        .WillOnce(testing::DoAll(
            testing::SaveArg<0>(&capturedTournament),
            testing::Return(std::expected<std::string, std::string>{tournamentIdToUpdate})
        ));

    nlohmann::json tournamentRequestBody = {{"id", tournamentIdToUpdate}, {"name", "Updated Tournament Name"}};
    crow::request tournamentRequest;
    tournamentRequest.body = tournamentRequestBody.dump();

    crow::response response = tournamentController->UpdateTournament(tournamentRequest);

    EXPECT_EQ(crow::NO_CONTENT, response.code);

    EXPECT_EQ(tournamentIdToUpdate, capturedTournament->Id());
    EXPECT_EQ("Updated Tournament Name", capturedTournament->Name());
}

TEST_F(TournamentControllerTest, UpdateTournament_NotFound404) {
    const std::string tournamentIdToUpdate = "8f1b5b6a-7b8c-4a3e-9c1d-0b7a8e1f2a3b";

    EXPECT_CALL(*tournamentDelegateMock, UpdateTournament(::testing::_))
    .WillOnce(testing::Return(std::unexpected("Entry not found.")));

    nlohmann::json tournamentRequestBody = {{"id", tournamentIdToUpdate}, {"name", "Updated Tournament Name"}};
    crow::request tournamentRequest;
    tournamentRequest.body = tournamentRequestBody.dump();

    crow::response response = tournamentController->UpdateTournament(tournamentRequest);

    EXPECT_EQ(crow::NOT_FOUND, response.code);
}

TEST_F(TournamentControllerTest, UpdateTournament_Conflict409) {
    const std::string tournamentIdToUpdate = "8f1b5b6a-7b8c-4a3e-9c1d-0b7a8e1f2a3b";

    EXPECT_CALL(*tournamentDelegateMock, UpdateTournament(::testing::_))
    .WillOnce(testing::Return(std::unexpected("Entry already exists.")));

    nlohmann::json tournamentRequestBody = {{"id", tournamentIdToUpdate}, {"name", "Duplicated Tournament Name"}};
    crow::request tournamentRequest;
    tournamentRequest.body = tournamentRequestBody.dump();

    crow::response response = tournamentController->UpdateTournament(tournamentRequest);

    EXPECT_EQ(crow::CONFLICT, response.code);
}

// --- Pruebas para DELETE /tournaments/{id} ---

TEST_F(TournamentControllerTest, DeleteTournament_Success204) {
    const std::string tournamentIdToDelete = "feb3b050-f7b8-4610-808a-1b01b8d61f2e";

    EXPECT_CALL(*tournamentDelegateMock, DeleteTournament(tournamentIdToDelete))
        .WillOnce(testing::Return(std::expected<void, std::string>{}));

    crow::response response = tournamentController->DeleteTournament(tournamentIdToDelete);

    EXPECT_EQ(crow::NO_CONTENT, response.code);
}

TEST_F(TournamentControllerTest, DeleteTournament_NotFound404) {
    const std::string tournamentIdToDelete = "feb3b050-f7b8-4610-808a-1b01b8d61f2e";

    EXPECT_CALL(*tournamentDelegateMock, DeleteTournament(tournamentIdToDelete))
        .WillOnce(testing::Return(std::unexpected("Entry not found.")));

    crow::response response = tournamentController->DeleteTournament(tournamentIdToDelete);

    EXPECT_EQ(crow::NOT_FOUND, response.code);
}

TEST_F(TournamentControllerTest, DeleteTournament_InvalidFormat400) {
    crow::response badRequest = tournamentController->DeleteTournament("");
    EXPECT_EQ(badRequest.code, crow::BAD_REQUEST);

    badRequest = tournamentController->DeleteTournament("mfasd#*");
    EXPECT_EQ(badRequest.code, crow::BAD_REQUEST);
}