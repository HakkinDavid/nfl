/*  
    NFL-style Tournament Logic Mockup
    Author: Mauricio Alcántar
    Last edited: 24-9-2025 
*/

/*
    The following implementation is only meant to convey the general structure of the final code,
    as well as the kinds of procedures that must be performed to actually pair up the teams correctly.

    There are many comments scattered throughout. Some explain what the code is doing, but the majority
    explain many grievances, doubts, and hopes for the shape of the final implementation.
*/

#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <memory>
#include <random>
#include <string>
#include <vector>

// Very simple function to generate a random integer within the range [low, high]
// Uses a uniform distribution
int uniformRNG(int low, int high) {
    std::random_device rd;
    std::default_random_engine gen(rd());
    std::uniform_int_distribution<int> dist(low, high);
    return dist(gen);
}

// Team class definition
class Team {
    int id;
    std::string name;
    std::string conference;
    int netPoints;

    // This vector is only here to help me alleviate some coding headache
    // Maybe it makes its way to the final code, but I'd imagine we would instead 
    // get these values from database queries and such
    std::array<int, 3> win_loss_tie;

public:
    Team(int id, std::string name, std::string conference) : id(id), name(name), conference(conference) {
        netPoints = 0;
        win_loss_tie = {0, 0, 0};
    }

    int Id() const { return id; }
    std::string Name() const { return name; }
    std::string Conference() const { return conference; }
    int &NetPoints() { return netPoints; }
    std::array<int, 3> &Win_loss_tie() { return win_loss_tie; }

    float calculateWinPercentage() {
        int totalGames = win_loss_tie[0] + win_loss_tie[1] + win_loss_tie[2];
        if(totalGames == 0) { 
            std::cerr << "Girl we have no games" << std::endl;
            return -1;
        }

        float winP = (win_loss_tie[0] + (0.5F * win_loss_tie[2])) / totalGames;
        return winP;
    }
};

// I'm conflicted as to whether a "game" should be an actual object class with instances
// that exist in code, or if it should just be represented as a row in the database
// (leaning towards the latter)
// In this code itself I'm really only using it to define the "play game" functions
class Game {
    std::shared_ptr<Team> homeTeam;
    std::shared_ptr<Team> awayTeam;
    int homeScore;
    int awayScore;
    // I believe an additional attribute, called "gameType" or something, could be good
    // This way we could discriminate games by whether they were played during the regular season,
    // wild card round, divisional round, conference final, or BIG BOWL

public:
    std::shared_ptr<Team> &HomeTeam() { return homeTeam; }
    std::shared_ptr<Team> &AwayTeam() { return awayTeam; }
    int HomeScore() const { return homeScore; }
    int AwayScore() const { return awayScore; }

    void playRegularGame() {
        homeScore = uniformRNG(0, 10);
        awayScore = uniformRNG(0, 10);
        homeTeam->NetPoints() += homeScore - awayScore;
        awayTeam->NetPoints() += awayScore - homeScore;

        if(homeScore > awayScore) {
            homeTeam->Win_loss_tie()[0]++;
            awayTeam->Win_loss_tie()[1]++;
        } else if(homeScore < awayScore) {
            homeTeam->Win_loss_tie()[1]++;
            awayTeam->Win_loss_tie()[0]++;
        } else {
            homeTeam->Win_loss_tie()[2]++;
            awayTeam->Win_loss_tie()[2]++;
        }
    }

    // A playoff game is not allowed to end in a tie
    int playPlayoffGame() {
        homeScore = uniformRNG(0, 10);
        awayScore = uniformRNG(0, 10);
        while(homeScore == awayScore) awayScore = uniformRNG(0, 10);
        // This isn't strictly necessary
        homeTeam->NetPoints() += homeScore - awayScore;
        awayTeam->NetPoints() += awayScore - homeScore;

        return (homeScore > awayScore) ? homeTeam->Id() : awayTeam->Id();
    }
};

