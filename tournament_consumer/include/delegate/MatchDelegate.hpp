//
// Created by developer on 10/13/25.
//

#ifndef CONSUMER_MATCHDELEGATE_HPP
#define CONSUMER_MATCHDELEGATE_HPP

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
    void CreateMatch(const domain::Match& match);
    void CreateFirstRoundMatches(const std::string& tournamentId);
};

inline void MatchDelegate::CreateMatch(const domain::Match& match) {
    // idk just call the dang repository and tell it to do the thingy
    // and i guess send the match.created message here
    
    std::string tournamentId = match.TournamentId();
    std::string matchId = matchRepo->Create(match);
    
    //std::string eventMessage = std::format("{{\"matchId\": \"{}\", \"tournamentId\": \"{}\"}}", matchId, tournamentId);
    //producer->SendMessage(eventMessage, "match.created");
}

inline void MatchDelegate::CreateFirstRoundMatches(const std::string& tournamentId) {
    auto groups = groupRepository->FindByTournamentId(tournamentId);

    for (int g1=0; g1 < 7; ++g1) {
        for (int g2=g1+1; g2 < 8; ++g2) {
            for (int t=0; t < 4; ++t) {
                domain::Match newMatch;
                newMatch.TournamentId() = tournamentId;
                newMatch.Home() = groups[g1]->Teams()[t];
                newMatch.Visitor() = groups[g2]->Teams()[t];
                newMatch.Round() = "First Round";

                CreateMatch(newMatch);
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

                CreateMatch(newMatch);
            }
        }
    }
}

#endif //CONSUMER_MATCHDELEGATE_HPP