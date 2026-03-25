import curses

from treasure_runner.bindings import Direction
from treasure_runner.models.exceptions import GameError, ImpassableError, NoPortalError, OutOfBoundsError


class GameUI:
    def __init__(self, stdscr, engine, profile, profile_path):
        self._stdscr = stdscr
        self._engine = engine
        self._profile = profile
        self._profile_path = profile_path
        self._message = "Welcome to the Treasure Runner"

    def init_screen(self) -> None:
        curses.cbreak()
        curses.noecho()
        curses.curs_set(0)
        self._stdscr.keypad(True)

        if curses.has_colors():
            curses.start_color()
            self._init_colors()

        self._stdscr.clear()
        self._stdscr.refresh()

    @classmethod
    def _init_colors(cls) -> None:
        curses.init_pair(1, curses.COLOR_CYAN, curses.COLOR_BLACK)
        curses.init_pair(2, curses.COLOR_WHITE, curses.COLOR_BLACK)
        curses.init_pair(3, curses.COLOR_BLACK, curses.COLOR_WHITE)
        curses.init_pair(4, curses.COLOR_YELLOW, curses.COLOR_BLACK)

    def run(self) -> None:
        self.init_screen()
        self.show_startup_screen()

        running = True
        while running:
            self.render()
            key = self._stdscr.getch()
            running = self._handle_key(key)

    def _handle_key(self, key: int) -> bool:
        if key == ord("q"):
            self.message("Quitting game...")
            return False

        if key == ord("r"):
            try:
                self._engine.reset()
                self.message("Game reset")
            except GameError as e:
                self.message(str(e))
            return True

        if key == ord(">"):
            self.message("Portal action not added")
            return True

        direction = self._read_direction(key)

        if direction is None:
            return True

        try:
            self._engine.move_player(direction)
            self.message(f"Moved {direction.name}")
        except OutOfBoundsError:
            self.message("You cannot go out of bounds")
        except ImpassableError:
            self.message("You cannot go that way")
        except NoPortalError:
            self.message("No portal here")
        except GameError as e:
            self.message(str(e))

        return True

    def _read_direction(self, key: int):
        if key == ord("w") or key == curses.KEY_UP:
            return Direction.NORTH
        if key == ord("s") or key == curses.KEY_DOWN:
            return Direction.SOUTH
        if key == ord("a") or key == curses.KEY_LEFT:
            return Direction.WEST
        if key == ord("d") or key == curses.KEY_RIGHT:
            return Direction.EAST
        return None

    def render(self) -> None:
        self._stdscr.clear()
        self._check_size()

        self._draw_message_bar()
        self._draw_room_info()
        self._draw_room()
        self._draw_legend()
        self._draw_controls()
        self._draw_status_bar()

        self._stdscr.refresh()

    def _draw_message_bar(self) -> None:
        max_y, max_x = self._stdscr.getmaxyx()
        self._stdscr.move(0, 0)
        self._stdscr.clrtoeol()
        self._stdscr.addstr(0, 0, self._message[:max_x - 1], curses.color_pair(4))

    def _draw_room_info(self) -> None:
        room_id = self._engine.player.get_room()
        title = f"Room: {room_id}"
        self._stdscr.addstr(2, 2, title, curses.A_BOLD)

    def _draw_room(self) -> None:
        room_text = self._engine.render_current_room()
        lines = room_text.splitlines()

        start_y = 4
        start_x = 2

        for i, line in enumerate(lines):
            self._stdscr.addstr(start_y + i, start_x, line, curses.color_pair(1))

    def _draw_legend(self) -> None:
        room_text = self._engine.render_current_room()
        lines = room_text.splitlines()

        longest = 0
        for line in lines:
            if len(line) > longest:
                longest = len(line)

        legend_x = longest + 8
        legend_y = 4

        self._stdscr.addstr(legend_y, legend_x, "Game Elements:", curses.A_BOLD)
        self._stdscr.addstr(legend_y + 2, legend_x, "@ - player")
        self._stdscr.addstr(legend_y + 3, legend_x, "# - wall")
        self._stdscr.addstr(legend_y + 4, legend_x, "$ - gold")
        self._stdscr.addstr(legend_y + 5, legend_x, "x - exit")

    def _draw_controls(self) -> None:
        room_text = self._engine.render_current_room()
        lines = room_text.splitlines()

        controls_y = 6 + len(lines)
        self._stdscr.addstr(controls_y, 2, "Controls: wasd / Arrows move, r reset, q quit, > portal")

    def _draw_status_bar(self) -> None:
        max_y, max_x = self._stdscr.getmaxyx()

        x, y = self._engine.player.get_position()
        collected = self._engine.player.get_collected_count()
        player_name = self._profile.get("player_name", "Player")

        status = f"Player: {player_name}   Position: ({x}, {y})   Treasures: {collected}"

        self._stdscr.move(max_y - 2, 0)
        self._stdscr.clrtoeol()
        self._stdscr.addstr(max_y - 2, 0, status[:max_x - 1], curses.color_pair(4))

        footer = "Treasure Runner"
        self._stdscr.move(max_y - 1, 0)
        self._stdscr.clrtoeol()
        self._stdscr.addstr(max_y - 1, 0, footer[:max_x - 1], curses.A_DIM)

    def message(self, msg: str) -> None:
        self._message = msg

    def show_startup_screen(self) -> None:
        self._stdscr.clear()

        lines = [
            "Treasure Runner",
            "",
            f"Player: {self._profile.get('player_name', 'Player')}",
            f"Games played: {self._profile.get('games_played', 0)}",
            f"Max treasure collected: {self._profile.get('max_treasure_collected', 0)}",
            f"Most rooms/world completed: {self._profile.get('most_rooms_world_completed', 0)}",
            f"Last played: {self._profile.get('timestamp_last_played', 'Never')}",
            "",
            "Controls:",
            "WASD / Arrow Keys = move",
            "r = reset",
            "q = quit",
            "> = portal",
            "",
            "Press any key to continue"
        ]

        self._draw_centered(lines)
        self._stdscr.getch()

    def show_end_screen(self) -> None:
        self._stdscr.clear()

        lines = [
            "Game Over",
            "",
            f"Player: {self._profile.get('player_name', 'Player')}",
            f"Games played: {self._profile.get('games_played', 0)}",
            f"Max treasure collected: {self._profile.get('max_treasure_collected', 0)}",
            f"Most rooms/world completed: {self._profile.get('most_rooms_world_completed', 0)}",
            f"Last played: {self._profile.get('timestamp_last_played', 'Never')}",
            "",
            "Press any key to exit"
        ]

        self._draw_centered(lines)
        self._stdscr.getch()

    def _draw_centered(self, lines) -> None:
        max_y, max_x = self._stdscr.getmaxyx()
        start_y = (max_y - len(lines)) // 2

        if start_y < 0:
            start_y = 0

        for i, line in enumerate(lines):
            start_x = (max_x - len(line)) // 2
            if start_x < 0:
                start_x = 0

            if start_y + i < max_y:
                self._stdscr.addstr(start_y + i, start_x, line[:max_x - 1])

        self._stdscr.refresh()

    def _check_size(self) -> None:
        max_y, max_x = self._stdscr.getmaxyx()

        if max_y < 20 or max_x < 60:
            raise RuntimeError("Terminal too small. Please resize it and try again.")