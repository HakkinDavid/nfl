//
// Created by root on 11/4/25.
//

#include "delegate/MatchDelegate.hpp"
#include "domain/Utilities.hpp"
#include <utility>
#include <format>
#include <string>

MatchDelegate::MatchDelegate(std::shared_ptr<IRepository<domain::Tournament, std::string>> tournamentRepo,
                             std::shared_ptr<IMatchRepository> matchRepo,
                             std::shared_ptr<IQueueMessageProducer> producer)
    : tournamentRepo(std::move(tournamentRepo)),
      matchRepo(std::move(matchRepo)),
      producer(std::move(producer)) {}

std::expected<std::string, std::string> MatchDelegate::CreateMatch(domain::Match& match) {
    try {
        std::string newId = matchRepo->Create(match);
        return newId;
    } catch (const std::exception& e) {
        return std::unexpected(e.what());
    }
}

std::expected<std::vector<domain::Match>, std::string> MatchDelegate::GetMatches(std::string_view tournamentId, std::string_view filter) {
    if (!tournamentRepo->ReadById(std::string(tournamentId))) {
        return std::unexpected("Tournament not found.");
    }

    auto allMatchesPtrs = matchRepo->FindAllByTournamentId(tournamentId);

    std::vector<domain::Match> allMatchesValues;
    allMatchesValues.reserve(allMatchesPtrs.size());

    for (const auto& matchPtr : allMatchesPtrs) {
        if (matchPtr) {
            allMatchesValues.push_back(*matchPtr);
        }
    }

    // Si no hay filtro, devolvemos el nuevo vector de valores.
    if (filter.empty() || filter == "all") {
        return allMatchesValues;
    }

    std::vector<domain::Match> filteredMatches;
    // Iteramos sobre el nuevo vector de valores para el filtrado.
    for (const auto& match : allMatchesValues) {
        bool isPlayed = match.Score().has_value();
        if (filter == "played" && isPlayed) {
            filteredMatches.push_back(match);
        } else if (filter == "pending" && !isPlayed) {
            filteredMatches.push_back(match);
        }
    }
    return filteredMatches;
}

std::expected<domain::Match, std::string> MatchDelegate::GetMatch(std::string_view tournamentId, std::string_view matchId) {
    auto match = matchRepo->FindByIdAndTournamentId(matchId, tournamentId);
    if (!match) {
        return std::unexpected("Match not found in this tournament.");
    }
    return *match;
}

std::expected<void, std::string> MatchDelegate::UpdateMatchScore(std::string_view tournamentId, std::string_view matchId, const domain::Score& score) {
    // Validación de marcador
    if (score.homeTeamScore < 0 || score.visitorTeamScore < 0) {
        return std::unexpected("Score cannot be negative.");
    }

    // Otra lógica de validación, como de empates, iría aquí

    try {
        matchRepo->UpdateScore(matchId, tournamentId, score);

        // Generar evento al registrar marcador
        std::string eventMessage = std::format("{{\"matchId\": \"{}\", \"tournamentId\": \"{}\"}}", matchId, tournamentId);
        producer->SendMessage(eventMessage, "match.score.updated");

        return {}; // Éxito
    } catch (const domain::NotFoundException& e) {
        return std::unexpected("Match not found.");
    } catch (const std::exception& e) {
        return std::unexpected(std::format("System error: {}", e.what()));
    }
}

std::expected<domain::Match, std::string> MatchDelegate::GetNextOpenMatch(std::string_view tournamentId) {
    auto match = matchRepo->FindLastOpenMatch(tournamentId);
    if (!match) {
        return std::unexpected("No open match found for this tournament.");
    }
    return *match;
}

std::expected<std::vector<domain::Match>, std::string> MatchDelegate::GetMatchesByRound(std::string_view tournamentId, std::string_view round) {
    if (!tournamentRepo->ReadById(std::string(tournamentId))) {
        return std::unexpected("Tournament not found.");
    }

    auto matchesPtrs = matchRepo->FindMatchesByTournamentAndRound(tournamentId, round);
    std::vector<domain::Match> matchesValues;
    matchesValues.reserve(matchesPtrs.size());
    for (const auto& ptr : matchesPtrs) {
        matchesValues.push_back(*ptr);
    }
    return matchesValues;
}