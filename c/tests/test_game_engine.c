#include <check.h>
#include <stdlib.h>
#include <string.h>
#include "game_engine.h"
#include "player.h"

START_TEST(engine_create_null_out)
{
    ck_assert_int_eq(game_engine_create("../assets/starter.ini", NULL), INVALID_ARGUMENT);
}
END_TEST

START_TEST(engine_create_null_config)
{
    GameEngine *e = (GameEngine *)0x1;
    ck_assert_int_eq(game_engine_create(NULL, &e), INVALID_ARGUMENT);
}
END_TEST

START_TEST(engine_create_bad_config)
{
    GameEngine *e = NULL;

    Status st = game_engine_create("definitely_not_real.ini", &e);
    ck_assert(st == WL_ERR_DATAGEN || st == WL_ERR_CONFIG);
    ck_assert_ptr_null(e);
}
END_TEST

START_TEST(engine_create_success_basic)
{
    GameEngine *e = NULL;

    ck_assert_int_eq(game_engine_create("../assets/starter.ini", &e), OK);
    ck_assert_ptr_nonnull(e);
}
END_TEST

START_TEST(engine_create_sets_player_and_graph)
{
    GameEngine *e = NULL;

    ck_assert_int_eq(game_engine_create("../assets/starter.ini", &e), OK);

    ck_assert_ptr_nonnull(e->graph);
    ck_assert_ptr_nonnull(e->player);
}
END_TEST


START_TEST(engine_destroy_basic)
{
    GameEngine *e = NULL;

    ck_assert_int_eq(game_engine_create("../assets/starter.ini", &e), OK);
    ck_assert_ptr_nonnull(e);

    game_engine_destroy(e);
}
END_TEST

START_TEST(engine_destroy_null_safe)
{
    game_engine_destroy(NULL);
}
END_TEST


START_TEST(get_player_basic)
{
    GameEngine *e = NULL;

    ck_assert_int_eq(game_engine_create("../assets/starter.ini", &e), OK);

    const Player *p = game_engine_get_player(e);
    ck_assert_ptr_nonnull(p);

    game_engine_destroy(e);
}
END_TEST

START_TEST(get_player_null_engine)
{
    ck_assert_ptr_null(game_engine_get_player(NULL));
}
END_TEST

START_TEST(get_player_same_pointer_each_time)
{
    GameEngine *e = NULL;

    ck_assert_int_eq(game_engine_create("../assets/starter.ini", &e), OK);

    const Player *p1 = game_engine_get_player(e);
    const Player *p2 = game_engine_get_player(e);

    ck_assert_ptr_nonnull(p1);
    ck_assert_ptr_eq(p1, p2);

    game_engine_destroy(e);
}
END_TEST

START_TEST(move_player_null_engine)
{
    ck_assert_int_eq(game_engine_move_player(NULL, DIR_NORTH), INVALID_ARGUMENT);
}
END_TEST

START_TEST(move_player_bad_dir)
{
    GameEngine *e = NULL;
    ck_assert_int_eq(game_engine_create("../assets/starter.ini", &e), OK);

    Status st = game_engine_move_player(e, (Direction)999);
    ck_assert(st == INVALID_ARGUMENT || st == INTERNAL_ERROR);

    game_engine_destroy(e);
}
END_TEST

START_TEST(move_player_impassable_doesnt_move)
{
    GameEngine *e = NULL;
    ck_assert_int_eq(game_engine_create("../assets/starter.ini", &e), OK);

    const Player *p = game_engine_get_player(e);
    ck_assert_ptr_nonnull(p);

    int x0 = -1, y0 = -1;
    ck_assert_int_eq(player_get_position(p, &x0, &y0), OK);

    Status st = game_engine_move_player(e, DIR_NORTH);
    if (st == ROOM_IMPASSABLE) {
        int x1 = -2, y1 = -2;
        ck_assert_int_eq(player_get_position(game_engine_get_player(e), &x1, &y1), OK);
        ck_assert_int_eq(x1, x0);
        ck_assert_int_eq(y1, y0);
    }

    game_engine_destroy(e);
}
END_TEST

