#ifndef B37FEB69_6E3C_4DA6_BBCA_1BD46BF5F632
#define B37FEB69_6E3C_4DA6_BBCA_1BD46BF5F632

#include <string>
#include <memory>
#include <crow.h>

class IMatchDelegate;

class MatchController {
    std::shared_ptr<IMatchDelegate> delegate;

public:
    explicit MatchController(std::shared_ptr<IMatchDelegate> delegate);

    // GET /tournaments/{id}/matches
    [[nodiscard]] crow::response GetMatches(const crow::request& req, const std::string& tournamentId) const;

    // GET /tournaments/{id}/matches/{id}
    [[nodiscard]] crow::response GetMatch(const std::string& tournamentId, const std::string& matchId) const;

    // PATCH /tournaments/{id}/matches/{id}
    [[nodiscard]] crow::response UpdateMatchScore(const crow::request& req, const std::string& tournamentId, const std::string& matchId) const;
};

#endif /* B37FEB69_6E3C_4DA6_BBCA_1BD46BF5F632 */
