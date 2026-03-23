import os
import unittest

from treasure_runner.models.game_engine import GameEngine


class TestPlayer(unittest.TestCase):

    def setUp(self):
        config_path = os.path.abspath(
            os.path.join(os.path.dirname(__file__), "..", "..", "assets", "starter.ini")
        )
        self.engine = GameEngine(config_path)
        self.player = self.engine.player

    def tearDown(self):
        self.engine.destroy()

    def test_get_room_returns_int(self):
        room_id = self.player.get_room()
        self.assertIsInstance(room_id, int)

    def test_get_position_returns_two_ints(self):
        x, y = self.player.get_position()
        self.assertIsInstance(x, int)
        self.assertIsInstance(y, int)

    def test_get_collected_count_returns_int_and_non_negative(self):
        c = self.player.get_collected_count()
        self.assertIsInstance(c, int)
        self.assertGreaterEqual(c, 0)

    def test_has_collected_treasure_returns_bool(self):
        v = self.player.has_collected_treasure(0)
        self.assertIsInstance(v, bool)

    def test_has_collected_treasure_false_for_large_id(self):
        self.assertFalse(self.player.has_collected_treasure(10**9))

    def test_get_collected_treasures_returns_list(self):
        treasures = self.player.get_collected_treasures()
        self.assertIsInstance(treasures, list)

    def test_get_collected_treasures_items_schema_and_types(self):
        treasures = self.player.get_collected_treasures()
        for t in treasures:
            self.assertIsInstance(t, dict)
            self.assertEqual(
                set(t.keys()), {
                                    "id",
                                    "name",
                                    "starting_room_id",
                                    "initial_x",
                                    "initial_y",
                                    "x",
                                    "y",
                                    "collected",
                                },
                            )
                            
            self.assertIsInstance(t["id"], int)
            self.assertIsInstance(t["name"], str)
            self.assertIsInstance(t["starting_room_id"], int)
            self.assertIsInstance(t["initial_x"], int)
            self.assertIsInstance(t["initial_y"], int)
            self.assertIsInstance(t["x"], int)
            self.assertIsInstance(t["y"], int)
            self.assertIsInstance(t["collected"], bool)


if __name__ == "__main__":
    unittest.main()