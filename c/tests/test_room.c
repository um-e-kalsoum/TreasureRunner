#include <check.h>
#include <stdlib.h>
#include <string.h>
#include "room.h"
#include "types.h"

START_TEST(create_ok)
{
    Room *r = room_create(5, "Hall", 4, 3);
    ck_assert_ptr_nonnull(r);
    ck_assert_int_eq(r->id, 5);
    ck_assert_int_eq(r->width, 4);
    ck_assert_int_eq(r->height, 3);
    room_destroy(r);
}
END_TEST

START_TEST(create_null_name)
{
    Room *r = room_create(2, NULL, 1, 1);
    ck_assert_ptr_nonnull(r);
    ck_assert_ptr_null(r->name);
    room_destroy(r);
}
END_TEST

START_TEST(get_width_ok)
{
    Room *r = room_create(1, "R", 8, 5);
    ck_assert_int_eq(room_get_width(r), 8);
    room_destroy(r);
}
END_TEST

START_TEST(get_width_null)
{
    ck_assert_int_eq(room_get_width(NULL), 0);
}
END_TEST

START_TEST(get_height_ok)
{
    Room *r = room_create(1, "R", 6, 9);
    ck_assert_int_eq(room_get_height(r), 9);
    room_destroy(r);
}
END_TEST

START_TEST(get_height_null)
{
    ck_assert_int_eq(room_get_height(NULL), 0);
}
END_TEST

START_TEST(set_grid_ok)
{
    Room *r = room_create(1, "R", 2, 2);

    bool *grid = malloc(sizeof(bool) * 4);
    grid[0] = true; grid[1] = false;
    grid[2] = true; grid[3] = true;

    ck_assert_int_eq(room_set_floor_grid(r, grid), OK);
    ck_assert_ptr_eq(r->floor_grid, grid);

    room_destroy(r);
}
END_TEST

START_TEST(set_grid_null_room)
{
    bool *grid = malloc(sizeof(bool) * 1);
    ck_assert_int_eq(room_set_floor_grid(NULL, grid), INVALID_ARGUMENT);
    free(grid);
}
END_TEST

START_TEST(set_grid_overwrite)
{
    Room *r = room_create(1, "R", 2, 2);

    bool *g1 = malloc(sizeof(bool) * 4);
    bool *g2 = malloc(sizeof(bool) * 4);

    ck_assert_int_eq(room_set_floor_grid(r, g1), OK);
    ck_assert_int_eq(room_set_floor_grid(r, g2), OK);
    ck_assert_ptr_eq(r->floor_grid, g2);

    room_destroy(r);
}
END_TEST

START_TEST(set_portals_ok)
{
    Room *r = room_create(1, "R", 2, 2);

    Portal *p = malloc(sizeof(Portal) * 1);
    p[0].id = 1;
    p[0].name = malloc(3);
    strcpy(p[0].name, "P1");
    p[0].x = 0;
    p[0].y = 0;
    p[0].target_room_id = 2;

    ck_assert_int_eq(room_set_portals(r, p, 1), OK);
    ck_assert_ptr_eq(r->portals, p);
    ck_assert_int_eq(r->portal_count, 1);

    room_destroy(r);
}
END_TEST

START_TEST(set_portals_bad)
{
    Room *r = room_create(1, "R", 2, 2);
    ck_assert_int_eq(room_set_portals(r, NULL, 1), INVALID_ARGUMENT);
    ck_assert_int_eq(room_set_portals(NULL, NULL, 0), INVALID_ARGUMENT);
    room_destroy(r);
}
END_TEST

START_TEST(set_treasures_ok)
{
    Room *r = room_create(1, "R", 2, 2);

    Treasure *t = malloc(sizeof(Treasure) * 1);
    t[0].id = 10;
    t[0].name = malloc(3);
    strcpy(t[0].name, "T1");
    t[0].x = 1;
    t[0].y = 1;
    t[0].collected = false;

    ck_assert_int_eq(room_set_treasures(r, t, 1), OK);
    ck_assert_ptr_eq(r->treasures, t);
    ck_assert_int_eq(r->treasure_count, 1);

    room_destroy(r);
}
END_TEST

START_TEST(set_treasures_bad)
{
    Room *r = room_create(1, "R", 2, 2);
    ck_assert_int_eq(room_set_treasures(r, NULL, 1), INVALID_ARGUMENT);
    ck_assert_int_eq(room_set_treasures(NULL, NULL, 0), INVALID_ARGUMENT);
    room_destroy(r);
}
END_TEST

START_TEST(place_treasure_ok)
{
    Room *r = room_create(1, "R", 2, 2);

    Treasure tr;
    tr.id = 1;

    tr.name = malloc(5); 
    ck_assert_ptr_nonnull(tr.name);
    strcpy(tr.name, "Gold");

    tr.x = 0;
    tr.y = 0;
    tr.collected = false;

    ck_assert_int_eq(room_place_treasure(r, &tr), OK);
    ck_assert_int_eq(r->treasure_count, 1);
    ck_assert_ptr_nonnull(r->treasures);
    ck_assert_int_eq(r->treasures[0].id, 1);
    ck_assert_str_eq(r->treasures[0].name, "Gold");

    room_destroy(r);
}
END_TEST

