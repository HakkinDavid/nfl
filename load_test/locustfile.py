from typing import Any
from locust import HttpUser, task
import json
import uuid
import random
import time

class TournamentUser(HttpUser):

    def create_teams(self):
        teams_created = list()
        for i in range(32):
            team_data = {
                "name": f"Team {uuid.uuid4()}"
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
            "name": f"Tournament - {uuid.uuid4()}"
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
        group_data = {
            "name": f"Group - {uuid.uuid4()}"
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
                    json=team_data, # Ahora el JSON es {"id": "...", "name": "..."}
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
        # espera hasta que haya el número esperado de partidos pendientes
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
        for _ in range(8):
            group_id = self.create_group(tournament_id)
            if group_id:
                group_ids.append(group_id)
        if len(group_ids) < 8:
            return
        # asigna 4 equipos a cada grupo
        for i, group_id in enumerate(group_ids):
            teams_for_group = created_teams[i*4:(i+1)*4]
            self.assign_teams_to_group(tournament_id, group_id, teams_for_group)
        # simula las rondas
        rounds = [
            ("primera ronda", 160, False),
            ("tarjeta salvaje", 6, True), # yo cuando me salvajeo
            ("grupo", 4, True),
            ("conferencia", 2, True),
            ("finales", 1, True)
        ]
        for round_name, expected_matches, no_ties in rounds:
            self.simulate_round(tournament_id, round_name, expected_matches, no_ties)
        # al final trae todos los partidos y muestra los resultados
        with self.client.get(
                f"/tournaments/{tournament_id}/matches",
                catch_response=True,
                name=f"GET /tournaments/{tournament_id}/matches"
        ) as response:
            if response.status_code == 200:
                try:
                    matches = response.json()
                    print(f"partidos finales del torneo {tournament_id}: {json.dumps(matches, indent=2)}")
                except Exception as e:
                    response.failure(f"maurico no pude leer los partidos finales perdon: {str(e)}")
            else:
                response.failure(f"falló traer los partidos finales: {response.status_code}")
