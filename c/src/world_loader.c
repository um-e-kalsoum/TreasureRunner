#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "world_loader.h"
#include "graph.h"
#include "room.h"
#include "datagen.h"
#include "types.h"
#include "extras.h"


//compare 2 generic pointers (so graph can work with any data type) to rooms
static int compare_rooms_by_id(const void *a, const void *b) {
    //cast so we can access fields
    const Room *room_a = (const Room *)a;
    const Room *room_b = (const Room *)b;

    if (room_a->id < room_b->id) {
        return -1;
    }
    if (room_a->id > room_b->id) {
        return 1;
    }
    return 0;
}

//destory rooms so we can destory graoh later
static void destroy_room_payload(void *payload) {
    room_destroy((Room *)payload);
}

//converts graph error code to loaders status value for consistency
static Status map_graph_status(GraphStatus graph_status) {
    if (graph_status == GRAPH_STATUS_NO_MEMORY) {
        return NO_MEMORY;
    }
    return WL_ERR_DATAGEN;
}

//deep copy of a string and returns pointer to new string
static char *copy_c_string(const char *text) {
    size_t len = 0;
    char *copy = NULL;

    if (text == NULL) {
        return NULL;
    }

    len = strlen(text);
    copy = (char *)malloc(len + 1);
    if (copy == NULL) {
        return NULL;
    }

    memcpy(copy, text, len + 1);
    return copy;
}

//populates charset struct
static Status copy_charset(Charset *charset_out) {
    const DG_Charset *dg_charset = NULL;

    dg_charset = dg_get_charset();
    if (dg_charset == NULL) {
        return WL_ERR_DATAGEN;
    }

    charset_out->wall = dg_charset->wall;
    charset_out->floor = dg_charset->floor;
    charset_out->player = dg_charset->player;
    charset_out->treasure = dg_charset->treasure;
    charset_out->portal = dg_charset->portal;
    charset_out->pushable = dg_charset->pushable;

    charset_out->switch_off = dg_charset->switch_off;
    charset_out->switch_on  = dg_charset->switch_on;

    return OK;
}

//copies floor grid and transfers ownership to room
static Status copy_floor_grid(Room *room, const DG_Room *dg_room) {
    int tile_count = 0;
    bool *floor_grid_copy = NULL;
    Status floor_status = OK;

    tile_count = dg_room->width * dg_room->height;
    floor_grid_copy = NULL;

    //alloc and copy into room struct
    if (dg_room->floor_grid != NULL) {
        floor_grid_copy = (bool *)malloc(sizeof(bool) * (size_t)tile_count);
        if (floor_grid_copy == NULL) {
            return NO_MEMORY;
        }
        memcpy(floor_grid_copy, dg_room->floor_grid, sizeof(bool) * (size_t)tile_count);
    }

    //assigns ownership to room
    floor_status = room_set_floor_grid(room, floor_grid_copy);
    if (floor_status != OK) {
        free(floor_grid_copy);
        return WL_ERR_DATAGEN;
    }

    return OK;
}

static Status copy_portals(Room *room, const DG_Room *dg_room) {
    Portal *portal_copy = NULL;
    Status portal_status = OK;
    int portal_index = 0;

    portal_copy = NULL;
    
    if (dg_room->portal_count > 0) {
        //calloc so everything is initalized to 0/null
        portal_copy = (Portal *)calloc((size_t)dg_room->portal_count, sizeof(Portal));
        if (portal_copy == NULL) {
            return NO_MEMORY;
        }

        //for each portal copy in all data
        for (portal_index = 0; portal_index < dg_room->portal_count; portal_index++) {
            portal_copy[portal_index].id = dg_room->portals[portal_index].id;
            portal_copy[portal_index].name = NULL;
            portal_copy[portal_index].x = dg_room->portals[portal_index].x;
            portal_copy[portal_index].y = dg_room->portals[portal_index].y;
            portal_copy[portal_index].target_room_id = dg_room->portals[portal_index].neighbor_id;
            portal_copy[portal_index].required_switch_id = dg_room->portals[portal_index].required_switch_id;
            portal_copy[portal_index].gated = (dg_room->portals[portal_index].required_switch_id >= 0);
        }
    }

    //set ownership to room
    portal_status = room_set_portals(room, portal_copy, dg_room->portal_count);
    if (portal_status != OK) {
        free(portal_copy);
        return WL_ERR_DATAGEN;
    }

    return OK;
}

//frees all treasure names 
static void free_treasure_names(Treasure *treasure_copy, int treasure_count) {
    if (treasure_copy == NULL) {
        return;
    }

    for (int i = 0; i < treasure_count; i++) {
        free(treasure_copy[i].name);
    }
}

