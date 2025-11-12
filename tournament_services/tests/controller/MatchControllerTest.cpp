#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <memory>
#include <vector>
#include <expected>
#include <string>
#include <crow.h>
#include <nlohmann/json.hpp>

#include "delegate/IMatchDelegate.hpp"
#include "controller/MatchController.hpp"
#include "domain/Match.hpp"
#include "domain/Team.hpp"
#include "domain/Utilities.hpp"
#include "common/Constants.hpp"

class MatchDelegateMock : public IMatchDelegate {
public:
    MOCK_METHOD((std::expected<std::vector<domain::Match>, std::string>), GetMatches, (std::string_view tournamentId, std::string_view filter), (override));
    MOCK_METHOD((std::expected<domain::Match, std::string>), GetMatch, (std::string_view tournamentId, std::string_view matchId), (override));
    MOCK_METHOD((std::expected<void, std::string>), UpdateMatchScore, (std::string_view tournamentId, std::string_view matchId, const domain::Score& score), (override));
    MOCK_METHOD((std::expected<std::string, std::string>), CreateMatch, (domain::Match& match), (override));
    MOCK_METHOD((std::expected<std::vector<domain::Match>, std::string>), GetMatchesByRound, (std::string_view tournamentId, std::string_view round), (override));
};

// Test Fixture
class MatchControllerTest : public ::testing::Test {
protected:
    std::shared_ptr<MatchDelegateMock> delegateMock;
    std::shared_ptr<MatchController> controller;

    // IDs válidos para las pruebas
    const std::string VALID_TOURNAMENT_ID = "0b9b3f3e-8f4b-4a3e-9c1d-0b7a8e1f2a3b";
    const std::string VALID_MATCH_ID = "c4e1b8a1-3b7c-4c6e-8d2f-1c5a9b3d4e5f";

    void SetUp() override {
        delegateMock = std::make_shared<MatchDelegateMock>();
        controller = std::make_shared<MatchController>(delegateMock);
    }
};

// Pruebas para GET /tournaments/{id}/matches
TEST_F(MatchControllerTest, GetMatches_Success200_NoFilter) {
    std::vector<domain::Match> matches = {domain::Match(), domain::Match()};
    EXPECT_CALL(*delegateMock, GetMatches(VALID_TOURNAMENT_ID, ""))
        .WillOnce(testing::Return(matches));
    
    crow::request req; // Petición vacía (sin filtro)

    crow::response res = controller->GetMatches(req, VALID_TOURNAMENT_ID);

    EXPECT_EQ(res.code, crow::OK);
    auto body = nlohmann::json::parse(res.body);
    ASSERT_TRUE(body.is_array());
    ASSERT_EQ(body.size(), 2);
}

TEST_F(MatchControllerTest, GetMatches_Success200_WithFilter) {
    std::vector<domain::Match> matches = {domain::Match()}; // Simulamos que el filtro devuelve 1 partido
    EXPECT_CALL(*delegateMock, GetMatches(VALID_TOURNAMENT_ID, "played"))
        .WillOnce(testing::Return(matches));

    crow::request req;

    crow::query_string query_params("?showMatches=played");

    req.url_params = query_params;

    crow::response res = controller->GetMatches(req, VALID_TOURNAMENT_ID);

    EXPECT_EQ(res.code, crow::OK);
    auto body = nlohmann::json::parse(res.body);
    ASSERT_EQ(body.size(), 1);
}

TEST_F(MatchControllerTest, GetMatches_Success200_WithFilterPending) {
    std::vector<domain::Match> matches = {domain::Match()}; // Simulamos que el filtro devuelve 1 partido
    EXPECT_CALL(*delegateMock, GetMatches(VALID_TOURNAMENT_ID, "pending"))
        .WillOnce(testing::Return(matches));

    crow::request req;

    crow::query_string query_params("?showMatches=pending");

    req.url_params = query_params;

    crow::response res = controller->GetMatches(req, VALID_TOURNAMENT_ID);

    EXPECT_EQ(res.code, crow::OK);
    auto body = nlohmann::json::parse(res.body);
    ASSERT_EQ(body.size(), 1);
}