START_TEST(place_treasure_bad)
{
    Room *r = room_create(1, "R", 2, 2);

    Treasure tr;
    tr.id = 1;
    tr.name = NULL; 

    ck_assert_int_eq(room_place_treasure(NULL, &tr), INVALID_ARGUMENT);
    ck_assert_int_eq(room_place_treasure(r, NULL), INVALID_ARGUMENT);

    room_destroy(r);
}
END_TEST

START_TEST(get_treasure_at_ok)
{
    Room *r = room_create(1, "R", 3, 3);

    Treasure *t = malloc(sizeof(Treasure) * 1);
    t[0].id = 7;
    t[0].name = malloc(2);
    strcpy(t[0].name, "T");
    t[0].x = 1;
    t[0].y = 2;
    t[0].collected = false;

    room_set_treasures(r, t, 1);

    ck_assert_int_eq(room_get_treasure_at(r, 1, 2), 7);
    ck_assert_int_eq(room_get_treasure_at(r, 0, 0), -1);

    room_destroy(r);
}
END_TEST

START_TEST(get_treasure_at_null)
{
    ck_assert_int_eq(room_get_treasure_at(NULL, 0, 0), -1);
}
END_TEST

START_TEST(get_portal_dest_ok)
{
    Room *r = room_create(1, "R", 3, 3);

    Portal *p = malloc(sizeof(Portal) * 1);
    p[0].id = 1;
    p[0].name = malloc(2);
    strcpy(p[0].name, "P");
    p[0].x = 1;
    p[0].y = 2;
    p[0].target_room_id = 99;

    room_set_portals(r, p, 1);

    ck_assert_int_eq(room_get_portal_destination(r, 1, 2), 99);
    ck_assert_int_eq(room_get_portal_destination(r, 0, 0), -1);

    room_destroy(r);
}
END_TEST

START_TEST(get_portal_dest_null)
{
    ck_assert_int_eq(room_get_portal_destination(NULL, 0, 0), -1);
}
END_TEST

START_TEST(is_walkable_ok)
{
    Room *r = room_create(1, "R", 2, 2);

    bool *grid = malloc(sizeof(bool) * 4);
    grid[0] = true;
    grid[1] = false;
    grid[2] = true;
    grid[3] = true;

    room_set_floor_grid(r, grid);

    ck_assert(room_is_walkable(r, 0, 0) == true);
    ck_assert(room_is_walkable(r, 1, 0) == false);
    ck_assert(room_is_walkable(r, 9, 9) == false);

    room_destroy(r);
}
END_TEST

START_TEST(is_walkable_null)
{
    ck_assert(room_is_walkable(NULL, 0, 0) == false);
}
END_TEST

START_TEST(classify_ok)
{
    Room *r = room_create(1, "R", 2, 2);

    bool *grid = malloc(sizeof(bool) * 4);
    for (int i = 0; i < 4; i++) grid[i] = true;
    grid[0] = false;
    room_set_floor_grid(r, grid);

    Treasure *t = malloc(sizeof(Treasure) * 1);
    t[0].id = 55;
    t[0].name = malloc(2);
    strcpy(t[0].name, "T");
    t[0].x = 1;
    t[0].y = 1;
    t[0].collected = false;
    room_set_treasures(r, t, 1);

    Portal *p = malloc(sizeof(Portal) * 1);
    p[0].id = 1;
    p[0].name = malloc(2);
    strcpy(p[0].name, "P");
    p[0].x = 0;
    p[0].y = 1;
    p[0].target_room_id = 88;
    room_set_portals(r, p, 1);

    int id = -1;
    ck_assert_int_eq(room_classify_tile(r, 0, 0, &id), ROOM_TILE_WALL);
    ck_assert_int_eq(room_classify_tile(r, 0, 1, &id), ROOM_TILE_PORTAL);
    ck_assert_int_eq(id, 88);
    ck_assert_int_eq(room_classify_tile(r, 1, 1, &id), ROOM_TILE_TREASURE);
    ck_assert_int_eq(id, 55);
    ck_assert_int_eq(room_classify_tile(r, 1, 0, NULL), ROOM_TILE_FLOOR);

    room_destroy(r);
}
END_TEST

START_TEST(classify_bad)
{
    ck_assert_int_eq(room_classify_tile(NULL, 0, 0, NULL), ROOM_TILE_INVALID);
}
END_TEST