// Division Class definition
// Teacher calls these "Groups" instead
class Division {
    int id;
    std::string name;
    std::array<std::shared_ptr<Team>, 4> divisionTeams;

public:
    Division(int id, std::string name) : id(id), name(name) { }

    int Id() const { return id; }
    std::string Name() const { return name; }
    std::array<std::shared_ptr<Team>, 4> &DivisionTeams() { return divisionTeams; }

    void playDivisionalGames() {
        for(int i=0; i < 3; ++i) {
            for(int j=i+1; j < 4; ++j) {
                Game newGame;
                newGame.HomeTeam() = divisionTeams[i];
                newGame.AwayTeam() = divisionTeams[j];
                newGame.playRegularGame();
            }
        }
    }
};

// Tournament class definition
// This implementation is the 32-team version that mirrors how the NFL is structured
// This is, it has 2 conferences with 4 divisions each and 4 teams in a division
class Tournament {
    std::array<std::string, 2> conferences;

    // These arrays should actually be some sort of Map object each
    // In the code, it's common for the team's ID to be "requested" and then used as an index for the array
    // But that relies on the teams to be ID'd as if they were indexes, which they won't (they'll be UUIDs)
    // Same goes for the divisions, albeit less frequently
    std::array<std::shared_ptr<Division>, 8> divisions;
    std::array<std::shared_ptr<Team>, 32> teams;

    // Some of the next attributes may or may not be necessary. Not sure yet
    std::array<int, 7> playoffTeamsA;
    std::array<int, 7> playoffTeamsB;
    int championID;

public:
    Tournament(std::array<std::string, 2> conferences) : conferences(conferences) { }

    std::array<std::shared_ptr<Team>, 32> &Teams() { return teams; }
    std::array<std::shared_ptr<Division>, 8> &Divisions() { return divisions; }

    // This function won't exist in the final code
    // Every time a division is filled with its respective 4 teams, the first line is run for that division
    // After that, the rest of the function is run
    void regularSeason() {
        for(auto &d : divisions) d->playDivisionalGames();

        for(int p=0; p < 4; ++p) {
            for(int i=0; i < 7; ++i) {
                for(int j=i+1; j < 8; ++j) {
                    Game newGame;
                    // We could also use the divisions array to find the teams
                    // Would be more legible, but simple arithmetic is probably faster than accessing shared_ptrs
                    newGame.HomeTeam() = teams[(i*4)+p];
                    newGame.AwayTeam() = teams[(j*4)+p];
                    newGame.playRegularGame();
                }
            }
        }
    }

    void decidePlayoffBracket(int conference) {
        std::array<int, 4> divChamps;
        for(int i=0; i < 4; ++i) divChamps[i] = divisionalRanking((4*conference)+i);

        auto orderedIds = fourTeamShuffle(divChamps[0], divChamps[1], divChamps[2], divChamps[3]);
        for(int i=0; i < 4; ++i) {
            if(conference == 0) playoffTeamsA[i] = orderedIds[i];
            else playoffTeamsB[i] = orderedIds[i];
        }

        std::vector<std::shared_ptr<Team>> otherTeams;
        for(int i=0; i < 16; ++i) {
            int j = (16*conference) + i;
            if(j == divChamps[0] || j == divChamps[1] || j == divChamps[2] || j == divChamps[3]) continue;
            otherTeams.push_back(teams[j]);
        }

        std::sort(otherTeams.begin(), otherTeams.end(), [](const std::shared_ptr<Team>& a, const std::shared_ptr<Team>& b) {
            return a->calculateWinPercentage() > b->calculateWinPercentage();
        });
        auto wildcardIds = fourTeamShuffle(otherTeams[0]->Id(), otherTeams[1]->Id(), 
                                          otherTeams[2]->Id(), otherTeams[3]->Id());
        std::array<float, 4> wps;
        for(int i=0; i < 4; ++i) wps[i] = teams[wildcardIds[i]]->calculateWinPercentage();

        if(wps[2] == wps[3]) {
            for(int i=4; i < 12; ++i) {
                float wpi = otherTeams[i]->calculateWinPercentage();
                if(wps[0] == wpi) {
                    int win = breakTie(wildcardIds[0], i, 0);
                    if(win == i) {
                        wildcardIds[2] = wildcardIds[1]; wildcardIds[1] = wildcardIds[0]; wildcardIds[0] = i;
                        continue;
                    }
                }
                if(wps[1] == wpi) {
                    int win = breakTie(wildcardIds[1], i, 0);
                    if(win == i) {
                        wildcardIds[2] = wildcardIds[1]; wildcardIds[1] = i;
                        continue;
                    }
                }
                if(wps[2] == wpi) {
                    int win = breakTie(wildcardIds[2], i, 0);
                    if(win == i) wildcardIds[2] = i;
                } else { break; }
            }
        }

        for(int i=0; i < 3; ++i) {
            if(conference == 0) playoffTeamsA[i+4] = wildcardIds[i];
            else playoffTeamsB[i+4] = wildcardIds[i];
        }
    }

