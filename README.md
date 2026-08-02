# Treasure Runner

Treasure Runner is a terminal-based puzzle game built with a C game engine and a Python `curses` interface. Players explore connected rooms, collect treasures, push objects, use portals, and try to collect every treasure in the world.

## Features

- C game engine with Python bindings
- Colored terminal interface
- WASD and arrow-key movement
- Treasures, portals, switches, and pushable objects
- Undo, room reset, and full-game reset
- Persistent player statistics saved in JSON
- Docker-based development environment

## Running the Project

This project runs inside the provided VS Code Dev Container.

### Requirements

- Docker Desktop
- Visual Studio Code
- Dev Containers extension

Open the project in VS Code, then select:

```text
Dev Containers: Reopen in Container
```

From the directory containing `run_game.py`, run:

```bash
python3 run_game.py --config ../assets/test.ini --profile ../assets/player_profile.json
```

If the profile file does not exist, the game will ask for a player name and create one automatically.

## Controls

| Key | Action |
|---|---|
| `WASD` or arrow keys | Move |
| `>` | Enter a portal |
| `U` | Undo the last move |
| `I` | Reset the current room |
| `R` | Reset the full game |
| `Q` | Quit |

The terminal should be at least 80 columns by 20 rows.

## Project Overview

The C layer handles rooms, movement, treasures, portals, graphs, rendering, and memory management. The Python layer handles the terminal UI, keyboard input, player profiles, and saved statistics.

## Author

Developed by Um-e-kalsoum Asif for CIS*2750.

   