START_TEST(render_ok)
{
    Room *r = room_create(1, "R", 2, 2);

    bool *grid = malloc(sizeof(bool) * 4);
    grid[0] = true;
    grid[1] = false;
    grid[2] = false;
    grid[3] = true;
    room_set_floor_grid(r, grid);

    Portal *p = malloc(sizeof(Portal) * 1);
    p[0].id = 1;
    p[0].name = malloc(2);
    strcpy(p[0].name, "P");
    p[0].x = 1;
    p[0].y = 1;
    p[0].target_room_id = 9;
    room_set_portals(r, p, 1);

    Treasure *t = malloc(sizeof(Treasure) * 1);
    t[0].id = 2;
    t[0].name = malloc(2);
    strcpy(t[0].name, "T");
    t[0].x = 1;
    t[0].y = 1;
    t[0].collected = false;
    room_set_treasures(r, t, 1);

    Charset cs;
    cs.wall = '#';
    cs.floor = '.';
    cs.player = '@';
    cs.treasure = '$';
    cs.portal = 'O';
    cs.pushable = '?';

    char buf[4];
    ck_assert_int_eq(room_render(r, &cs, buf, 2, 2), OK);
    ck_assert_int_eq(buf[0], '.');
    ck_assert_int_eq(buf[1], '#');
    ck_assert_int_eq(buf[2], '#');
    ck_assert_int_eq(buf[3], 'O');

    room_destroy(r);
}
END_TEST

START_TEST(render_bad)
{
    Room *r = room_create(1, "R", 2, 2);

    Charset cs;
    cs.wall = '#';
    cs.floor = '.';
    cs.player = '@';
    cs.treasure = '$';
    cs.portal = 'O';
    cs.pushable = '?';

    char buf[4];

    ck_assert_int_eq(room_render(NULL, &cs, buf, 2, 2), INVALID_ARGUMENT);
    ck_assert_int_eq(room_render(r, NULL, buf, 2, 2), INVALID_ARGUMENT);
    ck_assert_int_eq(room_render(r, &cs, NULL, 2, 2), INVALID_ARGUMENT);
    ck_assert_int_eq(room_render(r, &cs, buf, 1, 2), INVALID_ARGUMENT);

    room_destroy(r);
}
END_TEST

START_TEST(start_pos_ok)
{
    Room *r = room_create(1, "R", 3, 3);

    bool *grid = malloc(sizeof(bool) * 9);
    for (int i = 0; i < 9; i++) grid[i] = true;
    room_set_floor_grid(r, grid);

    Portal *p = malloc(sizeof(Portal) * 1);
    p[0].id = 1;
    p[0].name = malloc(2);
    strcpy(p[0].name, "P");
    p[0].x = 2;
    p[0].y = 1;
    p[0].target_room_id = 9;
    room_set_portals(r, p, 1);

    int x = -1, y = -1;
    ck_assert_int_eq(room_get_start_position(r, &x, &y), OK);
    ck_assert_int_eq(x, 2);
    ck_assert_int_eq(y, 1);

    room_destroy(r);
}
END_TEST

START_TEST(start_pos_bad)
{
    Room *r = room_create(1, "R", 2, 2);
    int x = 0, y = 0;

    ck_assert_int_eq(room_get_start_position(NULL, &x, &y), INVALID_ARGUMENT);
    ck_assert_int_eq(room_get_start_position(r, NULL, &y), INVALID_ARGUMENT);
    ck_assert_int_eq(room_get_start_position(r, &x, NULL), INVALID_ARGUMENT);

    room_destroy(r);
}
END_TEST

START_TEST(destroy_null_safe)
{
    room_destroy(NULL);
}
END_TEST

Suite *room_suite(void)
{
    Suite *s = suite_create("room");
    TCase *tc = tcase_create("room");

    tcase_add_test(tc, create_ok);
    tcase_add_test(tc, create_null_name);

    tcase_add_test(tc, get_width_ok);
    tcase_add_test(tc, get_width_null);
    tcase_add_test(tc, get_height_ok);
    tcase_add_test(tc, get_height_null);

    tcase_add_test(tc, set_grid_ok);
    tcase_add_test(tc, set_grid_null_room);
    tcase_add_test(tc, set_grid_overwrite);

    tcase_add_test(tc, set_portals_ok);
    tcase_add_test(tc, set_portals_bad);

    tcase_add_test(tc, set_treasures_ok);
    tcase_add_test(tc, set_treasures_bad);

    tcase_add_test(tc, place_treasure_ok);
    tcase_add_test(tc, place_treasure_bad);

    tcase_add_test(tc, get_treasure_at_ok);
    tcase_add_test(tc, get_treasure_at_null);

    tcase_add_test(tc, get_portal_dest_ok);
    tcase_add_test(tc, get_portal_dest_null);

    tcase_add_test(tc, is_walkable_ok);
    tcase_add_test(tc, is_walkable_null);

    tcase_add_test(tc, classify_ok);
    tcase_add_test(tc, classify_bad);

    tcase_add_test(tc, render_ok);
    tcase_add_test(tc, render_bad);

    tcase_add_test(tc, start_pos_ok);
    tcase_add_test(tc, start_pos_bad);

    tcase_add_test(tc, destroy_null_safe);

    suite_add_tcase(s, tc);
    return s;
}