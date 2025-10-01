#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <crow.h>

#include "domain/Team.hpp"
#include "delegate/ITeamDelegate.hpp"
#include "controller/TeamController.hpp"
#include "domain/Utilities.hpp"

class TeamDelegateMock : public ITeamDelegate {
    public:
    MOCK_METHOD(std::shared_ptr<domain::Team>, GetTeam, (const std::string id), (override));
    MOCK_METHOD(std::vector<std::shared_ptr<domain::Team>>, GetAllTeams, (), (override));
    MOCK_METHOD((std::expected<std::string, std::string>), SaveTeam, (const domain::Team& team), (override));
    MOCK_METHOD((std::expected<std::string, std::string>), UpdateTeam, (const std::string& teamId, const domain::Team& team), (override));
};

class TeamControllerTest : public ::testing::Test{
protected:
    std::shared_ptr<TeamDelegateMock> teamDelegateMock;
    std::shared_ptr<TeamController> teamController;

    void SetUp() override {
        teamDelegateMock = std::make_shared<TeamDelegateMock>();
        teamController = std::make_shared<TeamController>(TeamController(teamDelegateMock));
    }

    // TearDown() function
    void TearDown() override {
        // teardown code comes here
    }

};

TEST_F(TeamControllerTest, SaveTeamTest_Success201) {
    domain::Team capturedTeam;
    EXPECT_CALL(*teamDelegateMock, SaveTeam(::testing::_))
        .WillOnce(testing::DoAll(
                testing::SaveArg<0>(&capturedTeam),
                testing::Return(std::expected<std::string, std::string>{"new-id"})
            )
        );

    nlohmann::json teamRequestBody = {{"name", "new team"}};
    crow::request teamRequest;
    teamRequest.body = teamRequestBody.dump();

    crow::response response = teamController->SaveTeam(teamRequest);

    testing::Mock::VerifyAndClearExpectations(&teamDelegateMock);

    EXPECT_EQ(crow::CREATED, response.code);
    EXPECT_EQ("new-id", response.get_header_value("location"));
    EXPECT_EQ(teamRequestBody.at("name").get<std::string>(), capturedTeam.Name);
}

TEST_F(TeamControllerTest, SaveTeam_Conflict409) {
    // Simulamos que el Delegate lanza una DuplicateEntryException
    EXPECT_CALL(*teamDelegateMock, SaveTeam(::testing::_))
    .WillOnce(testing::Return(std::unexpected("Entry already exists.")));

    nlohmann::json teamRequestBody = {{"name", "Existing Team Name"}};
    crow::request teamRequest;
    teamRequest.body = teamRequestBody.dump();

    crow::response response = teamController->SaveTeam(teamRequest);

    // Verificamos que la respuesta sea 409 Conflict.
    EXPECT_EQ(crow::CONFLICT, response.code);
}

TEST_F(TeamControllerTest, GetTeamById_OK200) {
    const std::string validUuid = "8f1b5b6a-7b8c-4a3e-9c1d-0b7a8e1f2a3b";
    std::shared_ptr<domain::Team> expectedTeam = std::make_shared<domain::Team>(domain::Team{validUuid,  "Team Name"});

    EXPECT_CALL(*teamDelegateMock, GetTeam(testing::Eq(validUuid)))
        .WillOnce(testing::Return(expectedTeam));

    crow::response response = teamController->getTeam(validUuid);
    auto jsonResponse = crow::json::load(response.body);

    EXPECT_EQ(crow::OK, response.code);
    EXPECT_EQ(expectedTeam->Id, jsonResponse["id"]);
    EXPECT_EQ(expectedTeam->Name, jsonResponse["name"]);
}

TEST_F(TeamControllerTest, GetTeamNotFound_nullptr404) {
    const std::string validUuid = "8f1b5b6a-7b8c-4a3e-9c1d-0b7a8e1f2a3b";
    EXPECT_CALL(*teamDelegateMock, GetTeam(testing::Eq(validUuid)))
        .WillOnce(testing::Return(nullptr));

    crow::response response = teamController->getTeam(validUuid);

    EXPECT_EQ(crow::NOT_FOUND, response.code);
}

TEST_F(TeamControllerTest, GetTeamNotFound_WhenDelegateThrows_404) {
    const std::string validUuid = "8f1b5b6a-7b8c-4a3e-9c1d-0b7a8e1f2a3b";

    // Simulamos que el Delegate lanza la excepción NotFoundException
    EXPECT_CALL(*teamDelegateMock, GetTeam(testing::Eq(validUuid)))
        .WillOnce(testing::Throw(domain::NotFoundException()));

    crow::response response = teamController->getTeam(validUuid);

    // Verificamos que el resultado sigue siendo 404 Not Found
    EXPECT_EQ(crow::NOT_FOUND, response.code);
}

TEST_F(TeamControllerTest, GetTeamById_ErrorFormat400) {
    crow::response badRequest = teamController->getTeam("");

    EXPECT_EQ(badRequest.code, crow::BAD_REQUEST);

    badRequest = teamController->getTeam("mfasd#*");
    EXPECT_EQ(badRequest.code, crow::BAD_REQUEST);
}

