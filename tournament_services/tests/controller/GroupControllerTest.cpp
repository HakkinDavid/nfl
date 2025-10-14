#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <memory>
#include <vector>
#include <expected>
#include <string>
#include <crow.h>
#include <nlohmann/json.hpp>

#include "delegate/IGroupDelegate.hpp"
#include "controller/GroupController.hpp"
#include "domain/Group.hpp"
#include "domain/Team.hpp"
#include "common/Constants.hpp"

class GroupDelegateMock : public IGroupDelegate {
public:
    MOCK_METHOD((std::expected<std::vector<domain::Group>, std::string>), GetGroups, (std::string tournamentId), (override));
    MOCK_METHOD((std::expected<domain::Group, std::string>), GetGroup, (std::string tournamentId, std::string_view groupId), (override));
    MOCK_METHOD((std::expected<std::string, std::string>), CreateGroup, (std::string tournamentId, domain::Group& group), (override));
    MOCK_METHOD((std::expected<void, std::string>), AddTeamToGroup, (std::string_view tournamentId, std::string_view groupId, const domain::Team& team), (override));
    MOCK_METHOD((std::expected<void, std::string>), UpdateGroupName, (std::string_view tournamentId, std::string_view groupId, const domain::Group& groupUpdatePayload), (override));
    MOCK_METHOD((std::expected<void, std::string>), DeleteGroup, (std::string_view tournamentId, std::string groupId), (override));
};

class GroupControllerTest : public ::testing::Test {
protected:
    std::shared_ptr<GroupDelegateMock> groupDelegateMock;
    std::shared_ptr<GroupController> groupController;

    const std::string VALID_TOURNAMENT_ID = "0b9b3f3e-8f4b-4a3e-9c1d-0b7a8e1f2a3b";
    const std::string VALID_GROUP_ID = "c4e1b8a1-3b7c-4c6e-8d2f-1c5a9b3d4e5f";

    void SetUp() override {
        groupDelegateMock = std::make_shared<GroupDelegateMock>();
        groupController = std::make_shared<GroupController>(groupDelegateMock);
    }
};

// --- Pruebas para GET /tournaments/{id}/groups ---
TEST_F(GroupControllerTest, GetGroups_Success200) {
    std::vector<domain::Group> groups = {domain::Group("Group A"), domain::Group("Group B")};
    EXPECT_CALL(*groupDelegateMock, GetGroups(VALID_TOURNAMENT_ID))
        .WillOnce(testing::Return(groups));

    crow::response res = groupController->GetGroups(VALID_TOURNAMENT_ID);

    EXPECT_EQ(res.code, crow::OK);
    auto body = nlohmann::json::parse(res.body);
    ASSERT_EQ(body.size(), 2);
    EXPECT_EQ(body[0]["name"], "Group A");
}

TEST_F(GroupControllerTest, GetGroups_TournamentNotFound404) {
    EXPECT_CALL(*groupDelegateMock, GetGroups(VALID_TOURNAMENT_ID))
        .WillOnce(testing::Return(std::unexpected("Tournament not found.")));

    crow::response res = groupController->GetGroups(VALID_TOURNAMENT_ID);

    EXPECT_EQ(res.code, crow::NOT_FOUND);
}

// Pruebas para GET /tournaments/{id}/groups/{id}
TEST_F(GroupControllerTest, GetGroup_Success200) {
    domain::Group group("Group A", VALID_GROUP_ID);
    EXPECT_CALL(*groupDelegateMock, GetGroup(VALID_TOURNAMENT_ID, VALID_GROUP_ID))
        .WillOnce(testing::Return(group));

    crow::response res = groupController->GetGroup(VALID_TOURNAMENT_ID, VALID_GROUP_ID);

    EXPECT_EQ(res.code, crow::OK);
    auto body = nlohmann::json::parse(res.body);
    EXPECT_EQ(body["id"], VALID_GROUP_ID);
    EXPECT_EQ(body["name"], "Group A");
}

TEST_F(GroupControllerTest, GetGroup_NotFound404) {
    EXPECT_CALL(*groupDelegateMock, GetGroup(VALID_TOURNAMENT_ID, VALID_GROUP_ID))
        .WillOnce(testing::Return(std::unexpected("Group not found.")));

    crow::response res = groupController->GetGroup(VALID_TOURNAMENT_ID, VALID_GROUP_ID);

    EXPECT_EQ(res.code, crow::NOT_FOUND);
}

// Pruebas para POST /tournaments/{id}/groups

TEST_F(GroupControllerTest, CreateGroup_Success201) {
    const std::string newGroupId = "new-group-id-789";
    domain::Group capturedGroup;
    nlohmann::json requestBody = {{"name", "Group C"}};
    crow::request req;
    req.body = requestBody.dump();
    
    EXPECT_CALL(*groupDelegateMock, CreateGroup(VALID_TOURNAMENT_ID, ::testing::_))
        .WillOnce(testing::DoAll(
            testing::SaveArg<1>(&capturedGroup), // Captura el objeto Group
            testing::Return(newGroupId)
        ));

    crow::response res = groupController->CreateGroup(req, VALID_TOURNAMENT_ID);

    EXPECT_EQ(res.code, crow::CREATED);
    EXPECT_EQ(res.get_header_value("Location"), newGroupId);
    EXPECT_EQ(capturedGroup.Name(), "Group C"); // Verifica la transformación JSON -> Objeto
}