START_TEST(move_player_some_direction_moves)
{
    GameEngine *e = NULL;
    ck_assert_int_eq(game_engine_create("../assets/starter.ini", &e), OK);

    int x0 = -1, y0 = -1;
    ck_assert_int_eq(player_get_position(game_engine_get_player(e), &x0, &y0), OK);

    Status st = ROOM_IMPASSABLE;

    st = game_engine_move_player(e, DIR_NORTH);
    if (st != OK) st = game_engine_move_player(e, DIR_SOUTH);
    if (st != OK) st = game_engine_move_player(e, DIR_WEST);
    if (st != OK) st = game_engine_move_player(e, DIR_EAST);

    ck_assert_int_eq(st, OK);

    int x1 = -2, y1 = -2;
    ck_assert_int_eq(player_get_position(game_engine_get_player(e), &x1, &y1), OK);
    ck_assert(x1 != x0 || y1 != y0);

    game_engine_destroy(e);
}
END_TEST


START_TEST(get_room_count_null_engine)
{
    int n = -1;
    ck_assert_int_eq(game_engine_get_room_count(NULL, &n), INVALID_ARGUMENT);
}
END_TEST

START_TEST(get_room_count_null_out)
{
    GameEngine *e = NULL;
    ck_assert_int_eq(game_engine_create("../assets/starter.ini", &e), OK);

    ck_assert_int_eq(game_engine_get_room_count(e, NULL), NULL_POINTER);

    game_engine_destroy(e);
}
END_TEST

START_TEST(get_room_count_basic)
{
    GameEngine *e = NULL;
    ck_assert_int_eq(game_engine_create("../assets/starter.ini", &e), OK);

    int n = -1;
    ck_assert_int_eq(game_engine_get_room_count(e, &n), OK);
    ck_assert_int_gt(n, 0);

    game_engine_destroy(e);
}
END_TEST


START_TEST(get_room_dims_null_engine)
{
    int w = -1, h = -1;
    ck_assert_int_eq(game_engine_get_room_dimensions(NULL, &w, &h), INVALID_ARGUMENT);
}
END_TEST

START_TEST(get_room_dims_null_width)
{
    GameEngine *e = NULL;
    ck_assert_int_eq(game_engine_create("../assets/starter.ini", &e), OK);

    int h = -1;
    ck_assert_int_eq(game_engine_get_room_dimensions(e, NULL, &h), NULL_POINTER);

    game_engine_destroy(e);
}
END_TEST

START_TEST(get_room_dims_null_height)
{
    GameEngine *e = NULL;
    ck_assert_int_eq(game_engine_create("../assets/starter.ini", &e), OK);

    int w = -1;
    ck_assert_int_eq(game_engine_get_room_dimensions(e, &w, NULL), NULL_POINTER);

    game_engine_destroy(e);
}
END_TEST

START_TEST(get_room_dims_basic)
{
    GameEngine *e = NULL;
    ck_assert_int_eq(game_engine_create("../assets/starter.ini", &e), OK);

    int w = -1, h = -1;
    ck_assert_int_eq(game_engine_get_room_dimensions(e, &w, &h), OK);
    ck_assert_int_gt(w, 0);
    ck_assert_int_gt(h, 0);

    game_engine_destroy(e);
}
END_TEST


START_TEST(reset_null_engine)
{
    ck_assert_int_eq(game_engine_reset(NULL), INVALID_ARGUMENT);
}
END_TEST



START_TEST(reset_restores_start_position)
{
    GameEngine *e = NULL;
    ck_assert_int_eq(game_engine_create("../assets/starter.ini", &e), OK);

    const Player *p0 = game_engine_get_player(e);
    ck_assert_ptr_nonnull(p0);

    int x0 = -1, y0 = -1;
    ck_assert_int_eq(player_get_position(p0, &x0, &y0), OK);

    game_engine_move_player(e, DIR_EAST);
    game_engine_move_player(e, DIR_SOUTH);

    ck_assert_int_eq(game_engine_reset(e), OK);

    const Player *p1 = game_engine_get_player(e);
    ck_assert_ptr_nonnull(p1);

    int x1 = -2, y1 = -2;
    ck_assert_int_eq(player_get_position(p1, &x1, &y1), OK);

    ck_assert_int_eq(x1, x0);
    ck_assert_int_eq(y1, y0);

    game_engine_destroy(e);
}
END_TEST