static Status copy_treasures(Room *room, const DG_Room *dg_room) {
    Treasure *treasure_copy = NULL;
    Status treasure_status = OK;
    int treasure_index = 0;

    treasure_copy = NULL;

    if (dg_room->treasure_count > 0) {
        //calloc to initalize to 0/null, belongs to room
        treasure_copy = (Treasure *)calloc((size_t)dg_room->treasure_count, sizeof(Treasure));
        if (treasure_copy == NULL) {
            return NO_MEMORY;
        }

        //copy each treasure populate fields
        for (treasure_index = 0; treasure_index < dg_room->treasure_count; treasure_index++) {
           
            treasure_copy[treasure_index].id = dg_room->treasures[treasure_index].global_id;
            treasure_copy[treasure_index].name = copy_c_string(dg_room->treasures[treasure_index].name);

            if (dg_room->treasures[treasure_index].name != NULL && treasure_copy[treasure_index].name == NULL) {
                free_treasure_names(treasure_copy, treasure_index);
                free(treasure_copy);
                return NO_MEMORY;
            }

            treasure_copy[treasure_index].starting_room_id = dg_room->id;

            treasure_copy[treasure_index].initial_x = dg_room->treasures[treasure_index].x;
            treasure_copy[treasure_index].initial_y = dg_room->treasures[treasure_index].y;

            treasure_copy[treasure_index].x = dg_room->treasures[treasure_index].x;
            treasure_copy[treasure_index].y = dg_room->treasures[treasure_index].y;

            treasure_copy[treasure_index].collected = false;
        }
    }

    //set ownership
    treasure_status = room_set_treasures(room, treasure_copy, dg_room->treasure_count);
    if (treasure_status != OK) {
        free_treasure_names(treasure_copy, dg_room->treasure_count);
        free(treasure_copy);
        return WL_ERR_DATAGEN;
    }

    return OK;
}

static void loader_free_pushable_names(Pushable *arr, int count) {
    if (arr == NULL) return;
    for (int i = 0; i < count; i++) {
        free(arr[i].name);
    }
}

static Status copy_pushables(Room *room, const DG_Room *dg_room) {
    Pushable *push_copy = NULL;

    if (dg_room->pushable_count > 0) {
        push_copy = (Pushable *)calloc((size_t)dg_room->pushable_count, sizeof(Pushable));
        if (push_copy == NULL) {
            return NO_MEMORY;
        }

        for (int i = 0; i < dg_room->pushable_count; i++) {
            push_copy[i].id = dg_room->pushables[i].id;

            push_copy[i].name = copy_c_string(dg_room->pushables[i].name);
            if (dg_room->pushables[i].name != NULL && push_copy[i].name == NULL) {
                loader_free_pushable_names(push_copy, i);
                free(push_copy);
                return NO_MEMORY;
            }

            push_copy[i].initial_x = dg_room->pushables[i].x;
            push_copy[i].initial_y = dg_room->pushables[i].y;
            push_copy[i].x = dg_room->pushables[i].x;
            push_copy[i].y = dg_room->pushables[i].y;
        }
    }

    room->pushables = push_copy;
    room->pushable_count = dg_room->pushable_count;

    return OK;
}

static Status copy_switches(Room *room, const DG_Room *dg_room) {
    Switch *sw_copy = NULL;

    if (dg_room->switch_count > 0) {
        sw_copy = (Switch *)calloc((size_t)dg_room->switch_count, sizeof(Switch));
        if (sw_copy == NULL) {
            return NO_MEMORY;
        }

        for (int i = 0; i < dg_room->switch_count; i++) {
            sw_copy[i].id = dg_room->switches[i].id;
            sw_copy[i].x  = dg_room->switches[i].x;
            sw_copy[i].y  = dg_room->switches[i].y;
            sw_copy[i].portal_id = dg_room->switches[i].portal_id;
        }
    }

    room->switches = sw_copy;
    room->switch_count = dg_room->switch_count;

    return OK;
}



//stores all room pointers in temp array so loader can find rooms by id and connect with portals
static Status append_room(Room ***room_array_out, int *capacity_out, int *count_out, Room *room) {
    Room **room_array = NULL;
    Room **new_array = NULL;
    int capacity = 0;
    int count = 0;
    int new_capacity = 0;

    room_array = *room_array_out;
    capacity = *capacity_out;
    count = *count_out;

    //array is full so resize
    if (count == capacity) {
        new_capacity = (capacity == 0) ? 8 : capacity * 2;
        new_array = (Room **)realloc(room_array, sizeof(Room *) * (size_t)new_capacity);
        if (new_array == NULL) {
            return NO_MEMORY;
        }
        //reset
        room_array = new_array;
        capacity = new_capacity;
    }

    //insert
    room_array[count] = room;
    count++;

    //reset
    *room_array_out = room_array;
    *capacity_out = capacity;
    *count_out = count;

    return OK;
}

static Status connect_rooms(Graph *graph, Room **room_array, int room_count) {
    int from_index = 0;
    int portal_index= 0;
    int to_index = 0;
    int target_id =0;
    Room *from_room = NULL;
    Room *to_room  = NULL;
    GraphStatus connect_status = GRAPH_STATUS_OK;

    //loop every room
    for (from_index = 0; from_index < room_count; from_index++) {
        from_room = room_array[from_index];

        //loop through current rooms portals 
        for (portal_index = 0; portal_index < from_room->portal_count; portal_index++) {
            target_id = from_room->portals[portal_index].target_room_id;
            //no portal
            if (target_id < 0) {
                continue;
            }

            to_room = NULL;

            //find the room with this target_id
            for (to_index = 0; to_index < room_count; to_index++) {
                if (room_array[to_index]->id == target_id) {
                    //set
                    to_room = room_array[to_index];
                    break;
                }
            }
            
            //create edge
            if (to_room != NULL) {
                connect_status = graph_connect(graph, from_room, to_room);
                if (connect_status == GRAPH_STATUS_NO_MEMORY) {
                    return NO_MEMORY;
                }
            }
        }
    }

    return OK;
}