    int conferencePlayoffs(int conference) {
        int lowest = -1;
        std::vector<int> middle;
        auto playoffTeams = (conference == 0) ? playoffTeamsA : playoffTeamsB;

        // Wild card round
        // All games here would have the wild card identifier
        for(int i=0; i < 3; ++i) {
            Game newGame;
            newGame.HomeTeam() = teams[playoffTeams[3-i]];
            newGame.AwayTeam() = teams[playoffTeams[4+i]];
            int winner = newGame.playPlayoffGame();

            if(i == 0) lowest = winner;
            else {
                if(winner == playoffTeams[4+i]) {
                    middle.emplace_back(lowest);
                    lowest = winner;
                } else middle.emplace_back(winner);
            }

            // debug printing baby
            std::cout << "WILD CARD GAME: " << newGame.HomeTeam()->Name() << " " << newGame.HomeScore() <<
                         " - " << newGame.AwayScore() << " " << newGame.AwayTeam()->Name() << std::endl;
        }

        // Divisional round
        // Both games here would have the divisional identifier
        Game div1;
        div1.HomeTeam() = teams[playoffTeams[0]];
        div1.AwayTeam() = teams[lowest];
        int winner1 = div1.playPlayoffGame();
        Game div2;
        div2.HomeTeam() = teams[middle[0]];
        div2.AwayTeam() = teams[middle[1]];
        int winner2 = div2.playPlayoffGame();
        // debug printing baby
        std::cout << "DIVISIONAL GAME: " << div1.HomeTeam()->Name() << " " << div1.HomeScore() <<
                     " - " << div1.AwayScore() << " " << div1.AwayTeam()->Name() << std::endl;
        std::cout << "DIVISIONAL GAME: " << div2.HomeTeam()->Name() << " " << div2.HomeScore() <<
                     " - " << div2.AwayScore() << " " << div2.AwayTeam()->Name() << std::endl;

        // Conference finals
        // This would have the conference final identifier
        Game conf;
        conf.HomeTeam() = teams[winner1];
        conf.AwayTeam() = teams[winner2];
        int conferenceChamp = conf.playPlayoffGame();
        // debug printing baby
        std::cout << "CONFERENCE FINAL (" << conferences[conference] << "): " << conf.HomeTeam()->Name() << " " << 
                     conf.HomeScore() << " - " << conf.AwayScore() << " " << conf.AwayTeam()->Name() << std::endl;

        return conferenceChamp;
    }

    void playoffs() {
        int champ1 = conferencePlayoffs(0);
        int champ2 = conferencePlayoffs(1);

        // This would have the BIG BOWL identifier or whatever
        Game bigBowl;
        bigBowl.HomeTeam() = teams[champ1];
        bigBowl.AwayTeam() = teams[champ2];
        championID = bigBowl.playPlayoffGame();
        // debug printing baby
        std::cout << "THE BIG BOWL: " << bigBowl.HomeTeam()->Name() << " " << bigBowl.HomeScore() <<
                     " - " << bigBowl.AwayScore() << " " << bigBowl.AwayTeam()->Name() << std::endl;
    }

