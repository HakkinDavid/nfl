//
// Created by root on 10/1/25.
//

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <memory>
#include <vector>
#include <expected>
#include <string>

#include "persistence/repository/IRepository.hpp"
#include "domain/Team.hpp"
#include "domain/Utilities.hpp"
#include "delegate/TeamDelegate.hpp"

class TeamRepositoryMock : public IRepository<domain::Team, std::string> {
public:
    MOCK_METHOD(std::shared_ptr<domain::Team>, ReadById, (std::string id), (override));
    MOCK_METHOD(std::string, Create, (const domain::Team& entity), (override));
    MOCK_METHOD(std::string, Update, (const domain::Team& entity), (override));
    MOCK_METHOD(void, Delete, (std::string id), (override));
    MOCK_METHOD(std::vector<std::shared_ptr<domain::Team>>, ReadAll, (), (override));
};

class TeamDelegateTest : public ::testing::Test {
protected:
    std::shared_ptr<TeamRepositoryMock> teamRepositoryMock;
    std::shared_ptr<TeamDelegate> teamDelegate;

    void SetUp() override {
        teamRepositoryMock = std::make_shared<TeamRepositoryMock>();
        teamDelegate = std::make_shared<TeamDelegate>(teamRepositoryMock);
    }
};

// Pruebas para SaveTeam
TEST_F(TeamDelegateTest, SaveTeam_Success) {
    domain::Team teamToSave{"", "New Team"};
    domain::Team capturedTeam;
    std::string generatedId = "new-uuid-123";

    // Capturamos el argumento para verificarlo y simulamos que devuelve un ID.
    EXPECT_CALL(*teamRepositoryMock, Create(::testing::_))
        .WillOnce(testing::DoAll(
            testing::SaveArg<0>(&capturedTeam),
            testing::Return(generatedId)
        ));

    auto result = teamDelegate->SaveTeam(teamToSave);

    // Verificamos que el resultado es el esperado.
    ASSERT_TRUE(result.has_value()); // El resultado fue exitoso.
    EXPECT_EQ(generatedId, result.value()); // El ID devuelto es el que generó el repo.
    EXPECT_EQ(teamToSave.Name, capturedTeam.Name); // Se pasó el objeto correcto al repo.
}

TEST_F(TeamDelegateTest, SaveTeam_FailsOnDuplicate) {
    domain::Team teamToSave{"", "Existing Team"};

    // Simulamos que el repositorio lanza una excepción de duplicado.
    EXPECT_CALL(*teamRepositoryMock, Create(::testing::_))
        .WillOnce(testing::Throw(domain::DuplicateEntryException()));

    auto result = teamDelegate->SaveTeam(teamToSave);

    ASSERT_FALSE(result.has_value()); // El resultado fue un error.
    EXPECT_EQ("Entry already exists.", result.error()); // El mensaje de error es el correcto.
}

// Pruebas para GetTeam
TEST_F(TeamDelegateTest, GetTeam_Success) {
    std::string teamId = "existing-uuid";
    auto expectedTeam = std::make_shared<domain::Team>(domain::Team{teamId, "Found Team"});

    // Simulamos que el repositorio encuentra el equipo y lo devuelve.
    EXPECT_CALL(*teamRepositoryMock, ReadById(teamId))
        .WillOnce(testing::Return(expectedTeam));

    std::shared_ptr<domain::Team> result = teamDelegate->GetTeam(teamId);

    ASSERT_NE(nullptr, result); // No es nulo.
    EXPECT_EQ(expectedTeam->Id, result->Id);
    EXPECT_EQ(expectedTeam->Name, result->Name);
}

TEST_F(TeamDelegateTest, GetTeam_NotFoundReturnsNull) {
    std::string teamId = "non-existent-uuid";

    // Simulamos que el repositorio NO encuentra nada y devuelve nullptr.
    EXPECT_CALL(*teamRepositoryMock, ReadById(teamId))
        .WillOnce(testing::Return(nullptr));

    std::shared_ptr<domain::Team> result = teamDelegate->GetTeam(teamId);

    EXPECT_EQ(nullptr, result); // El resultado es nulo, como se esperaba.
}

// Pruebas para GetAllTeams

TEST_F(TeamDelegateTest, GetAllTeams_ReturnsList) {
    auto team1 = std::make_shared<domain::Team>(domain::Team{"id-1", "Team 1"});
    auto team2 = std::make_shared<domain::Team>(domain::Team{"id-2", "Team 2"});
    std::vector<std::shared_ptr<domain::Team>> expectedList = {team1, team2};

    // Simulamos que el repositorio devuelve una lista con equipos.
    EXPECT_CALL(*teamRepositoryMock, ReadAll())
        .WillOnce(testing::Return(expectedList));

    auto result = teamDelegate->GetAllTeams();

    ASSERT_EQ(2, result.size());
    EXPECT_EQ("Team 1", result[0]->Name);
}

TEST_F(TeamDelegateTest, GetAllTeams_ReturnsEmptyList) {
    std::vector<std::shared_ptr<domain::Team>> emptyList;

    // Simulamos que el repositorio devuelve una lista vacía.
    EXPECT_CALL(*teamRepositoryMock, ReadAll())
        .WillOnce(testing::Return(emptyList));

    auto result = teamDelegate->GetAllTeams();

    EXPECT_TRUE(result.empty());
}

// Pruebas para UpdateTeam

TEST_F(TeamDelegateTest, UpdateTeam_Success) {
    std::string teamId = "existing-uuid";
    domain::Team updatePayload{"", "Updated Name"}; // El payload del PATCH no tiene ID
    domain::Team capturedTeam;

    // Simulamos que la actualización en el repo es exitosa.
    EXPECT_CALL(*teamRepositoryMock, Update(::testing::_))
        .WillOnce(testing::DoAll(
            testing::SaveArg<0>(&capturedTeam),
            testing::Return(teamId)
        ));

    auto result = teamDelegate->UpdateTeam(teamId, updatePayload);

    ASSERT_TRUE(result.has_value()); // Fue exitoso.
    EXPECT_EQ(teamId, result.value()); // El ID es el correcto.
    // Verificar que el objeto pasado al repo se construyó correctamente con el ID de la URL y el nombre del payload.
    EXPECT_EQ("Updated Name", capturedTeam.Name);
    EXPECT_EQ(teamId, capturedTeam.Id);
}

TEST_F(TeamDelegateTest, UpdateTeam_FailsOnNotFound) {
    std::string teamId = "non-existent-uuid";
    domain::Team updatePayload{"", "Updated Name"};

    // Simulamos que el repo lanza una excepción porque no encuentra el ID.
    EXPECT_CALL(*teamRepositoryMock, Update(::testing::_))
        .WillOnce(testing::Throw(domain::NotFoundException()));

    auto result = teamDelegate->UpdateTeam(teamId, updatePayload);

    ASSERT_FALSE(result.has_value()); // Fue un error.
    EXPECT_EQ("Entry not found.", result.error()); // El mensaje es el correcto.
}