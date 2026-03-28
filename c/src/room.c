#include "room.h"
#include <stdlib.h>
#include <string.h>


Status room_reset_current_state(Room *r);


// ************** helpers start **************

static char *copy_c_string(const char *s) {
    if (s == NULL) {
        return NULL;
    }

    size_t len = strlen(s);
    char *out = (char *)malloc(len + 1);

    if (out == NULL) {
        return NULL;
    }

    memcpy(out, s, len + 1);
    return out;
}

static bool in_bounds(const Room *r, int x, int y) {
    return (r != NULL && x >= 0 && y >= 0 && x < r->width && y < r->height);
}


static void room_free_pushable_names(Pushable *arr, int count) {
    if (arr == NULL) {
        return;
    }
    for (int i = 0; i < count; i++) {
        free(arr[i].name);
    }
}

static void free_treasure_names(Treasure *arr, int count) {
    if (arr == NULL) {
        return;
    }
    for (int i = 0; i < count; i++) {
        free(arr[i].name);
    }
}

// ************** helpers end **************

Room *room_create(int id, const char *name, int width, int height) {
    Room *r = malloc(sizeof(Room));

    //alloc fail
    if(r == NULL){
        return NULL;
    }

    if(width < 1){
        width = 1; 
    }

    if(height < 1){
        height = 1;
    }

    r->name = copy_c_string(name);

    if (name != NULL && r->name == NULL) {
        free(r);
        return NULL;
    }

    r->id = id;
    r->width = width;
    r->height = height;
    r->floor_grid = NULL;

    r->neighbors = NULL;
    r->neighbor_count = 0;

    r->portals = NULL;
    r->portal_count = 0;

    r->treasures = NULL;
    r->treasure_count = 0;

    r->pushables = NULL;
    r->pushable_count = 0;

    r->switches = NULL;
    r->switch_count = 0;

    return r;
}

int room_get_width(const Room *r) {
    if(r == NULL){
        return 0;
    }    

    return r->width;
}

int room_get_height(const Room *r) {
    if(r == NULL){
        return 0;
    }    

    return r->height;
}

Status room_set_floor_grid(Room *r, bool *floor_grid) {
    if(r == NULL){
        return INVALID_ARGUMENT;
    }

    free(r->floor_grid);    
    r->floor_grid = floor_grid;

    return OK;
}

Status room_set_portals(Room *r, Portal *portals, int portal_count) {
    if(r == NULL || portal_count < 0){
        return INVALID_ARGUMENT;
    }

    if (portal_count > 0 && portals == NULL) {
        return INVALID_ARGUMENT;
    }

    if (r->portals != NULL) {
        for (int i = 0; i < r->portal_count; i++) {
            free(r->portals[i].name);
        }
        free(r->portals);
    }

    r->portals = portals;
    r->portal_count = portal_count;

    return OK;
}

Status room_set_treasures(Room *r, Treasure *treasures, int treasure_count) {
    if (r == NULL || treasure_count < 0) {
        return INVALID_ARGUMENT;
    }

    if (treasure_count > 0 && treasures == NULL) {
        return INVALID_ARGUMENT;
    }

    if (r->treasures != NULL) {
        for (int i = 0; i < r->treasure_count; i++) {
            free(r->treasures[i].name);
        }
        free(r->treasures);
    }

    r->treasures = treasures;
    r->treasure_count = treasure_count;

    return OK;
}

Status room_place_treasure(Room *r, const Treasure *treasure) {
    if (r == NULL || treasure == NULL) {
        return INVALID_ARGUMENT;
    }

    Treasure *new_arr = realloc(r->treasures,(r->treasure_count + 1) * sizeof(Treasure));

    if (new_arr == NULL) {
        return NO_MEMORY;
    }

    //assign treasures to new realloc array
    r->treasures = new_arr;
    r->treasures[r->treasure_count] = *treasure;
    r->treasure_count++;

    return OK;
}

int room_get_treasure_at(const Room *r, int x, int y) {
    if(r == NULL){
        return -1;
    }

    for (int i = 0; i < r->treasure_count; i++) {
        if (r->treasures[i].collected) {
            continue;
        }

    if (r->treasures[i].x == x && r->treasures[i].y == y) {
            return r->treasures[i].id;
        }
    }

    return -1;

}

