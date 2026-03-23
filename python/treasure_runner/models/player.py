import ctypes
from treasure_runner.bindings import lib, Status, Treasure
from treasure_runner.models.exceptions import status_to_exception

class Player:
 #defines a python player wrapper
 #python object that stores a pointer to the c player

    def __init__(self, ptr):
        self._ptr = ptr
        #constructor for the python wrapper
        #stores the raw c player pointer in self._ptr

    def get_room(self) -> int:
        return int(lib.player_get_room(self._ptr))

    def get_position(self) -> tuple[int, int]:
        x = ctypes.c_int()
        y = ctypes.c_int()
        status = int(lib.player_get_position(self._ptr, ctypes.byref(x), ctypes.byref(y)))

        if status != Status.OK:
            raise status_to_exception(Status(status), "player_get_position failed")

        return int(x.value), int(y.value)

    def get_collected_count(self) -> int:
        return int(lib.player_get_collected_count(self._ptr))

    def has_collected_treasure(self, treasure_id: int) -> bool:
        return bool(lib.player_has_collected_treasure(self._ptr, int(treasure_id)))

    def get_collected_treasures(self) -> list[dict]:
        count = ctypes.c_int()
        arr = lib.player_get_collected_treasures(self._ptr, ctypes.byref(count))

        treasure_count = int(count.value)
        out = []

        for i in range(treasure_count):
            t_ptr = arr[i]

            #skip null
            if not t_ptr:
                continue

            #dereference
            treasure: Treasure = t_ptr.contents

            #c string to python string
            if treasure.name:
                name = treasure.name.decode("utf-8")
            else:
                name = ""

            out.append( {
                    "id": int(treasure.id),
                    "name": name,
                    "starting_room_id": int(treasure.starting_room_id),
                    "initial_x": int(treasure.initial_x),
                    "initial_y": int(treasure.initial_y),
                    "x": int(treasure.x),
                    "y": int(treasure.y),
                    "collected": bool(treasure.collected),
                    }
                )

        return out
