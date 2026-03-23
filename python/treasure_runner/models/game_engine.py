import ctypes

from treasure_runner.bindings import lib, Status, Direction
from treasure_runner.models.exceptions import status_to_exception
from treasure_runner.models.player import Player


class GameEngine:
# python class that wraps the raw c engine pointer
#does not store the actual game data in python but it 
#stores a pointer to the c engine and calls c functions when needed
    

    #creates game engine from config file
    def __init__(self, config_path: str):
        self._destroyed = False
        self._eng = ctypes.c_void_p()
        self._player = None

        #create engine, raise exception if failed
        status = int(lib.game_engine_create(config_path.encode("utf-8"), ctypes.byref(self._eng)))
        if status != Status.OK:
            raise status_to_exception(Status(status), "game_engine_create failed")

        #get player pointer, raise exception if null
        p_ptr = lib.game_engine_get_player(self._eng)
        if not p_ptr:
            self.destroy()
            raise RuntimeError("game_engine_get_player is NULL")

        #python wrapper for raw c player pointer
        self._player = Player(p_ptr)

    #lets us access like engine.player instead of engine.player()
    @property
    def player(self) -> Player:
        return self._player

    #destroys the c engine and markss wrapper as destroyed
    def destroy(self) -> None:
        #already destroyed
        if self._destroyed:
            return

        #destroy if exists
        if self._eng:
            lib.game_engine_destroy(self._eng)
        self._eng = None
        self._destroyed = True


    def move_player(self, direction: Direction) -> None:
        status = int(lib.game_engine_move_player(self._eng, int(direction)))
        if status != Status.OK:
            raise status_to_exception(Status(status), "game_engine_move_player failed")

    #returns the current room as a python string
    def render_current_room(self) -> str:
        out = ctypes.c_char_p()

        status = int(lib.game_engine_render_current_room(self._eng, ctypes.byref(out)))
        if status != Status.OK:
            raise status_to_exception(Status(status), "game_engine_render_current_room failed")

        #if pointer is null or string is empty/null free and return empty string
        if not out or not out.value:
            if out:
                lib.game_engine_free_string(out)
            return ""

        result = out.value.decode("utf-8")
        #free c allocated string
        lib.game_engine_free_string(out)

        return result

    def get_room_count(self) -> int:
        count = ctypes.c_int()
        status = int(lib.game_engine_get_room_count(self._eng, ctypes.byref(count)))
        if status != Status.OK:
            raise status_to_exception(Status(status), "game_engine_get_room_count failed")
        return int(count.value)

    def get_room_dimensions(self) -> tuple[int, int]:
        width = ctypes.c_int()
        height = ctypes.c_int()
        status = int(lib.game_engine_get_room_dimensions(self._eng, ctypes.byref(width), ctypes.byref(height)))

        if status != Status.OK:
            raise status_to_exception(Status(status), "game_engine_get_room_dimensions failed")

        return int(width.value), int(height.value)

    #returns all room ids as a python list of ints 
    def get_room_ids(self) -> list[int]:
        ids_ptr = ctypes.POINTER(ctypes.c_int)()
        count = ctypes.c_int()

        status = int(lib.game_engine_get_room_ids(
            self._eng,
            ctypes.byref(ids_ptr),
            ctypes.byref(count)
        ))

        if status != Status.OK:
            raise status_to_exception(Status(status), "game_engine_get_room_ids failed")

        room_count = int(count.value)
        out = []

        for i in range(room_count):
            out.append(int(ids_ptr[i]))

        if ids_ptr:
            lib.game_engine_free_string(ids_ptr)

        return out

    #reset to inital state
    def reset(self) -> None:
        status = int(lib.game_engine_reset(self._eng))

        if status != Status.OK:
            raise status_to_exception(Status(status), "game_engine_reset failed")
