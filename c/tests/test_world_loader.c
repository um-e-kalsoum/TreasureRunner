#include <check.h>
#include "world_loader.h"
#include "room.h"

START_TEST(load_world_null_graph_out)
{
    Room *first = (Room *)0x1;
    int n = -1;
    Charset cs;

    ck_assert_int_eq(loader_load_world("whatever.ini", NULL, &first, &n, &cs), INVALID_ARGUMENT);
}
END_TEST

START_TEST(load_world_null_first_room_out)
{
    Graph *g = (Graph *)0x1;
    int n = -1;
    Charset cs;

    ck_assert_int_eq(loader_load_world("whatever.ini", &g, NULL, &n, &cs), INVALID_ARGUMENT);
}
END_TEST

START_TEST(load_world_null_num_rooms_out)
{
    Graph *g = (Graph *)0x1;
    Room *first = (Room *)0x1;
    Charset cs;

    ck_assert_int_eq(loader_load_world("whatever.ini", &g, &first, NULL, &cs), INVALID_ARGUMENT);
}
END_TEST

START_TEST(load_world_null_charset_out)
{
    Graph *g = (Graph *)0x1;
    Room *first = (Room *)0x1;
    int n = -1;

    ck_assert_int_eq(loader_load_world("whatever.ini", &g, &first, &n, NULL), INVALID_ARGUMENT);
}
END_TEST

START_TEST(load_world_null_config)
{
    Graph *g = NULL;
    Room *first = NULL;
    int n = -1;
    Charset cs;

    ck_assert_int_eq(loader_load_world(NULL, &g, &first, &n, &cs), INVALID_ARGUMENT);
    ck_assert_ptr_null(g);
    ck_assert_ptr_null(first);
}
END_TEST

START_TEST(load_world_empty_config)
{
    Graph *g = NULL;
    Room *first = NULL;
    int n = -1;
    Charset cs;

    Status st = loader_load_world("", &g, &first, &n, &cs);
    ck_assert(st == WL_ERR_CONFIG || st == WL_ERR_DATAGEN);
    ck_assert_ptr_null(g);
    ck_assert_ptr_null(first);
}
END_TEST

START_TEST(load_world_bad_path)
{
    Graph *g = NULL;
    Room *first = NULL;
    int n = -1;
    Charset cs;

    Status st = loader_load_world("whatever.ini", &g, &first, &n, &cs);
    ck_assert_int_eq(st, WL_ERR_CONFIG);
    ck_assert_ptr_null(g);
    ck_assert_ptr_null(first);
}
END_TEST

START_TEST(load_world_bad_extension)
{
    Graph *g = NULL;
    Room *first = NULL;
    int n = -1;
    Charset cs;

    Status st = loader_load_world("whatever.txt", &g, &first, &n, &cs);
    ck_assert(st == WL_ERR_CONFIG || st == WL_ERR_DATAGEN);
    ck_assert_ptr_null(g);
    ck_assert_ptr_null(first);
}
END_TEST

START_TEST(load_world_fail_keeps_nulls_null)
{
    Graph *g = NULL;
    Room *first = NULL;
    int n = 0;
    Charset cs;

    Status st = loader_load_world("whatever.ini", &g, &first, &n, &cs);

    ck_assert_int_eq(st, WL_ERR_CONFIG);
    ck_assert_ptr_null(g);
    ck_assert_ptr_null(first);
}
END_TEST

START_TEST(load_world_path_with_spaces)
{
    Graph *g = NULL;
    Room *first = NULL;
    int n = -1;
    Charset cs;

    Status st = loader_load_world("bad path/with spaces/config.ini", &g, &first, &n, &cs);
    ck_assert(st == WL_ERR_CONFIG || st == WL_ERR_DATAGEN);
    ck_assert_ptr_null(g);
    ck_assert_ptr_null(first);
}
END_TEST

START_TEST(load_world_two_calls_same_bad_config)
{
    Graph *g = NULL;
    Room *first = NULL;
    int n = -1;
    Charset cs;

    Status a = loader_load_world("whatever.ini", &g, &first, &n, &cs);
    ck_assert(a == WL_ERR_CONFIG || a == WL_ERR_DATAGEN);

    Status b = loader_load_world("whatever.ini", &g, &first, &n, &cs);
    ck_assert(b == WL_ERR_CONFIG || b == WL_ERR_DATAGEN);

    ck_assert_ptr_null(g);
    ck_assert_ptr_null(first);
}
END_TEST

START_TEST(load_world_success_basic)
{
    Graph *g = NULL;
    Room *first = NULL;
    int n = -1;
    Charset cs;

    ck_assert_int_eq(
        loader_load_world("../assets/starter.ini", &g, &first, &n, &cs),
        OK
    );

    ck_assert_ptr_nonnull(g);
    ck_assert_ptr_nonnull(first);
    ck_assert_int_gt(n, 0);
}
END_TEST


START_TEST(load_world_success_charset_loaded)
{
    Graph *g = NULL;
    Room *first = NULL;
    int n = 0;
    Charset cs;

    ck_assert_int_eq(
        loader_load_world("../assets/starter.ini", &g, &first, &n, &cs),
        OK
    );

    ck_assert(cs.wall != '\0');
    ck_assert(cs.floor != '\0');
    ck_assert(cs.portal != '\0');
}
END_TEST


START_TEST(load_world_success_first_room_valid)
{
    Graph *g = NULL;
    Room *first = NULL;
    int n = 0;
    Charset cs;

    ck_assert_int_eq(loader_load_world("../assets/starter.ini", &g, &first, &n, &cs), OK);

    ck_assert_ptr_nonnull(first);
    ck_assert_int_ge(room_get_width(first), 0);
    ck_assert_int_ge(room_get_height(first), 0);
}
END_TEST


START_TEST(load_world_success_two_calls)
{
    Graph *g1 = NULL;
    Room *r1 = NULL;
    int n1 = 0;
    Charset cs1;

    Graph *g2 = NULL;
    Room *r2 = NULL;
    int n2 = 0;
    Charset cs2;

    ck_assert_int_eq(loader_load_world("../assets/starter.ini", &g1, &r1, &n1, &cs1), OK);
    ck_assert_int_eq(loader_load_world("../assets/starter.ini", &g2, &r2, &n2, &cs2), OK);

    ck_assert_ptr_nonnull(g1);
    ck_assert_ptr_nonnull(g2);
    ck_assert_int_gt(n1, 0);
    ck_assert_int_gt(n2, 0);
}
END_TEST


Suite *world_loader_suite(void) {
    Suite *s = suite_create("world_loader");
    TCase *tc = tcase_create("stub");

    tcase_add_test(tc, load_world_null_graph_out);
    tcase_add_test(tc, load_world_null_first_room_out);
    tcase_add_test(tc, load_world_null_num_rooms_out);
    tcase_add_test(tc, load_world_null_charset_out);
    tcase_add_test(tc, load_world_null_config);
    tcase_add_test(tc, load_world_empty_config);
    tcase_add_test(tc, load_world_bad_path);
    tcase_add_test(tc, load_world_bad_extension);
    tcase_add_test(tc, load_world_fail_keeps_nulls_null);
    tcase_add_test(tc, load_world_path_with_spaces);
    tcase_add_test(tc, load_world_two_calls_same_bad_config);
    tcase_add_test(tc, load_world_success_basic);
    tcase_add_test(tc, load_world_success_charset_loaded);
    tcase_add_test(tc, load_world_success_first_room_valid);
    tcase_add_test(tc, load_world_success_two_calls);


    suite_add_tcase(s, tc);
    return s;
}
