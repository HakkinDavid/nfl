//
// Created by tomas on 8/31/25.
//
#include <string_view>
#include <memory>
#include <expected>

#include "delegate/TournamentDelegate.hpp"
#include "domain/Utilities.hpp"
#include "persistence/repository/IRepository.hpp"

TournamentDelegate::TournamentDelegate(std::shared_ptr<IRepository<domain::Tournament, std::string>> repository, std::shared_ptr<IQueueMessageProducer> producer) : tournamentRepository(std::move(repository)), producer(std::move(producer))
{
}

std::expected<std::string, std::string> TournamentDelegate::CreateTournament(std::shared_ptr<domain::Tournament> tournament)
{
    try {
        std::shared_ptr<domain::Tournament> tp = std::move(tournament);
        std::string id = tournamentRepository->Create(*tp);
        producer->SendMessage(id, "tournament.created");
        return id;
    } catch (const domain::DuplicateEntryException& e) {
        return std::unexpected(e.what());
    }
}

std::expected<std::string, std::string> TournamentDelegate::UpdateTournament(std::shared_ptr<domain::Tournament> tournament)
{
    try {
        std::shared_ptr<domain::Tournament> tp = std::move(tournament);
        std::string id = tournamentRepository->Update(*tp);
        producer->SendMessage(id, "tournament.updated");
        return id;
    } catch (const domain::NotFoundException& e) {
        return std::unexpected(e.what());
    } catch (const domain::DuplicateEntryException& e) {
        return std::unexpected(e.what());
    }
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
        // Comentado por ahora para ver si laggeaba el API call
        //producer->SendMessage(tournamentId, "tournament.deleted");
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