//
// Created by tomas on 8/31/25.
//
#include <string_view>
#include <memory>
#include <expected>

#include "delegate/TournamentDelegate.hpp"
#include "domain/Utilities.hpp"
#include "persistence/repository/IRepository.hpp"

TournamentDelegate::TournamentDelegate(std::shared_ptr<IRepository<domain::Tournament, std::string>> repository, std::shared_ptr<QueueMessageProducer> producer) : tournamentRepository(std::move(repository)), producer(std::move(producer))
{
}

std::expected<std::string, std::string> TournamentDelegate::CreateTournament(std::shared_ptr<domain::Tournament> tournament)
{
    // fill groups according to max groups
    std::shared_ptr<domain::Tournament> tp = std::move(tournament);
    // for (auto[i, g] = std::tuple{0, 'A'}; i < tp->Format().NumberOfGroups(); i++,g++) {
    //     tp->Groups().push_back(domain::Group{std::format("Tournament {}", g)});
    // }

    std::string id = tournamentRepository->Create(*tp);
    producer->SendMessage(id, "tournament.created");

    // if groups are completed also create matches

    return id;
}

std::expected<std::string, std::string> TournamentDelegate::UpdateTournament(std::shared_ptr<domain::Tournament> tournament)
{
    std::shared_ptr<domain::Tournament> tp = std::move(tournament);
    std::string id = tournamentRepository->Update(*tp);
    producer->SendMessage(id, "tournament.updated");
    return id;
}

std::shared_ptr<domain::Tournament> TournamentDelegate::GetTournament(std::string_view id)
{
    return tournamentRepository->ReadById(id.data());
}

std::expected<void, std::string> TournamentDelegate::DeleteTournament(const std::string &tournamentId)
{
    try
    {
        tournamentRepository->Delete(tournamentId);
        producer->SendMessage(tournamentId, "tournament.deleted");
        return {};
    }
    catch (const domain::NotFoundException &e)
    {
        return std::unexpected(e.what());
    }
}

std::vector<std::shared_ptr<domain::Tournament>> TournamentDelegate::ReadAll()
{
    return tournamentRepository->ReadAll();
}