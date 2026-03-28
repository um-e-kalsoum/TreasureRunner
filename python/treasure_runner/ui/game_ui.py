import curses

from treasure_runner.bindings import Direction
from treasure_runner.models.exceptions import GameError, ImpassableError, NoPortalError, OutOfBoundsError, InvalidArgumentError
import time

class GameUI:
    def __init__(self, stdscr, engine, profile, profile_path):
        self._stdscr = stdscr
        self._engine = engine
        self._profile = profile
        self._profile_path = profile_path
        self._message = "Welcome to the Treasure Runner"
        self._progress_text = "0/0 treasures collected"
        self._won = False
        self._start_time = None
        self._completion_time_seconds = None
        self._charset = self._engine.get_charset()
        self._visited_rooms = set()

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
        curses.init_pair(2, curses.COLOR_WHITE, curses.COLOR_BLACK)
        curses.init_pair(4, curses.COLOR_YELLOW, curses.COLOR_BLACK)

        curses.init_pair(5, curses.COLOR_GREEN, curses.COLOR_BLACK)
        curses.init_pair(6, curses.COLOR_YELLOW, curses.COLOR_BLACK)
        curses.init_pair(7, curses.COLOR_MAGENTA, curses.COLOR_BLACK)
        curses.init_pair(8, curses.COLOR_RED, curses.COLOR_BLACK)
        curses.init_pair(9, curses.COLOR_WHITE, curses.COLOR_BLACK)
        curses.init_pair(10, curses.COLOR_BLUE, curses.COLOR_BLACK)

    def run(self) -> None:
        self.init_screen()
        self.show_startup_screen()
        self._start_time = time.time()
        self._update_progress_text()
        self._visited_rooms.add(self._engine.player.get_room())

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
                self._update_progress_text()
                self.message("Game reset")
            except GameError as e:
                self.message(str(e))
            return True

        if key == ord("i"):
            try:
                self._engine.reset_current_room()
                self._update_progress_text()
                self.message("Current room reset")
            except GameError as e:
                self.message(str(e))
            return True

        if key == ord("u"):
            try:
                self._engine.undo_last_move()
                self._update_progress_text()
                self.message("Undid last move")
            except InvalidArgumentError:
                self.message("You already undid one move")
            except GameError as e:
                self.message(str(e))
            return True


        if key == ord(">"):
            try:
                self._engine.use_portal()
                self._visited_rooms.add(self._engine.player.get_room())
                self._update_progress_text()
                self.message("You entered a portal")
            except NoPortalError:
                self.message("You are not standing on a portal")
            except GameError as e:
                self.message(str(e))
            return True

        direction = self._read_direction(key)

        if direction is None:
            self.message("Not a valid key")
            return True

        before_collected = self._engine.player.get_collected_count()

        try:
            self._engine.move_player(direction)
            self._visited_rooms.add(self._engine.player.get_room())

            after_collected = self._engine.player.get_collected_count()
            self._update_progress_text()

            room_text = self._engine.render_current_room()
            px, py = self._engine.player.get_position()
            lines = room_text.splitlines()

            current_tile = None
            if 0 <= py < len(lines) and 0 <= px < len(lines[py]):
                current_tile = lines[py][px]

            if after_collected > before_collected:
                self.message("You picked up a treasure")
            else:
                self.message(f"You moved {direction.name.lower()}")

            total = self._engine.get_total_treasure_count()
            if after_collected == total:
                self.message("You Win!")
                self._won = True
                self._completion_time_seconds = int(time.time() - self._start_time)
                return False

        except OutOfBoundsError:
            self.message("You cannot go that way")
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

        message_x = 0
        progress_x = 45

        left_text = self._message
        right_text = self._progress_text

        self._stdscr.move(0, 0)
        self._stdscr.clrtoeol()

        if progress_x >= max_x:
            progress_x = max_x - 1

        left_max_len = progress_x - message_x - 2
        if left_max_len < 0:
            left_max_len = 0

        self._stdscr.addstr(0, message_x, left_text[:left_max_len], curses.color_pair(4))

        if progress_x < max_x:
            self._stdscr.addstr(0, progress_x, right_text[:max_x - progress_x - 1], curses.color_pair(4))

    def _draw_room_info(self) -> None:
        room_id = self._engine.player.get_room()
        title = f"Room: {room_id}"
        self._stdscr.addstr(2, 2, title, curses.A_BOLD)

    def _draw_room(self) -> None:
        room_text = self._engine.render_current_room()
        lines = room_text.splitlines()

        start_y = 4
        start_x = 2

        cs = self._charset

        for i, line in enumerate(lines):
            for j, ch in enumerate(line):
                color = curses.color_pair(2)

                if ch == cs["player"]:
                    color = curses.color_pair(5)
                elif ch == cs["treasure"]:
                    color = curses.color_pair(6)
                elif ch == cs["portal"]:
                    color = curses.color_pair(7)
                elif ch == cs["pushable"]:
                    color = curses.color_pair(8)
                elif ch == cs["floor"]:
                    color = curses.color_pair(9)
                elif ch == cs["wall"]:
                    color = curses.color_pair(10)

                self._stdscr.addch(start_y + i, start_x + j, ch, color)

    def _draw_legend(self) -> None:
        room_text = self._engine.render_current_room()
        lines = room_text.splitlines()

        longest = 0
        for line in lines:
            if len(line) > longest:
                longest = len(line)

        legend_x = longest + 8
        legend_y = 4
        cs = self._charset

        self._stdscr.addstr(legend_y, legend_x, "Game Elements", curses.A_BOLD)

        self._stdscr.addstr(legend_y + 2, legend_x, f"{cs['player']} - player", curses.color_pair(5))
        self._stdscr.addstr(legend_y + 3, legend_x, f"{cs['treasure']} - treasure", curses.color_pair(6))
        self._stdscr.addstr(legend_y + 4, legend_x, f"{cs['portal']} - portal", curses.color_pair(7))
        self._stdscr.addstr(legend_y + 5, legend_x, f"{cs['pushable']} - pushable", curses.color_pair(8))
        self._stdscr.addstr(legend_y + 6, legend_x, f"{cs['floor']} - floor", curses.color_pair(9))
        self._stdscr.addstr(legend_y + 7, legend_x, f"{cs['wall']} - wall", curses.color_pair(10))


    def _draw_controls(self) -> None:
        room_text = self._engine.render_current_room()
        lines = room_text.splitlines()

        controls_y = 6 + len(lines)
        max_y, max_x = self._stdscr.getmaxyx()

        controls_line1 = "Game Controls:"
        controls_line2 = "WASD / arrows = move, > = enter portal, u = undo last move"
        controls_line3 = "i = return to inital room state, r = full game reset, q = quit"

        if controls_y + 2 >= max_y:
            self._check_size()
            return

        self._stdscr.addstr(controls_y, 2, controls_line1[:max_x - 3])
        self._stdscr.addstr(controls_y + 1, 2, controls_line2[:max_x - 3])
        self._stdscr.addstr(controls_y + 2, 2, controls_line3[:max_x - 3])

    def _draw_status_bar(self) -> None:
        max_y, max_x = self._stdscr.getmaxyx()

        x, y = self._engine.player.get_position()
        collected = self._engine.player.get_collected_count()
        player_name = self._profile.get("player_name", "Player")

        status = f"Player: {player_name}   Position: ({x}, {y})   Treasures: {collected}"

        self._stdscr.move(max_y - 2, 0)
        self._stdscr.clrtoeol()
        self._stdscr.addstr(max_y - 2, 0, status[:max_x - 1], curses.color_pair(4))

        footer = "Treasure Runner \t\t\t\t\t uasif@uoguelph.ca"

        self._stdscr.move(max_y - 1, 0)
        self._stdscr.clrtoeol()

        self._stdscr.addstr(max_y - 1, 0, footer[:max_x - 1], curses.color_pair(6))


    def message(self, msg: str) -> None:
        self._message = msg

    def _update_progress_text(self) -> None:
        collected = self._engine.player.get_collected_count()
        total = self._engine.get_total_treasure_count()
        self._progress_text = f"{collected}/{total} treasures collected"

    def show_startup_screen(self) -> None:
        self._stdscr.clear()

        best_time = self._profile.get("best_completion_time_seconds")
        best_time_text = "None" if best_time is None else f"{best_time} seconds"


        lines = [
            f"Welcome to the Treasure Runner {self._profile.get('player_name', 'Player')}!",
            "",
            "Your stats are...",
            "",
            f"Games played: {self._profile.get('games_played', 0)}",
            f"Total wins: {self._profile.get('total_wins', 0)}",
            f"Max treasure collected: {self._profile.get('max_treasure_collected', 0)}",
            f"Most rooms/world completed: {self._profile.get('most_rooms_world_completed', 0)}",
            f"Best win time: {best_time_text}",
            f"Last played: {self._profile.get('timestamp_last_played', 'Never')}",
            "",
            "Press any key to continue"
        ]

        self._draw_centered(lines, curses.color_pair(4) | curses.A_BOLD)
        self._stdscr.getch()

    def show_end_screen(self) -> None:
        self._stdscr.clear()

        if self._won:
            title = f"You Win {self._profile.get('player_name', 'Player')}!"
        else:
            title = f"Game Over {self._profile.get('player_name', 'Player')}."

        best_time = self._profile.get("best_completion_time_seconds")
        best_time_text = "None" if best_time is None else f"{best_time} seconds"


        lines = [
            title,
            "",
            "Your stats are...",
            "",
            f"Games played: {self._profile.get('games_played', 0)}",
            f"Total wins: {self._profile.get('total_wins', 0)}",
            f"Max treasure collected: {self._profile.get('max_treasure_collected', 0)}",
            f"Most rooms/world completed: {self._profile.get('most_rooms_world_completed', 0)}",
            f"Best win time: {best_time_text}",
            f"Last played: {self._profile.get('timestamp_last_played', 'Never')}",
            "",
            "Press any key to exit"
        ]

        self._draw_centered(lines, curses.color_pair(4) | curses.A_BOLD)
        self._stdscr.getch()

    def _draw_centered(self, lines, attr=curses.A_NORMAL) -> None:
        max_y, max_x = self._stdscr.getmaxyx()
        start_y = (max_y - len(lines)) // 2

        if start_y < 0:
            start_y = 0

        for i, line in enumerate(lines):
            start_x = (max_x - len(line)) // 2
            if start_x < 0:
                start_x = 0

            if start_y + i < max_y:
                self._stdscr.addstr(start_y + i, start_x, line[:max_x - 1], attr)

        self._stdscr.refresh()

    def _check_size(self) -> None:
        while True:
            max_y, max_x = self._stdscr.getmaxyx()

            if max_y >= 20 and max_x >= 80:
                return

            self._stdscr.clear()

            msg1 = "Terminal too small"
            msg2 = "Please resize the window"
            msg3 = "The game will continue automatically"

            y = max_y // 2
            x1 = max(0, (max_x - len(msg1)) // 2)
            x2 = max(0, (max_x - len(msg2)) // 2)
            x3 = max(0, (max_x - len(msg3)) // 2)

            if y - 1 >= 0:
                self._stdscr.addstr(y - 1, x1, msg1[:max_x - 1], curses.A_BOLD)
            if y >= 0:
                self._stdscr.addstr(y, x2, msg2[:max_x - 1])
            if y + 1 < max_y:
                self._stdscr.addstr(y + 1, x3, msg3[:max_x - 1])

            self._stdscr.refresh()
            curses.napms(200)