TEST_F(MatchControllerTest, GetMatches_Success200_EmptyArray) {
    // Preparamos un vector de 'Match' vacío.
    std::vector<domain::Match> emptyMatches;

    // Configuramos el mock para que devuelva el vector vacío.
    EXPECT_CALL(*delegateMock, GetMatches(VALID_TOURNAMENT_ID, ""))
        .WillOnce(testing::Return(emptyMatches));

    crow::request req;

    crow::response res = controller->GetMatches(req, VALID_TOURNAMENT_ID);

    EXPECT_EQ(res.code, crow::OK);

    // Verificamos que el cuerpo de la respuesta es un arreglo JSON vacío.
    auto body = nlohmann::json::parse(res.body);
    ASSERT_TRUE(body.is_array());
    EXPECT_EQ(body.size(), 0);
}

TEST_F(MatchControllerTest, GetMatches_TournamentNotFound404) {
    EXPECT_CALL(*delegateMock, GetMatches(VALID_TOURNAMENT_ID, ""))
        .WillOnce(testing::Return(std::unexpected("Tournament not found.")));
    crow::request req;

    crow::response res = controller->GetMatches(req, VALID_TOURNAMENT_ID);

    EXPECT_EQ(res.code, crow::NOT_FOUND);
    EXPECT_EQ(res.body, "Tournament not found.");
}

TEST_F(MatchControllerTest, GetMatches_InvalidId400) {
    crow::request req;
    // No se configura EXPECT_CALL porque el controlador debe fallar antes

    crow::response res = controller->GetMatches(req, "id-invalido");

    EXPECT_EQ(res.code, crow::BAD_REQUEST);
}

TEST_F(MatchControllerTest, GetMatches_InternalServerError500) {
    crow::request req;

    // Simulamos que el Delegate falla con un error genérico.
    EXPECT_CALL(*delegateMock, GetMatches(VALID_TOURNAMENT_ID, ""))
        .WillOnce(testing::Return(std::unexpected("System error: Database connection failed.")));

    crow::response res = controller->GetMatches(req, VALID_TOURNAMENT_ID);

    // Verificamos que el código de respuesta es 500.
    EXPECT_EQ(res.code, crow::INTERNAL_SERVER_ERROR);

    EXPECT_EQ(res.body, "System error: Database connection failed.");
}

// Pruebas para GET /tournaments/{id}/matches/{id}
TEST_F(MatchControllerTest, GetMatch_Success200) {
    domain::Match match;
    match.Id() = VALID_MATCH_ID;
    EXPECT_CALL(*delegateMock, GetMatch(VALID_TOURNAMENT_ID, VALID_MATCH_ID))
        .WillOnce(testing::Return(match));

    crow::response res = controller->GetMatch(VALID_TOURNAMENT_ID, VALID_MATCH_ID);

    EXPECT_EQ(res.code, crow::OK);
    auto body = nlohmann::json::parse(res.body);
    EXPECT_EQ(body["id"], VALID_MATCH_ID);
}

TEST_F(MatchControllerTest, GetMatch_NotFound404) {
    EXPECT_CALL(*delegateMock, GetMatch(VALID_TOURNAMENT_ID, VALID_MATCH_ID))
        .WillOnce(testing::Return(std::unexpected("Match not found.")));

    crow::response res = controller->GetMatch(VALID_TOURNAMENT_ID, VALID_MATCH_ID);

    EXPECT_EQ(res.code, crow::NOT_FOUND);
}

