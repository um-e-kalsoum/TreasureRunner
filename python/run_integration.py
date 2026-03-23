#!/usr/bin/env python3
"""Deterministic system integration test runner for Treasure Runner."""

# below are the imports that are in the instructors solution.
# you may need different ones.  Use them if you wish.
import os
import argparse
from treasure_runner.bindings import Direction
from treasure_runner.models.game_engine import GameEngine
from treasure_runner.models.exceptions import GameError, ImpassableError

# ===================
# helpers for output
# ===================

# writes one log record to output
def _write(fp, event: str, fields: list[tuple[str, str]] | None = None) -> None:
    parts = [event]
    if fields:
        for k, v in fields:
            parts.append(f"{k}={v}")
    fp.write("|".join(parts) + "\n")

# player state string fomrmat
def _player_state(engine: GameEngine) -> str:
    room = engine.player.get_room()
    x, y = engine.player.get_position()
    collected = engine.player.get_collected_count()
    return f"room={room}|x={x}|y={y}|collected={collected}"

# get collected value
def _split_collected(state: str) -> int:
    return int(state.split("collected=")[1])

# ===================
# move and classify
# ===================

def _attempt_move(engine: GameEngine, direction: Direction) -> tuple[str, str, str, int]:    
    before = _player_state(engine)
    before_colect = _split_collected(before)

    try:
        engine.move_player(direction)
    except ImpassableError:
        return "BLOCKED", before, before, 0
    except GameError:
        return "ERROR", before, before, 0

    # move did not cause an exception, record state
    after = _player_state(engine)
    after_colect = _split_collected(after)

    #compute treasures collected
    delta = after_colect - before_colect

    if after == before:
        return "NO_PROGRESS", before, after, delta
    return "OK", before, after, delta

# ===================
# entry direction
# ===================

def _find_entry_direction(engine: GameEngine) -> Direction:

    trial_order = [Direction.SOUTH, Direction.WEST, Direction.NORTH, Direction.EAST]

    for d in trial_order:
        #record before move
        before_room = engine.player.get_room()
        before_x, before_y = engine.player.get_position()

        try:
            engine.move_player(d)
            after_room = engine.player.get_room()
            after_x, after_y = engine.player.get_position()

            #valid move? same room different x,y
            ok = (after_room == before_room) and (after_x != before_x or after_y != before_y)

            engine.reset()

            if ok:
                return d

        except ImpassableError:
            #try next direction
            engine.reset()
            continue
        except GameError:
            engine.reset()
            continue

    raise RuntimeError("No valid entry direction found.")


# ===================
# sweep runner
# ===================

def _run_sweep(fp, engine: GameEngine, phase: str, direction: Direction, step: int) -> tuple[int, int]:
    
    _write(fp, "SWEEP_START", [("phase", phase), ("dir", direction.name)])

    #set to track visited player states
    seen: set[str] = set()
    #amount of succesful moves in the given direction
    moves = 0

    while True:
        state_now = _player_state(engine)

        #if state repeats then there is a cycle
        if state_now in seen:
            _write(fp, "SWEEP_END", [("phase", phase), ("reason", "CYCLE_DETECTED"), ("moves", str(moves))])
            return step, _split_collected(state_now)

        seen.add(state_now)

        #try move
        result, before, after, delta = _attempt_move(engine, direction)
        step += 1

        if result == "OK":
            moves += 1

        #log move
        _write(
            fp,
            "MOVE",
            [
                ("step", str(step)),
                ("phase", phase),
                ("dir", direction.name),
                ("result", result),
                ("before", before),
                ("after", after),
                ("delta_collected", str(delta)),
            ],
        )

        if result in ("BLOCKED", "NO_PROGRESS"):
            _write(fp, "SWEEP_END", [("phase", phase), ("reason", "BLOCKED"), ("moves", str(moves))])
            return step, _split_collected(after)

        if result == "ERROR":
            _write(fp, "SWEEP_END", [("phase", phase), ("reason", "ERROR"), ("moves", str(moves))])
            return step, _split_collected(after)


def parse_args():
    parser = argparse.ArgumentParser(description="Treasure Runner integration test logger")
    parser.add_argument(
        "--config",
        required=True,
        help="Path to generator config file",
    )
    parser.add_argument(
        "--log",
        required=True,
        help="Output log path",
    )
    return parser.parse_args()


def main():
    args = parse_args()
    config_path = os.path.abspath(args.config)
    log_path = os.path.abspath(args.log)

    #config file exists?
    if not os.path.isfile(config_path):
        print(f"Error: invalid --config (file not found): {config_path}")
        return 1

    #log dir exists?
    log_dir = os.path.dirname(log_path) or "."
    if not os.path.isdir(log_dir):
        print(f"Error: invalid --log (directory not found): {log_dir}")
        return 1


    with open(log_path, "w", encoding="utf-8") as fp:

        #create game engine and set up first room
        engine = GameEngine(config_path)
        rooms = engine.get_room_count()
        room_width, room_height = engine.get_room_dimensions()

        #log the start of the run 
        _write(
            fp,
            "RUN_START",
            [
                ("config", config_path),
                ("rooms", str(rooms)),
                ("room_width", str(room_width)),
                ("room_height", str(room_height)),
            ],
        )

        step = 0
        #log spawn state
        _write(fp, "STATE", [("step", str(step)), ("phase", "SPAWN"), ("state", _player_state(engine))])

        #find first valid entry direction from spawn and log it
        entry_dir = _find_entry_direction(engine)
        _write(fp, "ENTRY", [("direction", entry_dir.name)])

        #preform the move and log it
        result, before, after, delta = _attempt_move(engine, entry_dir)
        step = 1

        _write(
            fp,
            "MOVE",
            [
                ("step", str(step)),
                ("phase", "ENTRY"),
                ("dir", entry_dir.name),
                ("result", result),
                ("before", before),
                ("after", after),
                ("delta_collected", str(delta)),
            ],
        )

        #log end of run if error occurs
        if result == "ERROR":
            fp.write("TERMINATED: Initial Move Error\n")
            _write(fp, "RUN_END", [("steps", str(step)), ("collected_total", str(_split_collected(after)))])
            return 0

        #fixed order
        sweeps = [
            ("SWEEP_SOUTH", Direction.SOUTH),
            ("SWEEP_WEST", Direction.WEST),
            ("SWEEP_NORTH", Direction.NORTH),
            ("SWEEP_EAST", Direction.EAST),
        ]

        #loop each sweep phase and log as it goes
        for phase, d in sweeps:
            step, _ = _run_sweep(fp, engine, phase, d, step)

        #save final state and log it
        final_state = _player_state(engine)
        _write(fp, "STATE", [("step", str(step)), ("phase", "FINAL"), ("state", final_state)])
        _write(fp, "RUN_END", [("steps", str(step)), ("collected_total", str(_split_collected(final_state)))])

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
