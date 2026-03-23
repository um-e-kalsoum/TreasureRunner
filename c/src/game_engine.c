#include <stdlib.h>
#include <string.h>
#include "game_engine.h"
#include "world_loader.h"
#include "graph.h"
#include "room.h"
#include "player.h"
#include "types.h"

//searches the graph for a room with a given id, returns pointer to room
static const Room *get_room_by_id(const Graph *graph, int room_id) {
    //initalize temp room value
    Room key_room;
    memset(&key_room, 0, sizeof(Room));

    //set and return room with the desired payload
    key_room.id = room_id;
    return (const Room *)graph_get_payload(graph, &key_room);
}

static Room *get_room_by_id_mut(Graph *graph, int room_id) {
    Room key_room;
    memset(&key_room, 0, sizeof(Room));

    key_room.id = room_id;
    return (Room *)graph_get_payload(graph, &key_room);
}

//builds entire game state
Status game_engine_create(const char *config_file_path, GameEngine **engine_out) {
    Graph *graph = NULL;
    Room *first_room = NULL;
    int room_count = 0;
    Charset charset;
    GameEngine *engine = NULL;
    Player *player = NULL;
    int start_x = 0;
    int start_y = 0;
    Status load_status = OK;
    Status player_status = OK;
    Status start_status = OK;

    if (config_file_path == NULL || engine_out == NULL) {
        return INVALID_ARGUMENT;
    }

    *engine_out = NULL;

    //load the world (create graph + loads charset + returns first room and total room count)
    load_status = loader_load_world(config_file_path, &graph, &first_room, &room_count, &charset);
    
    if (load_status != OK) {
        return load_status;
    }

    if (graph == NULL || first_room == NULL || room_count <= 0) {
        graph_destroy(graph);
        return WL_ERR_DATAGEN;
    }

    //determine valid player start position
    start_status = room_get_start_position(first_room, &start_x, &start_y);

    if (start_status != OK) {
        graph_destroy(graph);
        return start_status;
    }

    //create the player at the valid start position
    player_status = player_create(first_room->id, start_x, start_y, &player);

    if (player_status != OK) {
        graph_destroy(graph);
        return player_status;
    }

    //alloc the game engine
    engine = (GameEngine *)malloc(sizeof(GameEngine));

    if (engine == NULL) {
        player_destroy(player);
        graph_destroy(graph);
        return NO_MEMORY;
    }

    //populate game engine, game engine owns everything
    engine->graph = graph;
    engine->player = player;
    engine->charset = charset;

    engine->initial_room_id = first_room->id;
    engine->initial_player_x = start_x;
    engine->initial_player_y = start_y;

    engine->room_count = room_count;

    //returns created engine 
    *engine_out = engine;
    return OK;
}

void game_engine_destroy(GameEngine *eng) {
     if (eng == NULL) {
        return;
    }

    //free player struct
    player_destroy(eng->player);
    eng->player = NULL;

    //free graph which also frees rooms
    graph_destroy(eng->graph);
    eng->graph = NULL;

    //free engine itself
    free(eng);
}

const Player *game_engine_get_player(const GameEngine *eng) {
    if (eng == NULL) {
        return NULL;
    }
    return eng->player;
}

static bool dir_is_valid(Direction dir) {
    return (dir == DIR_NORTH || dir == DIR_SOUTH || dir == DIR_EAST || dir == DIR_WEST);
}

static void step_from_dir(Direction dir, int *dx_out, int *dy_out) {
    int dx = 0;
    int dy = 0;
    if (dir == DIR_NORTH) {
        dy = -1;
    } else if (dir == DIR_SOUTH) {
        dy = 1;
    } else if (dir == DIR_EAST)  {
        dx = 1;
    } else if (dir == DIR_WEST)  {
        dx = -1;
    }

    *dx_out = dx;
    *dy_out = dy;
}

static Status move_into_floor(GameEngine *eng, int next_x, int next_y) {

    return (player_set_position(eng->player, next_x, next_y) == OK)
        ? OK
        : ROOM_IMPASSABLE;
}

static Status handle_pushable(GameEngine *eng, Room *room, int push_idx, Direction dir, int next_x, int next_y) {
    Status st = room_try_push(room, push_idx, dir);
    if (st == OK){
        return move_into_floor(eng, next_x, next_y);
    } 
    if (st == ROOM_IMPASSABLE){
        return ROOM_IMPASSABLE;
    }
    return ROOM_IMPASSABLE; 
}

