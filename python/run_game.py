import argparse
import curses
import json
from datetime import datetime, timezone
from pathlib import Path

from treasure_runner.models.game_engine import GameEngine
from treasure_runner.ui.game_ui import GameUI

#reads command line arguments
def parse_args():
    parser = argparse.ArgumentParser(description="Run Treasure Runner")
    parser.add_argument("--config", required=True)
    parser.add_argument("--profile", required=True)
    return parser.parse_args()

#player profiles
def load_or_create_profile(profile_path: Path) -> dict:
    if profile_path.exists():
        with profile_path.open("r", encoding="utf-8") as file:
            return json.load(file)

    #if the profile dosent exist make the parent folder exist
    profile_path.parent.mkdir(parents=True, exist_ok=True)

    player_name = input("Enter player name: ").strip()
    if not player_name:
        player_name = "Player"

    #init profile
    profile = {
        "player_name": player_name,
        "games_played": 0,
        "max_treasure_collected": 0,
        "most_rooms_world_completed": 0,
        "timestamp_last_played": "Never"
    }

    with profile_path.open("w", encoding="utf-8") as file:
        #write to json file
        json.dump(profile, file, indent=4)

    return profile


def save_profile(profile_path: Path, profile: dict) -> None:

    profile_path.parent.mkdir(parents=True, exist_ok=True)

    with profile_path.open("w", encoding="utf-8") as file:
        #write profile dict into the file as json
        json.dump(profile, file, indent=4)


def update_profile(profile: dict, engine: GameEngine) -> dict:

    profile["games_played"] = profile.get("games_played", 0) + 1

    collected = engine.player.get_collected_count()

    #update treasure collected if its more then last run
    if collected > profile.get("max_treasure_collected", 0):
        profile["max_treasure_collected"] = collected

    current_room = engine.player.get_room()

    #updates progress if this run is the farthest
    if current_room > profile.get("most_rooms_world_completed", 0):
        profile["most_rooms_world_completed"] = current_room

    profile["timestamp_last_played"] = datetime.now(timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ")

    return profile


def main_curses(stdscr, config_path: str, profile: dict, profile_path: Path) -> None:
    engine = GameEngine(config_path)
    ui = GameUI(stdscr, engine, profile, str(profile_path))

    try:
        ui.run() #start game loop
    finally:
        updated_profile = update_profile(profile, engine) #update profile with final game state
        save_profile(profile_path, updated_profile) #save to json
        engine.destroy()


def main():
    args = parse_args()

    config_path = args.config
    profile_path = Path(args.profile)

    profile = load_or_create_profile(profile_path)

    curses.wrapper(main_curses, config_path, profile, profile_path)


if __name__ == "__main__":
    main()