#ifndef DOMAIN_UTILITIES_HPP
#define DOMAIN_UTILITIES_HPP

#include <nlohmann/json.hpp>
#include "domain/Team.hpp"
#include "domain/Tournament.hpp"
#include "domain/Group.hpp"
#include "domain/Match.hpp"

namespace domain {
    struct DuplicateEntryException : public std::runtime_error {
        DuplicateEntryException() : std::runtime_error("Entry already exists.") {}
    };

    struct NotFoundException : public std::runtime_error {
        NotFoundException() : std::runtime_error("Entry not found.") {}
    };

    inline void to_json(nlohmann::json& json, const Team& team) {
        json = {{"id", team.Id}, {"name", team.Name}};
    }

    inline void from_json(const nlohmann::json& json, Team& team) {
        if(json.contains("id")) {
            json.at("id").get_to(team.Id);
        }
        json.at("name").get_to(team.Name);
    }

    inline void from_json(const nlohmann::json& json, std::vector<Team>& teams) {
        teams.clear();  // Clear existing teams before deserializing
        for (auto j = json.begin(); j != json.end(); ++j) {
            Team team;
            if(j.value().contains("id")) {
                j.value().at("id").get_to(team.Id);
            }
            if(j.value().contains("name")) {
                j.value().at("name").get_to(team.Name);
            }
            teams.push_back(team);
        }
    }

    inline void to_json(nlohmann::json& json, const std::shared_ptr<Team>& team) {
        json = nlohmann::basic_json();
        json["name"] = team->Name;

        if (!team->Id.empty()) {
            json["id"] = team->Id;
        }
    }

    inline TournamentType fromString(std::string_view type) {
        if (type == "ROUND_ROBIN")
            return TournamentType::ROUND_ROBIN;
        if (type == "NFL")
            return TournamentType::NFL;

        return TournamentType::ROUND_ROBIN;
    }

    inline void from_json(const nlohmann::json& json, TournamentFormat& format) {
        if(json.contains("maxTeamsPerGroup"))
            json.at("maxTeamsPerGroup").get_to(format.MaxTeamsPerGroup());
        if(json.contains("numberOfGroups"))
            json.at("numberOfGroups").get_to(format.NumberOfGroups());
        if(json.contains("type"))
            format.Type() = fromString(json["type"].get<std::string>());
    }

    inline void to_json(nlohmann::json& json, const TournamentFormat& format) {
        json = {{"maxTeamsPerGroup", format.MaxTeamsPerGroup()}, {"numberOfGroups", format.NumberOfGroups()}};
        switch (format.Type()) {
            case TournamentType::ROUND_ROBIN:
                json["type"] = "ROUND_ROBIN";
                break;
            case TournamentType::NFL:
                json["type"] = "NFL";
                break;
            default:
                json["type"] = "ROUND_ROBIN";
        }
    }

    inline void to_json(nlohmann::json& json, const std::shared_ptr<Tournament>& tournament) {
        json = {{"name", tournament->Name()}};
        if (!tournament->Id().empty()) {
            json["id"] = tournament->Id();
        }
        json["format"] = tournament->Format();
    }

    inline void from_json(const nlohmann::json& json, std::shared_ptr<Tournament>& tournament) {
        if(json.contains("id")) {
            tournament->Id() = json["id"].get<std::string>();
        }
        json["name"].get_to(tournament->Name());
        if (json.contains("format"))
            json.at("format").get_to(tournament->Format());
    }

    inline void to_json(nlohmann::json& json, const Tournament& tournament) {
        json = {{"name", tournament.Name()}};
        if (!tournament.Id().empty()) {
            json["id"] = tournament.Id();
        }
        json["format"] = tournament.Format();
    }

    inline void from_json(const nlohmann::json& json, Tournament& tournament) {
        if(json.contains("id")) {
            tournament.Id() = json["id"].get<std::string>();
        }
        json["name"].get_to(tournament.Name());
        if (json.contains("format"))
            json.at("format").get_to(tournament.Format());
    }

    inline void from_json(const nlohmann::json& json, Group& group) {
        json.at("name").get_to(group.Name());

        if (json.contains("id")) {
            json.at("id").get_to(group.Id());
        }
        if (json.contains("tournamentId")) {
            json.at("tournamentId").get_to(group.TournamentId());
        }
        if (json.contains("teams")) {
            json.at("teams").get_to(group.Teams());
        }
        json["name"].get_to(group.Name());
        if (json.contains("teams")) {
            json["teams"].get_to(group.Teams());
        }
    }

    inline void to_json(nlohmann::json& json, const std::shared_ptr<Group>& group) {
        json["name"] = group->Name();
        json["tournamentId"] = group->TournamentId();
        if (!group->Id().empty()) {
            json["id"] = group->Id();
        }
        json["teams"] = group->Teams();
    }

    inline void to_json(nlohmann::json& json, const std::vector<std::shared_ptr<Group>>& groups) {
        json = nlohmann::json::array();
        for (const auto& group : groups) {
            auto jsonGroup = nlohmann::json();
            jsonGroup["name"] = group->Name();
            jsonGroup["tournamentId"] = group->TournamentId();
            if (!group->Id().empty()) {
                jsonGroup["id"] = group->Id();
            }
            jsonGroup["teams"] = group->Teams();
            json.push_back(jsonGroup);
        }
    }

    inline void to_json(nlohmann::json& json, const Group& group) {
        json["name"] = group.Name();
        json["tournamentId"] = group.TournamentId();
        if (!group.Id().empty()) {
            json["id"] = group.Id();
        }
        json["teams"] = group.Teams();
    }
}

namespace nlohmann {
    template <>
    struct adl_serializer<domain::Score> {
        static void to_json(json& j, const domain::Score& s) {
            j = json{{"home", s.homeTeamScore}, {"visitor", s.visitorTeamScore}};
        }
        static void from_json(const json& j, domain::Score& s) {
            j.at("home").get_to(s.homeTeamScore);
            j.at("visitor").get_to(s.visitorTeamScore);
        }
    };

    template <>
    struct adl_serializer<domain::Match> {
        static void to_json(json& j, const domain::Match& m) {
            j["id"] = m.Id();
            j["home"] = m.Home();
            j["visitor"] = m.Visitor();
            j["round"] = m.Round();

            if (m.Score().has_value()) {
                j["score"] = m.Score().value();
            }
        }

        static void from_json(const json& j, domain::Match& m) {
            j.at("home").get_to(m.Home());
            j.at("visitor").get_to(m.Visitor());
            j.at("round").get_to(m.Round());

            if (j.contains("score")) {
                domain::Score temp_score;
                j.at("score").get_to(temp_score);
                m.Score() = temp_score;
            } else {
                m.Score() = std::nullopt;
            }
        }
    };
}

#endif