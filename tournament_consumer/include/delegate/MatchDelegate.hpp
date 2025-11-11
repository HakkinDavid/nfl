//
// Created by Meeeeee on 11/10/25.
//

#ifndef CONSUMER_MATCHDELEGATE_HPP
#define CONSUMER_MATCHDELEGATE_HPP

#include <expected>
#include <memory>

#include "persistence/repository/IMatchRepository.hpp"
#include "persistence/repository/IGroupRepository.hpp"

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
};

inline void MatchDelegate::createMatch(const domain::Match& match) {
    std::string tournamentId = match.TournamentId();
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

    if ( round == "First Round") {
        totalMatches = 160;
    } else if (round == "Wild Card") { totalMatches = 6;}
    else if (round == "Group") { totalMatches = 4;}
    else if (round == "Conference") { totalMatches = 2;}
    else roundDone = false;
    
    auto matches = matchRepository->FindMatchesByTournamentAndRound(tournamentId, round);
    if (matches.size() != totalMatches) roundDone = false;
    else {
        for (auto &m : matches) {
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
        
        std::string prevRound = match->Round();
        if (prevRound == "Finals") return {};
        
        bool roundDone = checkPrevRound(tournamentId, prevRound);
        if (roundDone) {
            if (prevRound == "First Round") {createWildCardMatches(tournamentId);}
            else if (prevRound == "Wild Card") {createGroupMatches(tournamentId);}
            else if (prevRound == "Group") {createConferenceMatches(tournamentId);}
            else if (prevRound == "Conference") {createFinalMatch(tournamentId);}
        }
        
        return {};
    } catch (const std::exception& e) {
        return std::unexpected(e.what());
    }
}

inline void createWildCardMatches(const std::string& tournamentId) {

}
inline void createGroupMatches(const std::string& tournamentId) {

}
inline void createConferenceMatches(const std::string& tournamentId) {

}
inline void createFinalMatch(const std::string& tournamentId){

}

#endif //CONSUMER_MATCHDELEGATE_HPP