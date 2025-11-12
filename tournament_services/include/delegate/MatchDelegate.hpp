#ifndef A251C297_DF53_4BEB_93D6_DB45EAC8C825
#define A251C297_DF53_4BEB_93D6_DB45EAC8C825

#include "delegate/IMatchDelegate.hpp"
#include "persistence/repository/IRepository.hpp"
#include "domain/Tournament.hpp"
#include "persistence/repository/IMatchRepository.hpp"
#include "cms/IQueueMessageProducer.hpp"

class MatchDelegate : public IMatchDelegate {
    std::shared_ptr<IRepository<domain::Tournament, std::string>> tournamentRepo;
    std::shared_ptr<IMatchRepository> matchRepo;
    std::shared_ptr<IQueueMessageProducer> producer;

public:
    MatchDelegate(std::shared_ptr<IRepository<domain::Tournament, std::string>> tournamentRepo,
                  std::shared_ptr<IMatchRepository> matchRepo,
                  std::shared_ptr<IQueueMessageProducer> producer);

    std::expected<std::vector<domain::Match>, std::string> GetMatches(std::string_view tournamentId, std::string_view filter) override;
    std::expected<domain::Match, std::string> GetMatch(std::string_view tournamentId, std::string_view matchId) override;
    std::expected<void, std::string> UpdateMatchScore(std::string_view tournamentId, std::string_view matchId, const domain::Score& score) override;
    std::expected<std::string, std::string> CreateMatch(domain::Match& match) override;
    std::expected<std::vector<domain::Match>, std::string> GetMatchesByRound(std::string_view tournamentId, std::string_view round) override;
};


#endif /* A251C297_DF53_4BEB_93D6_DB45EAC8C825 */