START_TEST(reset_restores_start_room)
{
    GameEngine *e = NULL;
    ck_assert_int_eq(game_engine_create("../assets/starter.ini", &e), OK);

    const Player *p0 = game_engine_get_player(e);
    ck_assert_ptr_nonnull(p0);

    int r0 = player_get_room(p0);

    game_engine_move_player(e, DIR_EAST);
    game_engine_move_player(e, DIR_SOUTH);
    game_engine_move_player(e, DIR_WEST);
    game_engine_move_player(e, DIR_NORTH);

    ck_assert_int_eq(game_engine_reset(e), OK);

    const Player *p1 = game_engine_get_player(e);
    ck_assert_ptr_nonnull(p1);

    ck_assert_int_eq(player_get_room(p1), r0);

    game_engine_destroy(e);
}
END_TEST

START_TEST(render_null_engine)
{
    char *s = (char *)0x1;
    ck_assert_int_eq(game_engine_render_current_room(NULL, &s), INVALID_ARGUMENT);
}
END_TEST

START_TEST(render_null_out)
{
    GameEngine *e = NULL;
    ck_assert_int_eq(game_engine_create("../assets/starter.ini", &e), OK);

    ck_assert_int_eq(game_engine_render_current_room(e, NULL), INVALID_ARGUMENT);

    game_engine_destroy(e);
}
END_TEST



START_TEST(render_changes_after_move)
{
    GameEngine *e = NULL;
    ck_assert_int_eq(game_engine_create("../assets/starter.ini", &e), OK);

    char *a = NULL;
    char *b = NULL;

    ck_assert_int_eq(game_engine_render_current_room(e, &a), OK);
    ck_assert_ptr_nonnull(a);

    game_engine_move_player(e, DIR_EAST);

    ck_assert_int_eq(game_engine_render_current_room(e, &b), OK);
    ck_assert_ptr_nonnull(b);

    ck_assert_str_ne(a, b);

    free(a);
    free(b);
    game_engine_destroy(e);
}
END_TEST

START_TEST(render_room_null_engine)
{
    char *s = (char *)0x1;
    ck_assert_int_eq(game_engine_render_room(NULL, 0, &s), INVALID_ARGUMENT);
}
END_TEST

START_TEST(render_room_null_out)
{
    GameEngine *e = NULL;
    ck_assert_int_eq(game_engine_create("../assets/starter.ini", &e), OK);

    ck_assert_int_eq(game_engine_render_room(e, 0, NULL), NULL_POINTER);

    game_engine_destroy(e);
}
END_TEST

START_TEST(render_room_invalid_room_id)
{
    GameEngine *e = NULL;
    ck_assert_int_eq(game_engine_create("../assets/starter.ini", &e), OK);

    char *s = NULL;
    ck_assert_int_eq(game_engine_render_room(e, -1, &s), GE_NO_SUCH_ROOM);
    ck_assert_ptr_null(s);

    game_engine_destroy(e);
}
END_TEST

START_TEST(render_room_success_allocates_string)
{
    GameEngine *e = NULL;
    ck_assert_int_eq(game_engine_create("../assets/starter.ini", &e), OK);

    int count = 0;
    ck_assert_int_eq(game_engine_get_room_count(e, &count), OK);
    ck_assert_int_gt(count, 0);

    char *s = NULL;
    ck_assert_int_eq(game_engine_render_room(e, 0, &s), OK);
    ck_assert_ptr_nonnull(s);

    ck_assert_ptr_nonnull(strchr(s, '\n'));
    ck_assert_ptr_null(strchr(s, '@'));

    free(s);
    game_engine_destroy(e);
}
END_TEST


START_TEST(get_room_ids_null_engine)
{
    int *ids = (int *)0x1;
    int n = -1;

    ck_assert_int_eq(game_engine_get_room_ids(NULL, &ids, &n), INVALID_ARGUMENT);
}
END_TEST

START_TEST(get_room_ids_null_ids_out)
{
    GameEngine *e = NULL;
    ck_assert_int_eq(game_engine_create("../assets/starter.ini", &e), OK);

    int n = -1;
    ck_assert_int_eq(game_engine_get_room_ids(e, NULL, &n), NULL_POINTER);

    game_engine_destroy(e);
}
END_TEST