TEST_F(GroupControllerTest, CreateGroup_Unprocessable422) {
    nlohmann::json requestBody = {{"name", "Group D"}};
    crow::request req;
    req.body = requestBody.dump();
    
    EXPECT_CALL(*groupDelegateMock, CreateGroup(VALID_TOURNAMENT_ID, ::testing::_))
        .WillOnce(testing::Return(std::unexpected("Maximum number of groups has been reached.")));

    crow::response res = groupController->CreateGroup(req, VALID_TOURNAMENT_ID);

    EXPECT_EQ(res.code, 422); // Unprocessable Entity
    EXPECT_EQ(res.body, "Maximum number of groups has been reached.");
}

// Pruebas para POST /tournaments/{id}/groups/{id}/teams

TEST_F(GroupControllerTest, AddTeamToGroup_Success204) {
    domain::Team capturedTeam;
    nlohmann::json requestBody = {{"id", "team-123"}, {"name", "Team Rocket"}};
    crow::request req;
    req.body = requestBody.dump();

    EXPECT_CALL(*groupDelegateMock, AddTeamToGroup(VALID_TOURNAMENT_ID, VALID_GROUP_ID, ::testing::_))
        .WillOnce(testing::DoAll(
            testing::SaveArg<2>(&capturedTeam), // Captura el objeto Team
            testing::Return(std::expected<void, std::string>{})
        ));

    crow::response res = groupController->AddTeamToGroup(req, VALID_TOURNAMENT_ID, VALID_GROUP_ID);

    EXPECT_EQ(res.code, crow::NO_CONTENT);
    EXPECT_EQ(capturedTeam.Id, "team-123");
    EXPECT_EQ(capturedTeam.Name, "Team Rocket");
}

TEST_F(GroupControllerTest, AddTeamToGroup_GroupIsFull422) {
    crow::request req;
    req.body = R"({"id": "team-123", "name": "Team Aqua"})";
    
    EXPECT_CALL(*groupDelegateMock, AddTeamToGroup(::testing::_, ::testing::_, ::testing::_))
        .WillOnce(testing::Return(std::unexpected("Group is already full.")));

    crow::response res = groupController->AddTeamToGroup(req, VALID_TOURNAMENT_ID, VALID_GROUP_ID);

    EXPECT_EQ(res.code, 422);
    EXPECT_EQ(res.body, "Group is already full.");
}

TEST_F(GroupControllerTest, AddTeamToGroup_TeamNotFound422) {
    crow::request req;
    req.body = R"({"id": "non-existent-team", "name": "Ghost Team"})";

    EXPECT_CALL(*groupDelegateMock, AddTeamToGroup(::testing::_, ::testing::_, ::testing::_))
        .WillOnce(testing::Return(std::unexpected("Team with ID non-existent-team does not exist.")));

    crow::response res = groupController->AddTeamToGroup(req, VALID_TOURNAMENT_ID, VALID_GROUP_ID);

    EXPECT_EQ(res.code, 422);
}

// Pruebas para PATCH /tournaments/{id}/groups/{id}

TEST_F(GroupControllerTest, UpdateGroupName_Success204) {
    domain::Group capturedGroup;
    crow::request req;
    req.body = R"({"name": "New Group Name"})";

    EXPECT_CALL(*groupDelegateMock, UpdateGroupName(::testing::_, ::testing::_, ::testing::_))
        .WillOnce(testing::DoAll(
            testing::SaveArg<2>(&capturedGroup),
            testing::Return(std::expected<void, std::string>{})
        ));

    crow::response res = groupController->UpdateGroupName(req, VALID_TOURNAMENT_ID, VALID_GROUP_ID);

    EXPECT_EQ(res.code, crow::NO_CONTENT);
    EXPECT_EQ(capturedGroup.Name(), "New Group Name");
}

TEST_F(GroupControllerTest, UpdateGroupName_NotFound404) {
    crow::request req;
    req.body = R"({"name": "New Group Name"})";

    EXPECT_CALL(*groupDelegateMock, UpdateGroupName(::testing::_, ::testing::_, ::testing::_))
        .WillOnce(testing::Return(std::unexpected("Group not found in this tournament.")));

    crow::response res = groupController->UpdateGroupName(req, VALID_TOURNAMENT_ID, VALID_GROUP_ID);

    EXPECT_EQ(res.code, crow::NOT_FOUND);
}

TEST_F(GroupControllerTest, UpdateGroupName_Conflict409) {
    crow::request req;
    req.body = R"({"name": "Existing Group Name"})";

    EXPECT_CALL(*groupDelegateMock, UpdateGroupName(::testing::_, ::testing::_, ::testing::_))
        .WillOnce(testing::Return(std::unexpected("Group name already exists.")));

    crow::response res = groupController->UpdateGroupName(req, VALID_TOURNAMENT_ID, VALID_GROUP_ID);

    EXPECT_EQ(res.code, crow::CONFLICT);
}

// Pruebas para DELETE /tournaments/{id}/groups/{id}

TEST_F(GroupControllerTest, DeleteGroup_Success204) {
    EXPECT_CALL(*groupDelegateMock, DeleteGroup(VALID_TOURNAMENT_ID, VALID_GROUP_ID))
        .WillOnce(testing::Return(std::expected<void, std::string>{}));

    crow::response res = groupController->DeleteGroup(VALID_TOURNAMENT_ID, VALID_GROUP_ID);

    EXPECT_EQ(res.code, crow::NO_CONTENT);
}

TEST_F(GroupControllerTest, DeleteGroup_NotFound404) {
    EXPECT_CALL(*groupDelegateMock, DeleteGroup(VALID_TOURNAMENT_ID, VALID_GROUP_ID))
        .WillOnce(testing::Return(std::unexpected("Group not found.")));

    crow::response res = groupController->DeleteGroup(VALID_TOURNAMENT_ID, VALID_GROUP_ID);

    EXPECT_EQ(res.code, crow::NOT_FOUND);
}