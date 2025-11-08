#ifndef LISTENER_MATCHCREATED_LISTENER_HPP
#define LISTENER_MATCHCREATED_LISTENER_HPP

#include "QueueMessageListener.hpp"
#include <nlohmann/json.hpp>
#include <random>
#include <cpprest/http_client.h>
#include "../configuration/ContainerSetup.hpp"

class MatchCreatedListener : public QueueMessageListener {
    void processMessage(const std::string& message) override;
public:
    MatchCreatedListener(const std::shared_ptr<ConnectionManager>& conn)
        : QueueMessageListener(conn) {}
    ~MatchCreatedListener() override { Stop(); }
};

inline void MatchCreatedListener::processMessage(const std::string& message) {
    try {
        auto j = nlohmann::json::parse(message);
        std::string tournamentId = j.at("tournament_id");
        std::string matchId = j.at("match_id");

        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dist(0, 10);
        int score1 = dist(gen);
        int score2 = dist(gen);

        nlohmann::json body = {
            {"score", {{"home", score1}, {"visitor", score2}}}
        };

        web::http::client::http_client client(U(config::configuration["endpoint"]["url"]));
        web::http::uri_builder builder;
        builder.append_path("tournaments");
        builder.append_path(tournamentId);
        builder.append_path("matches");
        builder.append_path(matchId);

        web::http::http_request req(web::http::methods::PATCH);
        req.headers().add("Content-Type", "application/json");
        req.set_body(body.dump());

        auto response = client.request(req).get();
        std::println("PATCH to {} returned {}", builder.to_string(), response.status_code());

    } catch (const std::exception& e) {
        std::println("MatchCreatedListener error: {}", e.what());
    }
}

#endif // LISTENER_MATCHCREATED_LISTENER_HPP
