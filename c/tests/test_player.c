#include <check.h>
#include <stdlib.h>
#include "player.h"


START_TEST(player_create_basic)
{
    Player *p = NULL;

    ck_assert_int_eq(player_create(5, 2, 3, &p), OK);
    ck_assert_ptr_nonnull(p);

    ck_assert_int_eq(p->room_id, 5);
    ck_assert_int_eq(p->x, 2);
    ck_assert_int_eq(p->y, 3);

    free(p);
}
END_TEST

START_TEST(player_create_null_out)
{
    ck_assert_int_eq(player_create(1, 0, 0, NULL), INVALID_ARGUMENT);
}
END_TEST

START_TEST(player_create_zero_position)
{
    Player *p = NULL;

    ck_assert_int_eq(player_create(0, 0, 0, &p), OK);
    ck_assert_ptr_nonnull(p);

    ck_assert_int_eq(p->room_id, 0);
    ck_assert_int_eq(p->x, 0);
    ck_assert_int_eq(p->y, 0);

    free(p);
}
END_TEST

START_TEST(player_destroy_basic)
{
    Player *p = NULL;
    ck_assert_int_eq(player_create(1, 2, 3, &p), OK);

    player_destroy(p);
}
END_TEST

START_TEST(player_destroy_null_safe)
{
    player_destroy(NULL);
}
END_TEST

START_TEST(player_get_room_basic)
{
    Player *p = NULL;
    ck_assert_int_eq(player_create(7, 1, 1, &p), OK);

    ck_assert_int_eq(player_get_room(p), 7);

    player_destroy(p);
}
END_TEST

START_TEST(player_get_room_null)
{
    ck_assert_int_eq(player_get_room(NULL), -1);
}
END_TEST

START_TEST(player_get_position_basic)
{
    Player *p = NULL;
    ck_assert_int_eq(player_create(1, 4, 5, &p), OK);

    int x = -1, y = -1;
    ck_assert_int_eq(player_get_position(p, &x, &y), OK);

    ck_assert_int_eq(x, 4);
    ck_assert_int_eq(y, 5);

    player_destroy(p);
}
END_TEST

START_TEST(player_get_position_bad_args)
{
    Player *p = NULL;
    ck_assert_int_eq(player_create(1, 0, 0, &p), OK);

    int x, y;

    ck_assert_int_eq(player_get_position(NULL, &x, &y), INVALID_ARGUMENT);
    ck_assert_int_eq(player_get_position(p, NULL, &y), INVALID_ARGUMENT);
    ck_assert_int_eq(player_get_position(p, &x, NULL), INVALID_ARGUMENT);

    player_destroy(p);
}
END_TEST

START_TEST(player_set_position_basic)
{
    Player *p = NULL;
    ck_assert_int_eq(player_create(1, 0, 0, &p), OK);

    ck_assert_int_eq(player_set_position(p, 3, 4), OK);

    int x = -1, y = -1;
    ck_assert_int_eq(player_get_position(p, &x, &y), OK);
    ck_assert_int_eq(x, 3);
    ck_assert_int_eq(y, 4);

    player_destroy(p);
}
END_TEST

START_TEST(player_set_position_null_player)
{
    ck_assert_int_eq(player_set_position(NULL, 1, 2), INVALID_ARGUMENT);
}
END_TEST

START_TEST(player_move_to_room_basic)
{
    Player *p = NULL;
    ck_assert_int_eq(player_create(1, 2, 3, &p), OK);

    ck_assert_int_eq(player_move_to_room(p, 9), OK);
    ck_assert_int_eq(player_get_room(p), 9);

    player_destroy(p);
}
END_TEST

START_TEST(player_move_to_room_null_player)
{
    ck_assert_int_eq(player_move_to_room(NULL, 5), INVALID_ARGUMENT);
}
END_TEST

START_TEST(player_reset_to_start_basic)
{
    Player *p = NULL;
    ck_assert_int_eq(player_create(5, 1, 1, &p), OK);

    ck_assert_int_eq(player_reset_to_start(p, 9, 4, 3), OK);

    ck_assert_int_eq(player_get_room(p), 9);

    int x = -1, y = -1;
    ck_assert_int_eq(player_get_position(p, &x, &y), OK);
    ck_assert_int_eq(x, 4);
    ck_assert_int_eq(y, 3);

    player_destroy(p);
}
END_TEST

START_TEST(player_reset_to_start_null_player)
{
    ck_assert_int_eq(player_reset_to_start(NULL, 1, 0, 0), INVALID_ARGUMENT);
}
END_TEST


Suite *player_suite(void) {
    Suite *s = suite_create("player");
    TCase *tc = tcase_create("stub");

    tcase_add_test(tc, player_create_basic);
    tcase_add_test(tc, player_create_null_out);
    tcase_add_test(tc, player_create_zero_position);

    tcase_add_test(tc, player_destroy_basic);
    tcase_add_test(tc, player_destroy_null_safe);

    tcase_add_test(tc, player_get_room_basic);
    tcase_add_test(tc, player_get_room_null);

    tcase_add_test(tc, player_get_position_basic);
    tcase_add_test(tc, player_get_position_bad_args);

    tcase_add_test(tc, player_set_position_basic);
    tcase_add_test(tc, player_set_position_null_player);

    tcase_add_test(tc, player_move_to_room_basic);
    tcase_add_test(tc, player_move_to_room_null_player);

    tcase_add_test(tc, player_reset_to_start_basic);
    tcase_add_test(tc, player_reset_to_start_null_player);

    suite_add_tcase(s, tc);
    return s;
}
