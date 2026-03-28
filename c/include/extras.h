#ifndef EXTRAS
#define EXTRAS

#include <stdbool.h>
#include <stdint.h>
#include "types.h"
#include <stdio.h>
#include <stdbool.h>
#include "game_engine.h"
#include "player.h"
#include "world_loader.h"
#include "room.h"


Status room_reset_current_state(Room *r);

Status player_remove_collected_in_room(Player *p, int room_id);
Status player_uncollect_treasure(Player *p, int treasure_id);


Status game_engine_get_total_treasure_count(const GameEngine *eng,
                                            int *count_out);


Status game_engine_use_portal(GameEngine *eng);


Status game_engine_get_charset(const GameEngine *eng, Charset *charset_out);

Status game_engine_reset_current_room(GameEngine *eng);


typedef struct {
    bool valid;

    int prev_room_id;
    int prev_x;
    int prev_y;

    bool collected_treasure;
    int treasure_room_id;
    int treasure_id;
    int treasure_prev_x;
    int treasure_prev_y;

    bool moved_pushable;
    int pushable_room_id;
    int pushable_id;
    int pushable_prev_x;
    int pushable_prev_y;
} LastMoveState;

typedef struct {
    GameEngine base;
    LastMoveState undo;
} GameEngineEx;


#endif /* EXTRAS */