    std::array<int, 4> fourTeamShuffle(int idA, int idB, int idC = -1, int idD = -1) {
        std::array<int, 4> orderedIds{-1, -1, -1, -1};

        if(idC == -1) {
            orderedIds[0] = breakTie(idA, idB, 0);
            if(orderedIds[0] == idA) orderedIds[1] = idB;
            else orderedIds[1] = idA;
        } 
        else if(idD == -1) {
            std::array<int, 4> subOrder;
            orderedIds[0] = breakTie3(idA, idB, idC, 0);
            if(orderedIds[0] == idA) subOrder = fourTeamShuffle(idB, idC);
            else if(orderedIds[0] == idB) subOrder = fourTeamShuffle(idA, idC);
            else subOrder = fourTeamShuffle(idA, idB);
            orderedIds[1] = subOrder[0]; orderedIds[2] = subOrder[1];
        } 
        else {
            std::array<std::shared_ptr<Team>, 4> shuffle{teams[idA], teams[idB], teams[idC], teams[idD]};
            std::sort(shuffle.begin(), shuffle.end(), [](const std::shared_ptr<Team>& a, const std::shared_ptr<Team>& b) {
                return a->calculateWinPercentage() > b->calculateWinPercentage();
            });
            std::array<float, 4> wps;
            std::array<int, 4> ids;
            for(int i=0; i < 4; ++i) { 
                wps[i] = shuffle[i]->calculateWinPercentage();
                ids[i] = shuffle[i]->Id();
            }

            std::array<int, 4> subOrder;
            bool third = false;
            if(wps[0] == wps[3]) {
                orderedIds[0] = breakTie4(idA, idB, idC, idD);
                if(orderedIds[0] == idA) subOrder = fourTeamShuffle(idB, idC, idD);
                else if(orderedIds[0] == idB) subOrder = fourTeamShuffle(idA, idC, idD);
                else if(orderedIds[0] == idC) subOrder = fourTeamShuffle(idA, idB, idD);
                else subOrder = fourTeamShuffle(idA, idB, idC);
                orderedIds[1] = subOrder[0]; orderedIds[2] = subOrder[1]; orderedIds[3] = subOrder[2];
            } else if(wps[0] == wps[2]) {
                orderedIds = fourTeamShuffle(ids[0], ids[1], ids[2]);
                orderedIds[3] = ids[3];
            } else if(wps[0] == wps[1]) {
                orderedIds = fourTeamShuffle(ids[0], ids[1]);
                third = true;
            } else {
                orderedIds[0] = ids[0];
                if(wps[1] == wps[3]) {
                    subOrder = fourTeamShuffle(ids[1], ids[2], ids[3]);
                    orderedIds[1] = subOrder[0]; orderedIds[2] = subOrder[1]; orderedIds[3] = subOrder[2];
                } else if(wps[1] == wps[2]) {
                    subOrder = fourTeamShuffle(ids[1], ids[2]);
                    orderedIds[1] = subOrder[0]; orderedIds[2] = subOrder[1];
                    orderedIds[3] = ids[3];
                } else {
                    orderedIds[1] = ids[1];
                    third = true;
                }
            }
            if(third) {
                if(wps[2] == wps[3]) {
                    subOrder = fourTeamShuffle(ids[2], ids[3]);
                    orderedIds[2] = subOrder[0]; orderedIds[3] = subOrder[1];
                } else { orderedIds[2] = ids[2]; orderedIds[3] = ids[3]; }
            }
        }

        return orderedIds;
    }