TEST_F(TeamControllerTest, GetAllTeams_ReturnsListOfTeams200) {
    // Preparamos los datos de prueba
    auto team1 = std::make_shared<domain::Team>(domain::Team{"id-1", "Team 1"});
    auto team2 = std::make_shared<domain::Team>(domain::Team{"id-2", "Team 2"});
    std::vector<std::shared_ptr<domain::Team>> expectedTeams = {team1, team2};

    // Simulamos la respuesta del Delegate
    EXPECT_CALL(*teamDelegateMock, GetAllTeams())
        .WillOnce(testing::Return(expectedTeams));

    // Llamamos al Controller
    crow::response response = teamController->getAllTeams();
    auto jsonResponse = nlohmann::json::parse(response.body);

    // Verificamos la respuesta
    EXPECT_EQ(crow::OK, response.code);
    ASSERT_TRUE(jsonResponse.is_array());
    ASSERT_EQ(2, jsonResponse.size());
    EXPECT_EQ("id-1", jsonResponse[0]["id"]);
    EXPECT_EQ("Team 2", jsonResponse[1]["name"]);
}

TEST_F(TeamControllerTest, GetAllTeams_ReturnsEmptyList200) {
    // Preparamos una lista vacía
    std::vector<std::shared_ptr<domain::Team>> emptyList;

    // Simulamos la respuesta del Delegate
    EXPECT_CALL(*teamDelegateMock, GetAllTeams())
        .WillOnce(testing::Return(emptyList));

    // Llamamos al Controller
    crow::response response = teamController->getAllTeams();
    auto jsonResponse = nlohmann::json::parse(response.body);

    // Verificamos la respuesta
    EXPECT_EQ(crow::OK, response.code);
    ASSERT_TRUE(jsonResponse.is_array());
    EXPECT_EQ(0, jsonResponse.size());
}

TEST_F(TeamControllerTest, UpdateTeam_Success204) {
    const std::string teamIdToUpdate = "8f1b5b6a-7b8c-4a3e-9c1d-0b7a8e1f2a3b";
    domain::Team capturedTeam;
    std::string capturedId;

    // Capturamos los argumentos que se le pasan al Delegate
    EXPECT_CALL(*teamDelegateMock, UpdateTeam(::testing::_, ::testing::_))
        .WillOnce(testing::DoAll(
            testing::SaveArg<0>(&capturedId),
            testing::SaveArg<1>(&capturedTeam),
            testing::Return(std::expected<std::string, std::string>{teamIdToUpdate}) // Simulamos una respuesta exitosa
        ));

    nlohmann::json teamRequestBody = {{"name", "Updated Team Name"}};
    crow::request teamRequest;
    teamRequest.body = teamRequestBody.dump();

    crow::response response = teamController->UpdateTeam(teamRequest, teamIdToUpdate);

    // Verificamos que el código sea 204 No Content
    EXPECT_EQ(crow::NO_CONTENT, response.code);

    // Verificamos que el Delegate recibió los datos correctos
    EXPECT_EQ(teamIdToUpdate, capturedId);
    EXPECT_EQ("Updated Team Name", capturedTeam.Name);
}

TEST_F(TeamControllerTest, UpdateTeam_NotFound404) {
    const std::string teamIdToUpdate = "8f1b5b6a-7b8c-4a3e-9c1d-0b7a8e1f2a3b";

    // Simulamos que el Delegate lanza una NotFoundException
    EXPECT_CALL(*teamDelegateMock, UpdateTeam(::testing::_, ::testing::_))
    .WillOnce(testing::Return(std::unexpected("Entry not found.")));

    nlohmann::json teamRequestBody = {{"name", "Updated Team Name"}};
    crow::request teamRequest;
    teamRequest.body = teamRequestBody.dump();

    crow::response response = teamController->UpdateTeam(teamRequest, teamIdToUpdate);

    // Verificamos que la respuesta sea 404 Not Found
    EXPECT_EQ(crow::NOT_FOUND, response.code);
}

TEST_F(TeamControllerTest, UpdateTeam_Conflict409) {
    const std::string teamIdToUpdate = "8f1b5b6a-7b8c-4a3e-9c1d-0b7a8e1f2a3b";

    // Simulamos que el Delegate lanza una DuplicateEntryException
    EXPECT_CALL(*teamDelegateMock, UpdateTeam(::testing::_, ::testing::_))
    .WillOnce(testing::Return(std::unexpected("Entry already exists.")));

    nlohmann::json teamRequestBody = {{"name", "Name that already belongs to another team"}};
    crow::request teamRequest;
    teamRequest.body = teamRequestBody.dump();

    crow::response response = teamController->UpdateTeam(teamRequest, teamIdToUpdate);

    // Verificamos que la respuesta sea 409 Conflict
    EXPECT_EQ(crow::CONFLICT, response.code);
}