START_TEST(get_room_ids_null_count_out)
{
    GameEngine *e = NULL;
    ck_assert_int_eq(game_engine_create("../assets/starter.ini", &e), OK);

    int *ids = NULL;
    ck_assert_int_eq(game_engine_get_room_ids(e, &ids, NULL), NULL_POINTER);
    ck_assert_ptr_null(ids);

    game_engine_destroy(e);
}
END_TEST

START_TEST(get_room_ids_count_matches_room_count)
{
    GameEngine *e = NULL;
    ck_assert_int_eq(game_engine_create("../assets/starter.ini", &e), OK);

    int room_count = -1;
    ck_assert_int_eq(game_engine_get_room_count(e, &room_count), OK);
    ck_assert_int_gt(room_count, 0);

    int *ids = NULL;
    int n = 0;

    ck_assert_int_eq(game_engine_get_room_ids(e, &ids, &n), OK);
    ck_assert_int_eq(n, room_count);

    free(ids);
    game_engine_destroy(e);
}
END_TEST

START_TEST(get_room_ids_two_calls_return_independent_arrays)
{
    GameEngine *e = NULL;
    ck_assert_int_eq(game_engine_create("../assets/starter.ini", &e), OK);

    int *a = NULL;
    int na = 0;
    ck_assert_int_eq(game_engine_get_room_ids(e, &a, &na), OK);

    int *b = NULL;
    int nb = 0;
    ck_assert_int_eq(game_engine_get_room_ids(e, &b, &nb), OK);

    ck_assert_int_eq(na, nb);
    ck_assert_ptr_ne(a, b);

    free(a);
    free(b);
    game_engine_destroy(e);
}
END_TEST




Suite *game_engine_suite(void) {
    Suite *s = suite_create("game_engine");
    TCase *tc = tcase_create("stub");

    tcase_add_test(tc, engine_create_null_out);
    tcase_add_test(tc, engine_create_null_config);
    tcase_add_test(tc, engine_create_bad_config);
    tcase_add_test(tc, engine_create_success_basic);
    tcase_add_test(tc, engine_create_sets_player_and_graph);

    tcase_add_test(tc, engine_destroy_basic);
    tcase_add_test(tc, engine_destroy_null_safe);

    tcase_add_test(tc, get_player_basic);
    tcase_add_test(tc, get_player_null_engine);
    tcase_add_test(tc, get_player_same_pointer_each_time);

    tcase_add_test(tc, move_player_null_engine);
    tcase_add_test(tc, move_player_bad_dir);
    tcase_add_test(tc, move_player_impassable_doesnt_move);
    tcase_add_test(tc, move_player_some_direction_moves);

    tcase_add_test(tc, get_room_count_null_engine);
    tcase_add_test(tc, get_room_count_null_out);
    tcase_add_test(tc, get_room_count_basic);

    tcase_add_test(tc, get_room_dims_null_engine);
    tcase_add_test(tc, get_room_dims_null_width);
    tcase_add_test(tc, get_room_dims_null_height);
    tcase_add_test(tc, get_room_dims_basic);

    tcase_add_test(tc, reset_null_engine);    
    tcase_add_test(tc, reset_restores_start_position);
    tcase_add_test(tc, reset_restores_start_room);

    tcase_add_test(tc, render_null_engine);
    tcase_add_test(tc, render_null_out);
    tcase_add_test(tc, render_changes_after_move);

    tcase_add_test(tc, render_room_null_engine);
    tcase_add_test(tc, render_room_null_out);
    tcase_add_test(tc, render_room_invalid_room_id);
    tcase_add_test(tc, render_room_success_allocates_string);

    tcase_add_test(tc, get_room_ids_null_engine);
    tcase_add_test(tc, get_room_ids_null_ids_out);
    tcase_add_test(tc, get_room_ids_null_count_out);
    tcase_add_test(tc, get_room_ids_count_matches_room_count);
    tcase_add_test(tc, get_room_ids_two_calls_return_independent_arrays);



    suite_add_tcase(s, tc);
    return s;
}