    // Very much redundant function that could be substituted by a fourTeamShuffle but I'm tireeedddd
    int divisionalRanking(int div) {
        auto divTeams = divisions[div]->DivisionTeams();

        std::sort(divTeams.begin(), divTeams.end(), [](const std::shared_ptr<Team>& a, const std::shared_ptr<Team>& b) {
            return a->calculateWinPercentage() > b->calculateWinPercentage();
        });
        
        float wp = divTeams[0]->calculateWinPercentage();
        if(wp != divTeams[1]->calculateWinPercentage()) return divTeams[0]->Id();
        if(wp != divTeams[2]->calculateWinPercentage())
            return breakTie(divTeams[0]->Id(), divTeams[1]->Id(), 0);
        if(wp != divTeams[3]->calculateWinPercentage())
            return breakTie3(divTeams[0]->Id(), divTeams[1]->Id(), divTeams[2]->Id(), 0);
        else return breakTie4(divTeams[0]->Id(), divTeams[1]->Id(), divTeams[2]->Id(), divTeams[3]->Id());
    }

    int breakTie(int idA, int idB, bool skipWins) {
        // Step 1. Check amount of wins
        if(!skipWins) {
            int winsA = teams[idA]->Win_loss_tie()[0];
            int winsB = teams[idB]->Win_loss_tie()[0];
            if(winsA > winsB) return idA;
            else if(winsB > winsA) return idB;
        }
        // Step 2. Check net points
        int pointsA = teams[idA]->NetPoints();
        int pointsB = teams[idB]->NetPoints();
        if(pointsA > pointsB) return idA;
        else if(pointsB > pointsA) return idB;
        // Step 3. Give up
        else return (idA < idB) ? idA : idB;
    }
    int breakTie3(int idA, int idB, int idC, bool skipWins) {
        if(!skipWins) {
            std::vector<std::vector<int>> teamWins{{idA, 0}, {idB, 0}, {idC, 0}};
            for(int i=0; i < 3; ++i) teamWins[i][1] = teams[teamWins[i][0]]->Win_loss_tie()[0];

            std::sort(teamWins.begin(), teamWins.end(), [](const std::vector<int>& a, const std::vector<int>& b) {
                return a[1] > b[1];
            });
            if(teamWins[0][1] != teamWins[2][1]) {
                if(teamWins[0][1] == teamWins[1][1]) 
                    return breakTie(teamWins[0][0], teamWins[1][0], 1);
                else return teamWins[0][0];
            }
        }
        std::vector<std::vector<int>> teamPts{{idA, 0}, {idB, 0}, {idC, 0}};
        for(int i=0; i < 3; ++i) teamPts[i][1] = teams[teamPts[i][0]]->NetPoints();

        std::sort(teamPts.begin(), teamPts.end(), [](const std::vector<int>& a, const std::vector<int>& b) {
            return a[1] > b[1];
        });
        if(teamPts[0][1] != teamPts[2][1]) {
            if(teamPts[0][1] == teamPts[1][1]) 
                return (teamPts[0][0] < teamPts[1][0]) ? teamPts[0][0] : teamPts[1][0];
            else return teamPts[0][0];
        } 
        
        if(idA < idB && idA < idC) return idA;
        else return (idB < idC) ? idB : idC;
    }
    int breakTie4(int idA, int idB, int idC, int idD) {
        std::vector<std::vector<int>> teamWins{{idA, 0}, {idB, 0}, {idC, 0}, {idD, 0}};
        for(int i=0; i < 4; ++i) teamWins[i][1] = teams[teamWins[i][0]]->Win_loss_tie()[0];

        std::sort(teamWins.begin(), teamWins.end(), [](const std::vector<int>& a, const std::vector<int>& b) {
            return a[1] > b[1];
        });
        if(teamWins[0][1] != teamWins[3][1]) {
            if(teamWins[0][1] == teamWins[2][1])
                return breakTie3(teamWins[0][0], teamWins[1][0], teamWins[2][0], 1);
            else if(teamWins[0][1] == teamWins[1][1]) 
                return breakTie(teamWins[0][0], teamWins[1][0], 1);
            else return teamWins[0][0];
        }

        std::vector<std::vector<int>> teamPts{{idA, 0}, {idB, 0}, {idC, 0}, {idD, 0}};
        for(int i=0; i < 4; ++i) teamPts[i][1] = teams[teamPts[i][0]]->NetPoints();

        std::sort(teamPts.begin(), teamPts.end(), [](const std::vector<int>& a, const std::vector<int>& b) {
            return a[1] > b[1];
        });
        if(teamPts[0][1] != teamPts[3][1]) {
            if(teamPts[0][1] == teamPts[2][1]) 
                if(teamPts[0][0] < teamPts[1][0] && teamPts[0][0] < teamPts[2][0]) return teamPts[0][0];
                else return (teamPts[1][0] < teamPts[2][0]) ? teamPts[1][0] : teamPts[2][0];
            else if(teamPts[0][1] == teamPts[1][1]) 
                return (teamPts[0][0] < teamPts[1][0]) ? teamPts[0][0] : teamPts[1][0];
            else return teamPts[0][0];
        } 
        
        if(idA < idB && idA < idC && idA < idD) return idA;
        else if(idB < idC && idB < idD) return idB;
        else return (idC < idD) ? idC : idD;
    }

