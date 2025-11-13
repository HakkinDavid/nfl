//
// Created by Meeeeee on 11/10/25.
//

#ifndef CONSUMER_MATCHDELEGATE_HPP
#define CONSUMER_MATCHDELEGATE_HPP

#include <algorithm>
#include <expected>
#include <memory>
#include <tuple>

#include "persistence/repository/IMatchRepository.hpp"
#include "persistence/repository/IGroupRepository.hpp"

using TeamRecord = std::tuple<domain::Team, float, int, int>;

class MatchDelegate {
    std::shared_ptr<IGroupRepository> groupRepository;
    std::shared_ptr<IMatchRepository> matchRepository;
public:
    MatchDelegate(const std::shared_ptr<IMatchRepository>& matchRepository,
                  const std::shared_ptr<IGroupRepository>& groupRepository) 
        : groupRepository(groupRepository), matchRepository(matchRepository) {}

    void createMatch(const domain::Match& match);
    bool checkPrevRound(const std::string& tournamentId, const std::string& round);
    std::expected<void, std::string> generateNextRound(const std::string& matchId, const std::string& tournamentId);

    std::expected<void, std::string> createFirstRoundMatches(const std::string& tournamentId);
    void createWildCardMatches(const std::string& tournamentId);
    void createGroupMatches(const std::string& tournamentId);
    void createConferenceMatches(const std::string& tournamentId);
    void createFinalMatch(const std::string& tournamentId);
    std::vector<domain::Team> getPlayoffTeams(const std::string& tournamentId);
    std::vector<TeamRecord> sortTeams(std::vector<TeamRecord> teams);
};

inline void MatchDelegate::createMatch(const domain::Match& match) {
    std::string matchId = matchRepository->Create(match);
}

inline std::expected<void, std::string> MatchDelegate::createFirstRoundMatches(const std::string& tournamentId) {
    try {
        auto groups = groupRepository->FindByTournamentId(tournamentId);

        for (int g1=0; g1 < 7; ++g1) {
            for (int g2=g1+1; g2 < 8; ++g2) {
                for (int t=0; t < 4; ++t) {
                    domain::Match newMatch;
                    newMatch.TournamentId() = tournamentId;
                    newMatch.Home() = groups[g1]->Teams()[t];
                    newMatch.Visitor() = groups[g2]->Teams()[t];
                    newMatch.Round() = "First Round";

                    createMatch(newMatch);
                }
            }
        }

        for (const auto& group : groups) {
            for(int t1=0; t1 < 3; ++t1) {
                for(int t2=t1+1; t2 < 4; ++t2) {
                    domain::Match newMatch;
                    newMatch.TournamentId() = tournamentId;
                    newMatch.Home() = group->Teams()[t1];
                    newMatch.Visitor() = group->Teams()[t2];
                    newMatch.Round() = "First Round";

                    createMatch(newMatch);
                }
            }
        }

        return {};
    } catch (const std::exception& e) {
        return std::unexpected(e.what());
    }
}

inline bool MatchDelegate::checkPrevRound(const std::string& tournamentId, const std::string& round) {
    bool roundDone = true;
    
    int totalMatches = 0;

    if ( round == "First Round") { totalMatches = 160; } 
    else if (round == "Wild Card") { totalMatches = 6;}
    else if (round == "Group") { totalMatches = 4;}
    else if (round == "Conference") { totalMatches = 2;}
    else roundDone = false;
    
    auto matches = matchRepository->FindMatchesByTournamentAndRound(tournamentId, round);
    if (matches.size() != totalMatches) roundDone = false;
    else {
        for (const auto& m : matches) {
            if (!m->Score().has_value()) { 
                roundDone = false;
                break;
            }
        }
    }
    
    return roundDone;
}