int room_get_portal_destination(const Room *r, int x, int y) {
    if(r == NULL){
        return -1;
    }    

    for (int i = 0; i < r->portal_count; i++) {
        if (r->portals[i].x == x && r->portals[i].y == y) {
            return r->portals[i].target_room_id;
        }
    }

    return -1;
}

bool room_is_walkable(const Room *r, int x, int y) {
    if(r == NULL){
        return false;
    }

    if (!in_bounds(r, x, y)) {
        return false;
    }
    
    if (room_has_pushable_at(r, x, y, NULL)) {
        return false;
    }


    //floor grid exists
    if (r->floor_grid != NULL) {
        //index = row * width + col
        int exi = y * r->width + x;
        return r->floor_grid[exi];
    }

    //walls
    if (x == 0 || y == 0 || x == r->width - 1 || y == r->height - 1) {
        return false;
    }

    return true;
}

RoomTileType room_classify_tile(const Room *r, int x, int y, int *out_id) {
     if (r == NULL) {
        return ROOM_TILE_INVALID;
    }

    if (!in_bounds(r, x, y)) {
        return ROOM_TILE_INVALID;
    }

    //pushable?
    int pidx = -1;
    if (room_has_pushable_at(r, x, y, &pidx)) {
        if (out_id != NULL) {
            *out_id = pidx;
        }
        return ROOM_TILE_PUSHABLE;
    }

    //treasure?
    int tid = room_get_treasure_at(r, x, y);
    if (tid != -1) {
        if (out_id != NULL) {
            *out_id = tid;
        }
        return ROOM_TILE_TREASURE;
    }

    //portal?
    int pid = room_get_portal_destination(r, x, y);
    if (pid != -1) {
        if (out_id != NULL) {
            *out_id = pid;
        }
        return ROOM_TILE_PORTAL;
    }

    //walkable?
    if (room_is_walkable(r, x, y)) {
        return ROOM_TILE_FLOOR;
    }

    return ROOM_TILE_WALL;
}

// ********************** render helpers start **********************
static bool base_tile_is_floor(const Room *r, int x, int y) {
    if (r->floor_grid != NULL) {
        int idx = y * r->width + x;
        return r->floor_grid[idx];
    }

    return !(x == 0 || y == 0 || x == r->width - 1 || y == r->height - 1);
}

static void render_base_layer(const Room *r, const Charset *charset, char *buffer) {
    int w = r->width;
    int h = r->height;

    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            int idx = y * w + x;
            buffer[idx] = (char)(unsigned char)(base_tile_is_floor(r, x, y) ? charset->floor : charset->wall);

        }
    }
}

static void overlay_treasures(const Room *r, const Charset *charset, char *buffer) {
    int w = r->width;
    int h = r->height;

    for (int i = 0; i < r->treasure_count; i++) {
        if (r->treasures[i].collected) {
            continue;
        }

        int x = r->treasures[i].x;
        int y = r->treasures[i].y;

        if (x >= 0 && y >= 0 && x < w && y < h) {
            buffer[y * w + x] = (char)(unsigned char)charset->treasure;
        }
    }
}

static void overlay_pushables(const Room *r, const Charset *charset, char *buffer) {
    int w = r->width;
    int h = r->height;

    for (int i = 0; i < r->pushable_count; i++) {
        int x = r->pushables[i].x;
        int y = r->pushables[i].y;

        if (x >= 0 && y >= 0 && x < w && y < h) {
            buffer[y * w + x] = (char)(unsigned char)charset->pushable;
        }
    }
}

static void overlay_portals(const Room *r, const Charset *charset, char *buffer) {
    int w = r->width;
    int h = r->height;

    for (int i = 0; i < r->portal_count; i++) {
        int x = r->portals[i].x;
        int y = r->portals[i].y;

        if (x >= 0 && y >= 0 && x < w && y < h) {
            buffer[y * w + x] = (char)(unsigned char)charset->portal;
        }
    }
}

// ********************** render helpers end **********************


Status room_render(const Room *r,
                   const Charset *charset,
                   char *buffer,
                   int buffer_width,
                   int buffer_height) {

    if (r == NULL || charset == NULL || buffer == NULL) {
        return INVALID_ARGUMENT;
    }

    if (buffer_width != r->width || buffer_height != r->height) {
        return INVALID_ARGUMENT;
    }

    render_base_layer(r, charset, buffer);
    overlay_treasures(r, charset, buffer);
    overlay_pushables(r, charset, buffer);
    overlay_portals(r, charset, buffer);

    return OK;
}