static Status handle_treasure(GameEngine *eng, Room *room, int treasure_id, int next_x, int next_y) {
    Treasure *t = NULL;
    Status st = room_pick_up_treasure(room, treasure_id, &t);

    (void)next_x;
    (void)next_y;

    if (st != OK) {
        return ROOM_IMPASSABLE;
    }

    st = player_try_collect(eng->player, t);
    if (st != OK) {
        return ROOM_IMPASSABLE;
    }

    return OK;
}

static Status handle_portal(GameEngine *eng, int dest_room_id) {
    const Room *dest = get_room_by_id(eng->graph, dest_room_id);
    int sx = 0;
    int sy = 0;

    if (dest == NULL) {
        return GE_NO_SUCH_ROOM;
    }

    if (room_get_start_position(dest, &sx, &sy) != OK) {
        return ROOM_IMPASSABLE;
    }

    if (player_move_to_room(eng->player, dest_room_id) != OK) {
        return ROOM_IMPASSABLE;
    }
    if (player_set_position(eng->player, sx, sy) != OK) {
        return ROOM_IMPASSABLE;
    }

    return OK;
}

Status game_engine_move_player(GameEngine *eng, Direction dir) {
    int x = 0;
    int y = 0;
    int dx = 0;
    int dy = 0;
    int room_id = -1;
    Room *room = NULL;
    int out_id = -1;

    if (eng == NULL) {
        return INVALID_ARGUMENT;
    }
    if (eng->graph == NULL || eng->player == NULL) {
        return INTERNAL_ERROR;
    }
    if (!dir_is_valid(dir)) {
        return INVALID_ARGUMENT;
    }

    room_id = player_get_room(eng->player);
    room = get_room_by_id_mut(eng->graph, room_id);
    if (room == NULL) {
        return GE_NO_SUCH_ROOM;
    }

    if (player_get_position(eng->player, &x, &y) != OK) {
        return INTERNAL_ERROR;
    }

    step_from_dir(dir, &dx, &dy);
    int nx = x + dx;
    int ny = y + dy;

    RoomTileType tt = room_classify_tile(room, nx, ny, &out_id);

    if (tt == ROOM_TILE_INVALID || tt == ROOM_TILE_WALL) {
        return ROOM_IMPASSABLE;
    }
    if (tt == ROOM_TILE_PUSHABLE) {
        return handle_pushable(eng, room, out_id, dir, nx, ny);
    }


    if (tt == ROOM_TILE_TREASURE) {
        return handle_treasure(eng, room, out_id, nx, ny);
    }

    if (tt == ROOM_TILE_PORTAL && out_id >= 0) {
        return handle_portal(eng, out_id);
    }

    return move_into_floor(eng, nx, ny);
}

Status game_engine_get_room_count(const GameEngine *eng, int *count_out) {
     if (eng == NULL) {
        return INVALID_ARGUMENT;
    }
    if (count_out == NULL) {
        return NULL_POINTER;
    }

    *count_out = eng->room_count;
    return OK;
}

Status game_engine_get_room_dimensions(const GameEngine *eng, int *width_out, int *height_out) {
    int room_id = -1;
    const Room *room = NULL;

    if (eng == NULL) {
        return INVALID_ARGUMENT;
    }
    if (width_out == NULL || height_out == NULL) {
        return NULL_POINTER;
    }
    if (eng->player == NULL || eng->graph == NULL) {
        return INTERNAL_ERROR;
    }

    //find what room id player is in
    room_id = player_get_room(eng->player);

    //loop up the room
    room = get_room_by_id(eng->graph, room_id);

    if (room == NULL) {
        return GE_NO_SUCH_ROOM;
    }

    //set values
    *width_out = room_get_width(room);
    *height_out = room_get_height(room);
    return OK;
}

Status game_engine_reset(GameEngine *eng) {
    if (eng == NULL) {
        return INVALID_ARGUMENT;
    }

    if (eng->player == NULL || eng->graph == NULL) {
        return INTERNAL_ERROR;
    }

    if (player_reset_to_start(eng->player,
                              eng->initial_room_id,
                              eng->initial_player_x,
                              eng->initial_player_y) != OK) {
        return INTERNAL_ERROR;
    }

    const void * const *payloads = NULL;
    int count = 0;

    if (graph_get_all_payloads(eng->graph, &payloads, &count) != GRAPH_STATUS_OK) {
        return INTERNAL_ERROR;
    }

    for (int i = 0; i < count; i++) {
        Room *r = (Room *)payloads[i];

        // reset treasures
        for (int t = 0; t < r->treasure_count; t++) {
            r->treasures[t].collected = false;
            r->treasures[t].x = r->treasures[t].initial_x;
            r->treasures[t].y = r->treasures[t].initial_y;
        }

        // reset pushables
        for (int p = 0; p < r->pushable_count; p++) {
            r->pushables[p].x = r->pushables[p].initial_x;
            r->pushables[p].y = r->pushables[p].initial_y;
        }
    }

    return OK;
}

