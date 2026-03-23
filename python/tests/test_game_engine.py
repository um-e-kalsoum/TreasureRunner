import os
import unittest

from treasure_runner.bindings import Direction
from treasure_runner.models.game_engine import GameEngine
from treasure_runner.models.exceptions import GameError, ImpassableError


class TestGameEngine(unittest.TestCase):

    def setUp(self):
        config_path = os.path.abspath(
            os.path.join(os.path.dirname(__file__), "..", "..", "assets", "starter.ini")
        )
        self.engine = GameEngine(config_path)

    def tearDown(self):
        self.engine.destroy()

    def test_engine_create_player_exists(self):
        self.assertIsNotNone(self.engine.player)

    def test_room_count_positive(self):
        rooms = self.engine.get_room_count()
        self.assertIsInstance(rooms, int)
        self.assertGreater(rooms, 0)

    def test_room_dimensions_positive(self):
        w, h = self.engine.get_room_dimensions()
        self.assertIsInstance(w, int)
        self.assertIsInstance(h, int)
        self.assertGreater(w, 0)
        self.assertGreater(h, 0)

    def test_get_room_ids_matches_room_count_and_unique(self):
        ids = self.engine.get_room_ids()
        self.assertIsInstance(ids, list)
        self.assertEqual(len(ids), self.engine.get_room_count())
        self.assertEqual(len(ids), len(set(ids)))

    def test_render_current_room_returns_string_with_newlines(self):
        s = self.engine.render_current_room()
        self.assertIsInstance(s, str)
        self.assertGreater(len(s), 0)
        self.assertIn("\n", s)

    def test_destroy_is_idempotent(self):
        self.engine.destroy()
        self.engine.destroy()

    def test_reset_is_idempotent(self):
        self.engine.reset()
        self.engine.reset()

    def test_reset_restores_spawn_state(self):
        spawn_room = self.engine.player.get_room()
        spawn_pos = self.engine.player.get_position()
        spawn_collected = self.engine.player.get_collected_count()

        for d in (Direction.NORTH, Direction.EAST, Direction.SOUTH, Direction.WEST):
            try:
                self.engine.move_player(d)
            except ImpassableError:
                pass
            except GameError:
                pass

        self.engine.reset()

        self.assertEqual(self.engine.player.get_room(), spawn_room)
        self.assertEqual(self.engine.player.get_position(), spawn_pos)
        self.assertEqual(self.engine.player.get_collected_count(), spawn_collected)

    
if __name__ == "__main__":
    unittest.main()