inline std::expected<void, std::string> MatchDelegate::generateNextRound(const std::string& matchId, const std::string& tournamentId) {
    try {
        auto match = matchRepository->FindByIdAndTournamentId(matchId, tournamentId);

        if (!match) {
            return std::unexpected("Match not found.");
        }
        std::string prevRound = match->Round();
        if (prevRound == "Finals") return {};
        
        bool roundDone = checkPrevRound(tournamentId, prevRound);
        if (roundDone) {
            if (prevRound == "First Round") { createWildCardMatches(tournamentId); }
            else if (prevRound == "Wild Card") { createGroupMatches(tournamentId); }
            else if (prevRound == "Group") { createConferenceMatches(tournamentId); }
            else if (prevRound == "Conference") { createFinalMatch(tournamentId); }
        }
        
        return {};
    } catch (const std::exception& e) {
        return std::unexpected(e.what());
    }
}

inline void MatchDelegate::createWildCardMatches(const std::string& tournamentId) {
    auto playoffTeams = getPlayoffTeams(tournamentId);

    for (int c=0; c < 2; ++c) {
        int offset = 7 * c;
        for (int t=1; t < 4; ++t) {
            domain::Match newMatch;
            newMatch.TournamentId() = tournamentId;
            newMatch.Home() = playoffTeams[offset + t];
            newMatch.Visitor() = playoffTeams[offset + 7 - t];
            newMatch.Round() = "Wild Card";

            createMatch(newMatch);
        }
    }
}
inline void MatchDelegate::createGroupMatches(const std::string& tournamentId) {
    auto playoffTeams = getPlayoffTeams(tournamentId);
    auto wcMatches = matchRepository->FindMatchesByTournamentAndRound(tournamentId, "Wild Card");
    
    std::vector<int> c1winners;
    std::vector<int> c2winners;
    for (const auto& m : wcMatches) {
        auto winner = m->Score().value().GetWinner();
        domain::Team winningTeam = (winner == domain::Winner::HOME) ? m->Home() : m->Visitor();

        for (int i=0; i < 14; ++i) {
            if (winningTeam.Id == playoffTeams[i].Id) {
                if (i < 7) c1winners.emplace_back(i);
                else c2winners.emplace_back(i);
            }
        }
    }
    std::sort(c1winners.begin(), c1winners.end());
    std::sort(c2winners.begin(), c2winners.end());

    domain::Match match1, match2, match3, match4;

    match1.TournamentId() = tournamentId; match1.Home() = playoffTeams[0];
    match1.Visitor() = playoffTeams[c1winners[2]]; match1.Round() = "Group";
    createMatch(match1);

    match2.TournamentId() = tournamentId; match2.Home() = playoffTeams[c1winners[0]];
    match2.Visitor() = playoffTeams[c1winners[1]]; match2.Round() = "Group";
    createMatch(match2);

    match3.TournamentId() = tournamentId; match3.Home() = playoffTeams[7];
    match3.Visitor() = playoffTeams[c2winners[2]]; match3.Round() = "Group";
    createMatch(match3);

    match4.TournamentId() = tournamentId; match4.Home() = playoffTeams[c2winners[0]];
    match4.Visitor() = playoffTeams[c2winners[1]]; match4.Round() = "Group";
    createMatch(match4);
}
inline void MatchDelegate::createConferenceMatches(const std::string& tournamentId) {
    auto groupMatches = matchRepository->FindMatchesByTournamentAndRound(tournamentId, "Group");
    auto groups = groupRepository->FindByTournamentId(tournamentId);
    
    std::vector<domain::Team> c1winners;
    std::vector<domain::Team> c2winners;
    for (const auto& m : groupMatches) {
        auto winner = m->Score().value().GetWinner();
        domain::Team winningTeam = (winner == domain::Winner::HOME) ? m->Home() : m->Visitor();
        int conference = 2;
        
        for (int g=0; g < 4; ++g) {
            for (const auto& t : groups[g]->Teams()) {
                if (winningTeam.Id == t.Id) {
                    conference = 1;
                    break;
                }
            }
            if (conference == 1) break;
        }

        if (conference == 1) c1winners.push_back(winningTeam);
        else c2winners.push_back(winningTeam);
    }

    domain::Match match1, match2;

    match1.TournamentId() = tournamentId; match1.Home() = c1winners[0];
    match1.Visitor() = c1winners[1]; match1.Round() = "Conference";
    createMatch(match1);

    match2.TournamentId() = tournamentId; match2.Home() = c2winners[0];
    match2.Visitor() = c2winners[1]; match2.Round() = "Conference";
    createMatch(match2);
}
inline void MatchDelegate::createFinalMatch(const std::string& tournamentId){
    auto confMatches = matchRepository->FindMatchesByTournamentAndRound(tournamentId, "Conference");

    std::vector<domain::Team> finalists;
    for (const auto& m : confMatches) {
        auto winner = m->Score().value().GetWinner();
        domain::Team winningTeam = (winner == domain::Winner::HOME) ? m->Home() : m->Visitor();
        finalists.push_back(winningTeam);
    }

    domain::Match finalMatch;
    finalMatch.TournamentId() = tournamentId;
    finalMatch.Home() = finalists[0];
    finalMatch.Visitor() = finalists[1];
    finalMatch.Round() = "Finals";

    createMatch(finalMatch);
}