Status game_engine_render_current_room(const GameEngine *eng, char **str_out) {
    int room_id = -1;
    int player_x = 0;
    int player_y = 0;
    char *room_string = NULL;
    int width = 0;
    int height = 0;
    int line_length = 0;

    if (eng == NULL || str_out == NULL) {
        return INVALID_ARGUMENT;
    }
    if (eng->player == NULL) {
        return INTERNAL_ERROR;
    }

    //determine what room player is in
    room_id = player_get_room(eng->player);

    //render room
    if (game_engine_render_room(eng, room_id, &room_string) != OK) {
        return INTERNAL_ERROR;
    }

    //get dimensions 
    if (game_engine_get_room_dimensions(eng, &width, &height) != OK) {
        free(room_string);
        return INTERNAL_ERROR;
    }

    //get pos
    if (player_get_position(eng->player, &player_x, &player_y) != OK) {
        free(room_string);
        return INTERNAL_ERROR;
    }

    //compute length
    line_length = width + 1;

    if (player_x >= 0 && player_x < width && player_y >= 0 && player_y < height) {
        //replace tile char with player char at given pos 
        room_string[player_y * line_length + player_x] = eng->charset.player;
    }

    *str_out = room_string;
    return OK;
}

Status game_engine_render_room(const GameEngine *eng, int room_id, char **str_out) {
    const Room *room = NULL;
    int width = 0;
    int height = 0;
    int tile_count = 0;
    char *framebuffer = NULL;
    char *output = NULL;
    int output_size = 0;
    int out_index = 0;
    int y = 0;
    int x = 0;
    Status render_status = OK;

    if (eng == NULL) {
        return INVALID_ARGUMENT;
    }
    if (str_out == NULL) {
        return NULL_POINTER;
    }
    if (eng->graph == NULL) {
        return INTERNAL_ERROR;
    }

    *str_out = NULL;

    //find the room from graph
    room = get_room_by_id(eng->graph, room_id);
    if (room == NULL) {
        return GE_NO_SUCH_ROOM;
    }

    //set values
    width = room_get_width(room);
    height = room_get_height(room);

    if (width <= 0 || height <= 0) {
        return INTERNAL_ERROR;
    }

    //allocate frame buuffer
    tile_count = width * height;
    framebuffer = (char *)malloc((size_t)tile_count);

    if (framebuffer == NULL) {
        return NO_MEMORY;
    }

    //render room to fill framebuffer with the correct chars
    render_status = room_render(room, &eng->charset, framebuffer, width, height);

    if (render_status != OK) {
        free(framebuffer);
        return INTERNAL_ERROR;
    }

    //allocate the final printable representation of the room -> 1D
    output_size = tile_count + height + 1;
    output = (char *)malloc((size_t)output_size);

    if (output == NULL) {
        free(framebuffer);
        return NO_MEMORY;
    }

    //copy the frambuffer into the printable representation of the room -> 2D
    out_index = 0;
    for (y = 0; y < height; y++) {
        for (x = 0; x < width; x++) {
            output[out_index] = framebuffer[y * width + x];
            out_index++;
        }
        output[out_index] = '\n';
        out_index++;
    }
    output[out_index] = '\0';

    free(framebuffer);
    *str_out = output;
    return OK;
}

Status game_engine_get_room_ids(const GameEngine *eng, int **ids_out, int *count_out) {
    const void * const *payloads = NULL;
    int payload_count = 0;
    int *ids = NULL;
    int i = 0;
    GraphStatus graph_status = GRAPH_STATUS_OK;

    if (eng == NULL) {
        return INVALID_ARGUMENT;
    }
    if (ids_out == NULL || count_out == NULL) {
        return NULL_POINTER;
    }
    if (eng->graph == NULL) {
        return INTERNAL_ERROR;
    }

    *ids_out = NULL;
    *count_out = 0;

    //get all payloads 
    graph_status = graph_get_all_payloads(eng->graph, &payloads, &payload_count);

    if (graph_status != GRAPH_STATUS_OK) {
        return INTERNAL_ERROR;
    }

    //alloc the array to hold all the ids
    ids = (int *)malloc(sizeof(int) * (size_t)payload_count);

    if (ids == NULL) {
        return NO_MEMORY;
    }

    //add the ids of all the rooms into the array
    for (i = 0; i < payload_count; i++) {
        const Room *room = (const Room *)payloads[i]; //convert generic pointer to room
        ids[i] = room->id;
    }

    *ids_out = ids;
    *count_out = payload_count;
    return OK;
}


// ****************** new to A2 ******************
void game_engine_free_string(void *ptr){
    free(ptr);
}
