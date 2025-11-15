from typing import Any
from locust import HttpUser, task
import json
import random
import time

# ando bien nombrado
NFL_TEAMS = [
    "buffalo bills", "miami dolphins", "new england patriots", "new york jets",
    "baltimore ravens", "cincinnati bengals", "cleveland browns", "pittsburgh steelers",
    "houston texans", "indianapolis colts", "jacksonville jaguars", "tennessee titans",
    "denver broncos", "kansas city chiefs", "las vegas raiders", "los angeles chargers",
    "dallas cowboys", "new york giants", "philadelphia eagles", "washington commanders",
    "chicago bears", "detroit lions", "green bay packers", "minnesota vikings",
    "atlanta falcons", "carolina panthers", "new orleans saints", "tampa bay buccaneers",
    "arizona cardinals", "los angeles rams", "san francisco 49ers", "seattle seahawks"
]

NFL_GROUPS = [
    "afc este", "afc norte", "afc sur", "afc oeste",
    "nfc este", "nfc norte", "nfc sur", "nfc oeste"
]

NFL_TOURNAMENT_NAMES = [
    "nfl season simulation",
    "super bowl run",
    "offseason madness",
    "preseason chaos",
    "nfl experimental cup"
]

class TournamentUser(HttpUser):

    def create_teams(self):
        teams_created = list()
        for i in range(32):
            team_data = {
                "name": NFL_TEAMS[i]
            }
            with self.client.post(
                    "/teams",
                    json=team_data,
                    catch_response=True,
                    name="POST /teams"
            ) as response:
                if response.status_code == 200 or response.status_code == 201:
                    location = response.headers.get("Location") or response.headers.get("location")

                    teams_created.append({
                        "id": location,
                        "name": team_data["name"]
                    })
                else:
                    response.failure(f"falló crear el equipo: {response.status_code}")
        return teams_created

    def create_tournament(self):
        tournament_data = {
            "name": random.choice(NFL_TOURNAMENT_NAMES)
        }
        with self.client.post(
                "/tournaments",
                json=tournament_data,
                catch_response=True,
                name="POST /tournaments"
        ) as response:
            if response.status_code == 200 or response.status_code == 201:
                return response.headers.get("Location") or response.headers.get("location")
            else:
                response.failure(f"falló crear el torneo: {response.status_code}")
                return None

    def create_group(self, tournament_id: Any | None):
        group_ids = getattr(self, "_group_ids_for_group_creation", None)
        if group_ids is None:
            group_ids = []
        group_data = {
            "name": NFL_GROUPS[len(group_ids)]
        }
        with self.client.post(
                f"/tournaments/{tournament_id}/groups",
                json=group_data,
                catch_response=True,
                name=f"POST /tournaments/{tournament_id}/groups"
        ) as response:
            if response.status_code == 200 or response.status_code == 201:
                return response.headers.get("Location") or response.headers.get("location")
            else:
                response.failure(f"falló crear el grupo: {response.status_code}")
                return None

    def assign_teams_to_group(self, tournament_id: Any, group_id: Any, teams_list: list):
        for team in teams_list:
            team_data = {
                "id": team["id"],
                "name": team["name"]
            }

            with self.client.post(
                    f"/tournaments/{tournament_id}/groups/{group_id}/teams",
                    json=team_data, # NO ME DEJA ENTRAR AL melvincasa JSON
                    catch_response=True,
                    name=f"POST /tournaments/{tournament_id}/groups/{group_id}/teams"
            ) as response:
                if response.status_code not in [200, 201, 204]:
                    response.failure(f"falló asignar equipo {team['id']} al grupo: {response.status_code}")

    def fetch_pending_matches(self, tournament_id: Any):
        with self.client.get(
                f"/tournaments/{tournament_id}/matches?showMatches=pending",
                catch_response=True,
                name=f"GET /tournaments/{tournament_id}/matches?showMatches=pending"
        ) as response:
            if response.status_code == 200:
                try:
                    return response.json()
                except Exception as e:
                    response.failure(f"profe no pude leer los partidos: {str(e)}")
                    return []
            else:
                response.failure(f"falló traer los partidos: {response.status_code}")
                return []

    def patch_score(self, tournament_id: Any, match_id: Any, home_score: int, visitor_score: int):
        score_data = {"score": {"home": home_score, "visitor": visitor_score}}
        with self.client.patch(
                f"/tournaments/{tournament_id}/matches/{match_id}",
                json=score_data,
                catch_response=True,
                name=f"PATCH /tournaments/{tournament_id}/matches/{match_id}"
        ) as response:
            if response.status_code not in [200, 201, 204]:
                response.failure(f"falló actualizar el marcador: {response.status_code}")

    def simulate_round(self, tournament_id: Any, round_name: str, expected_matches: int, no_ties=False):
        matches = []
        # ps a ver si la api ya se digna a soltar los mugres partidos
        while True:
            matches = self.fetch_pending_matches(tournament_id)
            if len(matches) == expected_matches:
                break
            time.sleep(1)
        # actualiza los marcadores de todos los partidos
        for match in matches:
            home_score = random.randint(0, 10)
            visitor_score = random.randint(0, 10)
            if no_ties:
                while visitor_score == home_score:
                    visitor_score = random.randint(0, 10)
            self.patch_score(tournament_id, match.get("id"), home_score, visitor_score)

    @task
    def full_tournament_flow(self):
        tournament_id = self.create_tournament()
        if not tournament_id:
            return
        created_teams = self.create_teams()
        if len(created_teams) < 32:
            return
        group_ids = []
        self._group_ids_for_group_creation = group_ids
        for _ in range(8):
            group_id = self.create_group(tournament_id)
            if group_id:
                group_ids.append(group_id)
        del self._group_ids_for_group_creation
        if len(group_ids) < 8:
            return
        # reparto 4 equipos por piocha
        for i, group_id in enumerate(group_ids):
            teams_for_group = created_teams[i*4:(i+1)*4]
            self.assign_teams_to_group(tournament_id, group_id, teams_for_group)
        rounds = [
            ("primera ronda", 160, False),
            ("tarjeta salvaje", 6, True), # yo cuando me salvajeo
            ("grupo", 4, True),
            ("conferencia", 2, True),
            ("finales", 1, True)
        ]
        for round_name, expected_matches, no_ties in rounds:
            self.simulate_round(tournament_id, round_name, expected_matches, no_ties)
        with self.client.get(
                f"/tournaments/{tournament_id}/matches",
                catch_response=True,
                name=f"GET /tournaments/{tournament_id}/matches"
        ) as response:
            if response.status_code == 200:
                try:
                    matches = response.json()
                    print(f"partidos finales del torneo {tournament_id}: {json.dumps(matches, indent=2)}")
                    print("sería todo :)")
                    return
                except Exception as e:
                    response.failure(f"maurico no pude leer los partidos finales perdon: {str(e)}")
            else:
                response.failure(f"falló traer los partidos finales: {response.status_code}")