    // Debug print functions so we can SEE what is happening
    void printWinPercentages() {
        int divNum = 0;
        std::cout << conferences[0] << std::endl;
        for(auto d : divisions) {
            if(divNum == 4) std::cout << conferences[1] << std::endl;
            ++divNum;
            std::cout << d->Name() << std::endl;
            for(auto t : d->DivisionTeams()) {
                auto wlt = t->Win_loss_tie();
                std::cout << t->Name() << "   ";
                std::cout << wlt[0] << "-" << wlt[1] << "-" << wlt[2];
                std::cout << "   " << t->calculateWinPercentage();
                std::cout << "\t" << t->NetPoints() << std::endl;
            }
        }
    }
    void printPlayoffBracket() {
        std::cout << conferences[0] << std::endl;
        for(int i=0; i < 7; ++i) {
            std::cout << i+1 << " | " << teams[playoffTeamsA[i]]->Name() << std::endl;
        }
        std::cout << conferences[1] << std::endl;
        for(int i=0; i < 7; ++i) {
            std::cout << i+1 << " | " << teams[playoffTeamsB[i]]->Name() << std::endl;
        }
    }
};

int main() {
    std::array<std::string, 2> conferences = {"SuperFL", "DuperFL"};
    Tournament tournament(conferences);
    std::array<std::shared_ptr<Division>, 8> divisions;
    std::array<std::shared_ptr<Team>, 32> teams;

    // Creation of teams
    for(int i=0; i < 32; ++i) {
        std::string name = "Team xx";
        name[5] = char(65+(i/4));
        name[6] = char(49+(i%4));
        std::string conf = (i < 16) ? conferences[0] : conferences[1];
        teams[i] = std::make_shared<Team>(i, name, conf);
    }
    tournament.Teams() = teams;

    // Creation of divisions
    for(int i=0; i < 8; ++i) {
        std::string name = "Division x";
        name[9] = char(65+i);
        divisions[i] = std::make_shared<Division>(i, name);
        std::array<std::shared_ptr<Team>, 4> divTeams;
        for(int j=0; j < 4; ++j) divTeams[j] = teams[(i*4)+j];
        // std::copy(teams.begin()+(i*4), teams.end()+((i+1)*4), divTeams.begin());
        divisions[i]->DivisionTeams() = divTeams;
    }
    tournament.Divisions() = divisions;

    std::cout << "PLaying regular season..." << std::endl;
    tournament.regularSeason();
    std::cout << "Results:\n" << std::endl;
    tournament.printWinPercentages();
    std::cout << std::endl;

    tournament.decidePlayoffBracket(0);
    tournament.decidePlayoffBracket(1);
    std::cout << "Playoff brackets:" << std::endl;
    tournament.printPlayoffBracket();
    std::cout << std::endl;

    std::cout << "PLAYOFF TIME" << std::endl;
    tournament.playoffs();
    return 0;
}