static Status start_world_and_graph(const char *config_file,
                                   Charset *charset_out,
                                   Graph **graph_out) {

    if (config_file == NULL || charset_out == NULL || graph_out == NULL) {
        return INVALID_ARGUMENT;
    }

    *graph_out = NULL;
    int datagen_status = 0;
    GraphStatus graph_create_status = GRAPH_STATUS_OK;

    //start datagen
    datagen_status = start_datagen(config_file);

    if (datagen_status == DG_ERR_CONFIG) {
        return WL_ERR_CONFIG;
    }
    if (datagen_status == DG_ERR_OOM) {
        return NO_MEMORY;
    }
    if (datagen_status != DG_OK) {
        return WL_ERR_DATAGEN;
    }

    //copy charset
    if (copy_charset(charset_out) != OK) {
        stop_datagen();
        return WL_ERR_DATAGEN;
    }

    //create graph
    graph_create_status = graph_create(compare_rooms_by_id, destroy_room_payload, graph_out);
    if (graph_create_status != GRAPH_STATUS_OK) {
        stop_datagen();
        return map_graph_status(graph_create_status);
    }

    return OK;
}

Status loader_load_world(const char *config_file,
                         Graph **graph_out,
                         Room **first_room_out,
                         int *num_rooms_out,
                         Charset *charset_out) {
    
    //graph being built
    Graph *graph = NULL;

    //temp dynamic list of Room *
    Room **room_array = NULL;
    int room_array_capacity = 0;
    int room_count = 0;

    //room from datagen
    DG_Room dg_room;

    //newly created Room
    Room *room = NULL;

    Status status = OK;

    if (config_file == NULL || graph_out == NULL || first_room_out == NULL || num_rooms_out == NULL || charset_out == NULL) {
        return INVALID_ARGUMENT;
    }

    *graph_out = NULL;
    *first_room_out = NULL;
    *num_rooms_out = 0;

    //datagen starts running and charset is copied and graph points to new empty graph
    status = start_world_and_graph(config_file, charset_out, &graph);

    if (status != OK) {
        return status;
    }

    room_array = NULL;
    room_array_capacity = 0;
    room_count = 0;

    //copying every dg_room and copying it into newly created Room
    while (has_more_rooms()) {
        dg_room = get_next_room();

        //for each room in dg_room call room_create 
        room = room_create(dg_room.id, NULL, dg_room.width, dg_room.height);

        if (room == NULL) {
            free(room_array);
            graph_destroy(graph);
            stop_datagen();
            return NO_MEMORY;
        }

        //for each room copy in floor_grid data form datagen
        status = copy_floor_grid(room, &dg_room);

        if (status != OK) {
            room_destroy(room);
            free(room_array);
            graph_destroy(graph);
            stop_datagen();
            return status;
        }

        //for each room copy in portal data form datagen
        status = copy_portals(room, &dg_room);

        if (status != OK) {
            room_destroy(room);
            free(room_array);
            graph_destroy(graph);
            stop_datagen();
            return status;
        }

        //for each room copy in treasure data + names form datagen
        status = copy_treasures(room, &dg_room);

        if (status != OK) {
            room_destroy(room);
            free(room_array);
            graph_destroy(graph);
            stop_datagen();
            return status;
        }

        status = copy_pushables(room, &dg_room);

        if (status != OK) {
            room_destroy(room);
            free(room_array);
            graph_destroy(graph);
            stop_datagen();
            return status;
        }

        status = copy_switches(room, &dg_room);
        
        if (status != OK) {
            room_destroy(room);
            free(room_array);
            graph_destroy(graph);
            stop_datagen();
            return status;
        }



        //insert room nodes into graph
        if (graph_insert(graph, room) != GRAPH_STATUS_OK) {
            room_destroy(room);
            free(room_array);
            graph_destroy(graph);
            stop_datagen();
            return WL_ERR_DATAGEN;
        }

        //the first room instered is the start room
        if (room_count == 0) {
            *first_room_out = room;
        }

        //adds rooms to room_array so know which room corresponds to each portal dest id
        //need this in order to connect rooms with edges (portals)
        status = append_room(&room_array, &room_array_capacity, &room_count, room);

        if (status != OK) {
            free(room_array);
            graph_destroy(graph);
            stop_datagen();
            return status;
        }
    }

    //create edges after all rooms have been initalized 
    status = connect_rooms(graph, room_array, room_count);

    //dont need this anymore
    stop_datagen();

    if (status != OK) {
        free(room_array);
        graph_destroy(graph);
        return status;
    }

    *graph_out = graph;
    *num_rooms_out = room_count;

    free(room_array);
    return OK;
}