inline std::vector<domain::Team> MatchDelegate::getPlayoffTeams(const std::string& tournamentId) {
    std::vector<domain::Team> playoffTeams;
    auto groups = groupRepository->FindByTournamentId(tournamentId);

    for (int c=0; c < 2; ++c) {
        std::vector<TeamRecord> groupWinners;
        std::vector<TeamRecord> otherTeams;

        for (int g=0; g < 4; ++g) {
            std::vector<TeamRecord> groupTeams;

            for (int t=0; t < 4; ++t) {
                int teamWins = 0;
                float teamWP = 0;
                int netPoints = 0;
                auto team = groups[(c*4)+g]->Teams()[t];
                std::string teamId = team.Id;

                auto teamMatches = matchRepository->GetMatchesByTeamId(tournamentId, teamId);
                for (const auto& m : teamMatches) {
                    if (m->Round() != "First Round") continue;

                    auto winner = m->Score().value().GetWinner();
                    if (winner == domain::Winner::TIE) { teamWP += 0.5; }
                    else if ((winner == domain::Winner::HOME && m->Home().Id == teamId) ||
                             (winner == domain::Winner::VISITOR && m->Visitor().Id == teamId)) { 
                        ++teamWins; 
                    }

                    int netScore = m->Score().value().homeTeamScore - m->Score().value().visitorTeamScore;
                    if (m->Home().Id == teamId) { netPoints += netScore; }
                    else { netPoints -= netScore; }
                }

                teamWP = (teamWP + teamWins) / 10;
                groupTeams.emplace_back(team, teamWP, teamWins, netPoints);
            }

            groupTeams = sortTeams(groupTeams);
            groupWinners.push_back(groupTeams[0]);
            for (int i=1; i < 4; ++i) otherTeams.push_back(groupTeams[i]);
        }

        groupWinners = sortTeams(groupWinners);
        for (int i=0; i < 4; ++i) playoffTeams.push_back(std::get<0>(groupWinners[i]));
        otherTeams = sortTeams(otherTeams);
        for (int i=0; i < 3; ++i) playoffTeams.push_back(std::get<0>(otherTeams[i]));
    }

    return playoffTeams;
}

inline std::vector<TeamRecord> MatchDelegate::sortTeams(std::vector<TeamRecord> teams) {
    std::sort(teams.begin(), teams.end(), [](const TeamRecord& a, const TeamRecord& b) {
        if (std::get<1>(a) != std::get<1>(b)) { // First try to sort by win percentage
            return std::get<1>(a) > std::get<1>(b); 
        } else if (std::get<2>(a) != std::get<2>(b)) { // Second by number of wins
            return std::get<2>(a) > std::get<2>(b);
        } else if (sttd::get<3>(a) != std::get<3>(b)) { // Third by net points scored
            return std::get<3>(a) > std::get<3>(b);
        } else { // And then alphabetically by team name
            return std::get<0>(a).Name < std::get<0>(b).Name;
        }
    });

    return teams;
}

#endif //CONSUMER_MATCHDELEGATE_HPP