Status room_get_start_position(const Room *r, int *x_out, int *y_out) {
    if (r == NULL || x_out == NULL || y_out == NULL) {
            return INVALID_ARGUMENT;
        }

        //first portal
        if (r->portal_count > 0 && r->portals != NULL) {
            *x_out = r->portals[0].x;
            *y_out = r->portals[0].y;
            return OK;
        }

        //first walkable
        for (int row = 1; row < r->height - 1; row++) {
            for (int col = 1; col < r->width - 1; col++) {
                if (room_is_walkable(r, col, row)) {
                    *x_out = col;
                    *y_out = row;
                    return OK;
                }
            }
        }

    return ROOM_NOT_FOUND;
}

void room_destroy(Room *r) {
    if(r == NULL) {
        return;
    }

    free(r->name);
    free(r->floor_grid);
    free(r->neighbors);

    if (r->portals != NULL) {
        for (int i = 0; i < r->portal_count; i++) {
            free(r->portals[i].name);
        }
        free(r->portals);
    }

    if (r->treasures != NULL) {
        free_treasure_names(r->treasures, r->treasure_count);
        free(r->treasures);
    }

    if (r->pushables != NULL) {
        room_free_pushable_names(r->pushables, r->pushable_count);
        free(r->pushables);
    }

    if (r->switches != NULL) {
        free(r->switches);
    }

    free(r);

}

// ****************** new to A2 ******************

int room_get_id(const Room *r){
    if (r == NULL) {
        return -1;
    }

    return r->id;
}

Status room_pick_up_treasure(Room *r, int treasure_id, Treasure **treasure_out){
    if (r == NULL || treasure_out == NULL || treasure_id < 0) {
        return INVALID_ARGUMENT;
    }

    for (int i = 0; i < r->treasure_count; i++) {
        if (r->treasures[i].id == treasure_id) {
            if (r->treasures[i].collected) {
                return INVALID_ARGUMENT;
            }
            r->treasures[i].collected = true;
            *treasure_out = &r->treasures[i];
            return OK;
        }
    }

    return ROOM_NOT_FOUND;
}

void destroy_treasure(Treasure *t){
    if (t == NULL) {
        return;
    }

    free(t->name);
    free(t);
}

bool room_has_pushable_at(const Room *r,
                          int x,
                          int y,
                          int *pushable_idx_out){
    if (r == NULL) {
        return false;
    }

    for (int i = 0; i < r->pushable_count; i++) {
        if (r->pushables[i].x == x && r->pushables[i].y == y) {
            if (pushable_idx_out != NULL) {
                *pushable_idx_out = i;
            }
            return true;
        }
    }

    return false;
}

Status room_try_push(Room *r,
                     int pushable_idx,
                     Direction dir){
    if (r == NULL) {
        return INVALID_ARGUMENT;
    }

    if (pushable_idx < 0 || pushable_idx >= r->pushable_count) {
        return INVALID_ARGUMENT;
    }

    if (dir != DIR_NORTH && dir != DIR_SOUTH && dir != DIR_EAST && dir != DIR_WEST) {
        return INVALID_ARGUMENT;
    }

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

    int from_x = r->pushables[pushable_idx].x;
    int from_y = r->pushables[pushable_idx].y;

    int to_x = from_x + dx;
    int to_y = from_y + dy;

    if (!room_is_walkable(r, to_x, to_y)) {
        return ROOM_IMPASSABLE;
    }

    if (room_has_pushable_at(r, to_x, to_y, NULL)) {
        return ROOM_IMPASSABLE;
    }

    r->pushables[pushable_idx].x = to_x;
    r->pushables[pushable_idx].y = to_y;

    return OK;
}

//added for a3

Status room_reset_current_state(Room *r) {
    int i = 0;

    if (r == NULL) {
        return INVALID_ARGUMENT;
    }

    for (i = 0; i < r->pushable_count; i++) {
        r->pushables[i].x = r->pushables[i].initial_x;
        r->pushables[i].y = r->pushables[i].initial_y;
    }

    for (i = 0; i < r->treasure_count; i++) {
        r->treasures[i].x = r->treasures[i].initial_x;
        r->treasures[i].y = r->treasures[i].initial_y;
        r->treasures[i].collected = false;
    }

    return OK;
}