TEST_F(MatchControllerTest, GetMatch_InternalServerError500) {
    EXPECT_CALL(*delegateMock, GetMatch(VALID_TOURNAMENT_ID, VALID_MATCH_ID))
        .WillOnce(testing::Return(std::unexpected("System error: Something broke.")));

    crow::response res = controller->GetMatch(VALID_TOURNAMENT_ID, VALID_MATCH_ID);

    EXPECT_EQ(res.code, crow::INTERNAL_SERVER_ERROR);
    EXPECT_EQ(res.body, "System error: Something broke.");
}

// --- Pruebas para PATCH /tournaments/{id}/matches/{id} ---
TEST_F(MatchControllerTest, UpdateMatchScore_Success204) {
    crow::request req;
    req.body = R"({ "score": { "home": 10, "visitor": 5 } })";
    domain::Score capturedScore;

    EXPECT_CALL(*delegateMock, UpdateMatchScore(VALID_TOURNAMENT_ID, VALID_MATCH_ID, ::testing::_))
        .WillOnce(testing::DoAll(
            testing::SaveArg<2>(&capturedScore), // Captura el objeto Score
            testing::Return(std::expected<void, std::string>{})
        ));

    crow::response res = controller->UpdateMatchScore(req, VALID_TOURNAMENT_ID, VALID_MATCH_ID);

    EXPECT_EQ(res.code, crow::NO_CONTENT);
    // Verifica que la transformación JSON -> Score fue correcta
    EXPECT_EQ(capturedScore.homeTeamScore, 10);
    EXPECT_EQ(capturedScore.visitorTeamScore, 5);
}

TEST_F(MatchControllerTest, UpdateMatchScore_MatchNotFound404) {
    crow::request req;
    req.body = R"({ "score": { "home": 10, "visitor": 5 } })";
    
    EXPECT_CALL(*delegateMock, UpdateMatchScore(::testing::_, ::testing::_, ::testing::_))
        .WillOnce(testing::Return(std::unexpected("Match not found.")));

    crow::response res = controller->UpdateMatchScore(req, VALID_TOURNAMENT_ID, VALID_MATCH_ID);

    EXPECT_EQ(res.code, crow::NOT_FOUND);
}

TEST_F(MatchControllerTest, UpdateMatchScore_InvalidScore422) {
    crow::request req;
    req.body = R"({ "score": { "home": -1, "visitor": 5 } })";
    
    EXPECT_CALL(*delegateMock, UpdateMatchScore(::testing::_, ::testing::_, ::testing::_))
        .WillOnce(testing::Return(std::unexpected("Score cannot be negative.")));

    crow::response res = controller->UpdateMatchScore(req, VALID_TOURNAMENT_ID, VALID_MATCH_ID);

    EXPECT_EQ(res.code, 422); // Unprocessable Entity
    EXPECT_EQ(res.body, "Score cannot be negative.");
}

TEST_F(MatchControllerTest, UpdateMatchScore_InvalidJsonBody400) {
    crow::request req;
    req.body = R"({ "score": { "home": "diez" } })"; // "diez" no es un int
    // No se espera llamada al delegate

    crow::response res = controller->UpdateMatchScore(req, VALID_TOURNAMENT_ID, VALID_MATCH_ID);

    EXPECT_EQ(res.code, crow::BAD_REQUEST);
}

TEST_F(MatchControllerTest, UpdateMatchScore_InternalServerError500) {
    crow::request req;
    req.body = R"({ "score": { "home": 10, "visitor": 5 } })";

    // Simulamos que el Delegate falla con un error genérico
    EXPECT_CALL(*delegateMock, UpdateMatchScore(::testing::_, ::testing::_, ::testing::_))
        .WillOnce(testing::Return(std::unexpected("Database connection failed.")));

    crow::response res = controller->UpdateMatchScore(req, VALID_TOURNAMENT_ID, VALID_MATCH_ID);

    // Verificamos que el código de respuesta es 500.
    EXPECT_EQ(res.code, crow::INTERNAL_SERVER_ERROR);

    EXPECT_EQ(res.body, "Database connection failed.");
}