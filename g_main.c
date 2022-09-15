#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "raylib.h"
#include "raymath.h"

#define RAYMATH_IMPLEMENTATION

#define min(a,b) (a < b? a : b)

/*
 *TODO: 
 * start making a tilemap editor since I'll definitely need it
 
 *brain for enemy

 *random idea
 *enemies can get possessed by more intelligent daemons

 * -refactor all the messiness






 * handle invalid paths properly
 * one way is do a raycast
 * or some sorta flood fill outward to find a proper tile
 */

// for the love of christ keep things simple on this one.
// you're not allowed to any graphics code until you have a
// playable world. I MEAN IT
// this is a GAMEPLAY toybox.
// graphics are BAD on this one
// no shaders, no bs till the game is DONE

// done my own pathfinding, now try
// reading about A-Star
// my current pathfinding is about 2KB per screen sized tilemap/path array
// and that's actually excessive
// so we can get about 500-ish concurrent paths per mb of ram, which is flenty

// premise for this game is: Stalker / Diablo




// end the session with refactoring

// tilemaps
// they derive their power from the speed of random access on arrays
// and the fact that you can quickly compute lookups into tiles based on your world position
// so we'll have an array of different tiles, start by rendering them in different colors.

// I think I just don't like camel case

// I do have an alternative thought about pathfinding:
// a geometric approach where you 'drop a line of string' from
// your position to destination and then collision detect
// the blockages, and then relax it to fit around the obstacles



typedef enum {
    BRICKTILE,
    GRASSTILE,
    MUDTILE,
    DIRTTILE,
    NUMTILES,
} TILETYPE;

typedef unsigned int uint;

typedef struct {
    uint x;
    uint y;
} TileMapResult;

typedef struct NodeStruct {
    uint predecessor;
    uint current;
} Node;

typedef struct {
    Node neighbours[8]; // at most 
    uint count;
} NeighbourResult;

typedef struct {
    Node *nodes;
    // uint *indices; // could store the locations of each, here?
    uint count;
    uint max;
} NodeArray;

typedef struct {
    uint *indices;
    uint len;
    uint max;
} Path;

typedef struct {
    TILETYPE *tilemap;
    uint tiles_across;
    uint tiles_down;
    uint tile_width;
    uint tile_height;
} TileMap;

typedef struct {
    Vector2 position;
    Vector2 destination;
    Vector2 heading;
    uint path_counter;
    float speed;
    float max_speed;
    bool attacking;
    Path path;
    
} Player;

typedef struct {
    bool hit;
    Vector2 position;
} RaycastResult;

static TILETYPE g_tilemap[1000] = {
    1,1,1,1,1,1,1,1,1,1,    1,1,1,1,1,1,1,1,1,1,    1,1,1,1,1,1,1,1,1,1,    1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,    1,1,1,1,0,0,0,1,1,1,    1,1,1,1,1,1,1,1,1,1,    1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,    1,1,1,1,0,1,0,1,1,1,    1,1,1,1,1,1,1,1,1,1,    1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,    1,1,1,1,0,0,0,1,1,1,    1,1,1,1,1,1,1,1,1,1,    1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,    1,1,1,1,1,1,1,1,1,1,    1,1,1,1,1,1,1,1,1,1,    1,1,1,1,1,1,1,1,1,1,

    1,1,1,1,1,1,1,1,1,1,    1,1,1,1,1,1,1,1,1,1,    1,1,1,1,1,1,1,1,1,1,    1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,    1,1,1,1,1,1,1,1,1,1,    1,1,1,1,1,1,1,1,1,1,    1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,    1,1,1,1,1,1,1,1,1,1,    1,1,1,1,1,1,1,1,1,1,    1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,    1,1,1,1,1,1,1,1,1,1,    1,1,1,1,1,1,1,1,1,1,    1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,    1,1,1,1,1,1,1,1,1,1,    1,1,1,1,1,1,1,1,1,1,    1,1,1,1,1,1,1,1,1,1,

    1,1,1,1,1,1,1,1,1,1,    1,1,1,1,1,1,1,1,1,1,    1,1,1,1,1,1,1,1,1,1,    1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,    1,1,1,1,1,1,1,1,1,1,    1,1,1,1,1,1,1,1,1,1,    1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,    1,1,1,1,1,1,1,1,1,1,    1,1,1,1,1,1,1,1,1,1,    1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,    1,1,1,1,1,1,1,1,1,1,    1,1,1,1,1,1,1,1,1,1,    1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,    1,1,1,1,1,1,1,1,1,1,    1,1,1,1,1,1,1,1,1,1,    1,1,1,1,1,1,1,1,1,1,

    1,1,1,1,1,1,1,1,1,1,    1,1,1,1,1,1,1,1,1,1,    1,1,1,1,1,1,1,1,1,1,    1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,    1,1,1,1,1,1,1,1,1,1,    1,1,1,1,1,1,1,1,1,1,    1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,    1,1,1,1,1,1,1,1,1,1,    1,1,1,1,1,1,1,1,1,1,    1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,    1,1,1,1,1,1,1,1,1,1,    1,1,1,1,1,1,1,1,1,1,    1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,    1,1,1,1,1,1,1,1,1,1,    1,1,1,1,1,1,1,1,1,1,    1,1,1,1,1,1,1,1,1,1,

    1,1,1,1,1,1,1,1,1,1,    1,1,1,1,1,1,1,1,1,1,    1,1,1,1,1,1,1,1,1,1,    1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,    1,1,1,1,1,1,1,1,1,1,    1,1,1,1,1,1,1,1,1,1,    1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,    1,1,1,1,1,1,1,1,1,1,    1,1,1,1,1,1,1,1,1,1,    1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,    1,1,1,1,1,1,1,1,1,1,    1,1,1,1,1,1,1,1,1,1,    1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,    1,1,1,1,1,1,1,1,1,1,    1,1,1,1,1,1,1,1,1,1,    1,1,1,1,1,1,1,1,1,1,
    
    

};

bool out_of_screen(Vector2 pos, int screen_width, int screen_height)
{
    if (pos.x < 0.0f ||
	pos.x > screen_width ||
	pos.y < 0.0f ||
	pos.y > screen_height) {
	return true;
    }
    return false;
}

bool node_equals(Node n1, Node n2)
{
    bool result = false;

    if (n1.current == n2.current &&
	n1.predecessor== n2.predecessor) {
	result = true;
    }
    
    return result;
}

uint find_index_of_node(uint raw_index, NodeArray node_array)
{
    for (int i = 0; i < node_array.count; i++) {
	if (node_array.nodes[i].current == raw_index) {
	    return i;
	}
    }
}


TileMapResult world_space_to_tilemap(float world_x, float world_y, uint width, uint height)
{
    TileMapResult result;
    result.x = world_x / width;
    result.y = world_y / height;
    return result;
}

Vector2 tilemap_to_world_space(uint x, uint y, uint width, uint height)
{
    // correct
    Vector2 result;
    result.x = x * width + width/2.0f;
    result.y = y * height + height/2.0f;
    return result;
}

uint tilemap_to_index(TileMapResult tilemap_result, uint stride)
{
    uint result = tilemap_result.x + tilemap_result.y * stride;
    return result;
}

TileMapResult index_to_tilemap(uint index, uint stride)
{
    TileMapResult result;
    result.x = index % stride;
    result.y = index / stride;
    return result;
}

TILETYPE tilemap_to_type(TileMapResult tilemap_result, TILETYPE *tilemap, uint stride)
{
    // is that correct?
    // this should be tiles across not width
    return tilemap[tilemap_result.x + tilemap_result.y*stride];
}

TILETYPE world_space_to_type(TileMap tilemap, float world_x, float world_y)
{
    TileMapResult map_result = world_space_to_tilemap(world_x, world_y, tilemap.tile_width, tilemap.tile_height);
    TILETYPE result = tilemap_to_type(map_result, tilemap.tilemap, tilemap.tiles_across);
    return result;
}

bool free_tile(TileMapResult tilemap_result, TILETYPE *tilemap, uint stride)
{
    // seems we are allowing ourselves to pass over brick tiles?
    bool result = true;
    if (tilemap_to_type(tilemap_result, tilemap, stride) == BRICKTILE) {
	result = false;
    }
    return result;
}

bool tilemap_equals(TileMapResult t1, TileMapResult t2)
{
    if ((t1.x == t2.x) && (t1.y == t2.y)) {
	return true;
    }
    return false;
}

NeighbourResult get_neighbours(Node node, TILETYPE *tilemap, uint tiles_across, uint tiles_down)
{

    // this could be a lot shorter
    NeighbourResult result;
    TileMapResult nodeTile = index_to_tilemap(node.current, tiles_across);
    uint x = nodeTile.x;
    uint y = nodeTile.y;
    // check edge cases
    if (x == 0 && y == 0) {
	TileMapResult t0 = {.x = x + 1, .y = y};
	TileMapResult t1 = {.x = x + 1, .y = y + 1};
	TileMapResult t2 = {.x = x, .y = y + 1};
	TileMapResult tiles[3] = {t0, t1, t2};
	int added = 0;
	for (int i = 0; i < 3; i++) {
	    if (free_tile(tiles[i], tilemap, tiles_across)) {
		result.neighbours[added].current = tilemap_to_index(tiles[i], tiles_across);
		result.neighbours[added].predecessor = node.current;
		added++;
	    }
	}
	result.count = added;
    } else if (x == tiles_across && y == tiles_down) {
	TileMapResult t0 = {.x = x - 1, .y = y};
	TileMapResult t1 = {.x = x - 1, .y = y -1};
	TileMapResult t2 = {.x = x, .y = y - 1};
	TileMapResult tiles[3] = {t0, t1, t2};
	int added = 0;
	for (int i = 0; i < 3; i++) {
	    if (free_tile(tiles[i], tilemap, tiles_across)) {
		result.neighbours[added].current = tilemap_to_index(tiles[i], tiles_across);
		result.neighbours[added].predecessor = node.current;
		added++;
	    }
	}
	result.count = added;
    } else if (x == 0) {
	TileMapResult t0 = {.x = x + 1, .y = y};
	TileMapResult t1 = {.x = x + 1, .y = y + 1};
	TileMapResult t2 = {.x = x, .y = y + 1};

	TileMapResult t3 = {.x = x + 1, .y = y - 1};
	TileMapResult t4 = {.x = x, .y = y - 1};
	TileMapResult tiles[5] = {t0, t1, t2, t3, t4};
	int added = 0;
	for (int i = 0; i < 5; i++) {
	    if (free_tile(tiles[i], tilemap, tiles_across)) {
		result.neighbours[added].current = tilemap_to_index(tiles[i], tiles_across);
		result.neighbours[added].predecessor = node.current;
		added++;
	    }
	}
	result.count = added;
    } else if (x == tiles_across) {
	TileMapResult t0 = {.x = x - 1, .y = y};
	TileMapResult t1 = {.x = x - 1, .y = y + 1};
	TileMapResult t2 = {.x = x, .y = y + 1};

	TileMapResult t3 = {.x = x - 1, .y = y - 1};
	TileMapResult t4 = {.x = x, .y = y - 1};

	TileMapResult tiles[5] = {t0, t1, t2, t3, t4};
	int added = 0;
	for (int i = 0; i < 5; i++) {
	    if (free_tile(tiles[i], tilemap, tiles_across)) {
		result.neighbours[added].current = tilemap_to_index(tiles[i], tiles_across);
		result.neighbours[added].predecessor = node.current;
		added++;
	    }
	}
	result.count = added;
    } else if (y == 0) {
	TileMapResult t0 = {.x = x - 1, .y = y};
	TileMapResult t1 = {.x = x - 1, .y = y + 1};
	TileMapResult t2 = {.x = x, .y = y + 1};

	TileMapResult t3 = {.x = x + 1, .y = y};
	TileMapResult t4 = {.x = x + 1, .y = y + 1};

	TileMapResult tiles[5] = {t0, t1, t2, t3, t4};
	int added = 0;
	for (int i = 0; i < 5; i++) {
	    if (free_tile(tiles[i], tilemap, tiles_across)) {
		result.neighbours[added].current = tilemap_to_index(tiles[i], tiles_across);
		result.neighbours[added].predecessor = node.current;
		added++;
	    }
	}
	result.count = added;
    } else if (y == tiles_down) {
	TileMapResult t0 = {.x = x - 1, .y = y};
	TileMapResult t1 = {.x = x - 1, .y = y - 1};
	TileMapResult t2 = {.x = x, .y = y - 1};

	TileMapResult t3 = {.x = x + 1, .y = y};
	TileMapResult t4 = {.x = x + 1, .y = y - 1};

	TileMapResult tiles[5] = {t0, t1, t2, t3, t4};
	int added = 0;
	for (int i = 0; i < 5; i++) {
	    if (free_tile(tiles[i], tilemap, tiles_across)) {
		result.neighbours[added].current = tilemap_to_index(tiles[i], tiles_across);
		result.neighbours[added].predecessor = node.current;
		added++;
	    }
	}
	result.count = added;
    } else {
	TileMapResult t0 = {.x = x - 1, .y = y};
	TileMapResult t1 = {.x = x - 1, .y = y - 1};
	TileMapResult t2 = {.x = x - 1, .y = y + 1};
	TileMapResult t3 = {.x = x, .y = y - 1};
	TileMapResult t4 = {.x = x, .y = y + 1};
	TileMapResult t5 = {.x = x + 1, .y = y};
	TileMapResult t6 = {.x = x + 1, .y = y - 1};
	TileMapResult t7 = {.x = x + 1, .y = y + 1};

	TileMapResult tiles[8] = {t0, t1, t2, t3, t4, t5, t6, t7};
	int added = 0;
	for (int i = 0; i < 8; i++) {
	    if (free_tile(tiles[i], tilemap, tiles_across)) {
		result.neighbours[added].current = tilemap_to_index(tiles[i], tiles_across);
		result.neighbours[added].predecessor = node.current;
		added++;
	    }
	}
	result.count = added;
    }
	
    
    
    return result;
}

bool contains_node(NodeArray nodes, Node node, uint added)
{
    bool result = false;
    
    for (int i = 0; i < added; i++) {
	// i think we just want to see if the current is the same?
	if (nodes.nodes[i].current == node.current) {
	    return true;
	}
    }
    
    return false;
}

bool add_node(Node node, NodeArray *nodes)
{
    if (nodes->count >= nodes->max) {
	return false;
    }
    nodes->nodes[nodes->count] = node;
    nodes->count++;
    
    return true;
}

Node tilemap_to_terminal_node(TileMapResult tilemap, uint tiles_across)
{
    // turn a tilemap into a terminal node
    Node result;
    uint index = tilemap_to_index(tilemap, tiles_across);
    result.current = index;
    result.predecessor = index;

    return result;
}

TileMapResult find_next_free_tile(TileMapResult pos, TILETYPE *tilemap, uint tiles_across, uint tiles_down)
{
    //get index of current node?

    int rect_radius = 1;

    // increase the radius outwards


    bool found = false;
    TileMapResult bogusResult = pos;
    // not efficient yet
    while (!found) {
	for (int x = -rect_radius; x <= rect_radius; x++) {
	    for (int y = -rect_radius; y <= rect_radius; y++) {
		TileMapResult toTest = {.x = pos.x + x, .y = pos.y + y};
		if (free_tile(toTest, tilemap, tiles_across)) {
		    return toTest;
		}
	    }
	
	}
	rect_radius += 1;
	if (rect_radius >= tiles_across || rect_radius >= tiles_down) {
	    found = true;
	}
    }

    // top left, top right
    // bottom left, bottom right
    // loop between them, how?
    return bogusResult;
}



int flood_fill_to_destination(TileMapResult start, TileMapResult end, TILETYPE *tilemap, NodeArray *nodes, uint tiles_across, uint tiles_down)
{
    // potential issue; will this tunnel through walls?
    int result = -1;
    
    // clear the NodeArray for use
    nodes->count = 0;
    
    // i don't think height is coming into play

    // we should do a check to see if the end point is actually a valid tile, and if not, pick a nearby one


    // check if end node is valid
    if (!free_tile(end, tilemap, tiles_across)) {
	// find next free tile
	end = find_next_free_tile(end, tilemap, tiles_across, tiles_down);
    }
    

    Node start_node = tilemap_to_terminal_node(start, tiles_across);
    Node end_node = tilemap_to_terminal_node(end, tiles_across);
    if (node_equals(start_node, end_node)) {
	// this case should be handled correctly
    }
    
    uint nodes_added = 0;
    uint nodes_searched = 0;
    add_node(start_node, nodes);
    nodes_added++;

    Node end_reconstruct;

    bool destination_found = false;

    // possible off by one error here
    while (!destination_found && nodes_searched < (nodes->max)) {
	// I think what we want to do is check if no nodes were added
	// since last time, if so we should stop THEN
	Node current = nodes->nodes[nodes_searched];

	if (current.current == end_node.current) {
	    destination_found = true;
	    add_node(current, nodes);
	    nodes_added++;
	    break;
	}
	
	//
	NeighbourResult neighbour_result = get_neighbours(current, tilemap, tiles_across, tiles_down);
	for (int i = 0; i < neighbour_result.count; i++) {
	    Node new = neighbour_result.neighbours[i];	    
	    if (!contains_node(*nodes, new, nodes_added)) {
		add_node(new, nodes);
		nodes_added++;
	    }
	}
	nodes_searched++;

    }

    if (destination_found) {
	result = nodes_searched;
    }

    return result;
}

void reconstruct_path(Path *path, NodeArray nodes, TileMapResult start, int end_index, uint tiles_across)
{
    
    Node current_node = nodes.nodes[end_index];
    uint path_len = 0;
    bool counted_path = false;
    uint start_index = tilemap_to_index(start, tiles_across);
    // we need to figure out how long the path will actually be
    while (!counted_path) {
	if (current_node.current == start_index) {
	    counted_path = true;
	} else {
	    path_len++;
	    // we could instead store the position when we place it, would be better
	    current_node = nodes.nodes[find_index_of_node(current_node.predecessor, nodes)];
	}
    }
    
    path->len = path_len;
    current_node = nodes.nodes[end_index];
    for (int i = path_len; i >= 0; i--) {
	path->indices[i] = current_node.current;
	current_node = nodes.nodes[find_index_of_node(current_node.predecessor, nodes)];
    }
    // so we can THEN do a countdown-add to the path, set its length appropriately and use that later
}

void update_player(Player *player, float dt, NodeArray *node_array, TILETYPE *tilemap)
{
}

void draw_tilemap(TileMap tilemap)
{
    // slower but easier to read
    // and compiler should clean it up anyway
    uint tiles_down = tilemap.tiles_down;
    uint tiles_across = tilemap.tiles_across;
    uint tile_width = tilemap.tile_width;
    uint tile_height = tilemap.tile_height;
    
    for (int y = 0; y < tiles_down; y++) {
	for (int x = 0; x < tiles_across; x++) {
	    TILETYPE tile = tilemap.tilemap[y*tiles_across + x];
	    if (tile == GRASSTILE) {
		DrawRectangle(x * tile_width, y * tile_height, tile_width, tile_height, GREEN);
	    }
	    if (tile == BRICKTILE) {
		DrawRectangle(x * tile_width, y * tile_height, tile_width, tile_height, PINK);
	    }
	    if (tile == MUDTILE) {
		DrawRectangle(x * tile_width, y * tile_height, tile_width, tile_height, DARKBROWN);
	    }
	    if (tile == DIRTTILE) {
		DrawRectangle(x * tile_width, y * tile_height, tile_width, tile_height, BEIGE);
	    }
	}
    }
}

void update_player();

void update_enemies();

void draw_player();

void draw_enemies();

void draw_ui();

int save_map();

int load_map();

RaycastResult raycast(TileMap tilemap, Vector2 start, Vector2 direction, Vector2 target, float length, int screen_width, int screen_height)
{
    RaycastResult result = {.position = {.x = start.x, .y = start.y},
	.hit = false};

    TileMapResult target_tile = world_space_to_tilemap(target.x, target.y, tilemap.tile_width, tilemap.tile_height);
    for (float t = 0.0f; t += 0.5f; t <= length) {
	// maybe better to start at .5?
	result.position = Vector2Add(start, Vector2Scale(direction, t));
	if (out_of_screen(result.position, screen_width, screen_height)) {
	    return result;
	}
	// need to check for casting outside the screen
	TileMapResult position_tile = world_space_to_tilemap(result.position.x, result.position.y, tilemap.tile_width, tilemap.tile_height);
	
	TILETYPE tile = world_space_to_type(tilemap, result.position.x, result.position.y);
	if (tile == BRICKTILE) {
	    return result;
	}
	if (tilemap_equals(position_tile, target_tile)) {
	    result.hit = true;
	    return result;
	}
    }
    return result;	
}

int main(int argc, char **argv)
{
    const int screen_width = 1280;
    const int screen_height = 800;
    
    float game_timer = 0.0f;
    
    SetWindowState(FLAG_VSYNC_HINT);
    InitWindow(screen_width, screen_height, "Through A Dark Meadow");
    HideCursor();

    SetTargetFPS(60);               // Set our game to run at 60 frames-per-second

    InitAudioDevice();
    
    float target_ms_per_frame = 1.0f/60.0f;

    Vector2 player_position = {.x = screen_width/2.0f, .y = screen_height/2.0f};
    Vector2 player_destination = {.x = player_position.x, .y = player_position.y};
    Vector2 player_velocity = {.x = 0.0f, .y = 0.0f};
    Vector2 player_heading= {.x = 0.0f, .y = 0.0f};

    Vector2 enemy_position = {.x = 600.0f, .y = 400.0f};

    int enemy_health = 100;

    bool player_attacking = false;

    float player_attack_radius = 20.0f;

    Sound rainSound = LoadSound("sound/rain-07.wav");

    uint tile_width = 32;
    uint tile_height = 32;    
    //uint tiles_across = screen_width / tile_width;
    //uint tiles_down = screen_height / tile_height;

    uint tiles_across = 40;
    uint tiles_down = 25;
    
    TILETYPE *tilemap = malloc(sizeof *tilemap * tiles_across * tiles_down);
    for (int j = 0; j < tiles_down; j++) {
	for (int i = 0; i < tiles_across; i++) {
	    int rand_tile = rand() % NUMTILES;
	    //tilemap[j*tiles_across + i] = rand_tile;
	    tilemap[j*tiles_across + i] = g_tilemap[j*tiles_across + i];
	}
    }

    TileMap tilemap_t = {.tilemap = tilemap,
	.tiles_across = tiles_across,
	.tiles_down = tiles_down,
	.tile_width = tile_width,
	.tile_height = tile_height
    };

    // some test destinations for the enemy navigation
    
    NodeArray node_array;
    node_array.count = 0;
    node_array.max = tiles_across * tiles_down;
    node_array.nodes = malloc(node_array.max * sizeof(Node));
    for (int i = 0; i < node_array.max; i++) {
	node_array.nodes[i] = (Node){.predecessor = 0, .current = 0};
    }

    Path path_array;
    Path player_path_array;
    path_array.indices = malloc(sizeof(uint) * tiles_across * tiles_down);
    path_array.max = tiles_across * tiles_down;
    path_array.len = 0;

    player_path_array.indices = malloc(sizeof(uint) * tiles_across * tiles_down);    
    player_path_array.max = tiles_across * tiles_down;
    player_path_array.len = 0;
    for (int i = 0; i < player_path_array.max; i++) {
	player_path_array.indices[i] = 0;
    }
    
    // Vector2 enemyDestinationTest = {.x = 550.0f, .y = 550.0f};
    Vector2 enemy_destination_test = {.x = 32.0f, .y = 320.0f};
    int flood_result = flood_fill_to_destination(world_space_to_tilemap(enemy_position.x, enemy_position.y, tile_width, tile_height),
						 world_space_to_tilemap(enemy_destination_test.x, enemy_destination_test.y, tile_width, tile_height),
						 tilemap,
						 &node_array, tiles_across, tiles_down);
    if (flood_result != -1) {
	reconstruct_path(&path_array, node_array, world_space_to_tilemap(enemy_position.x, enemy_position.y, tile_width, tile_height),
		     flood_result, tiles_across);
    }
    
    uint enemy_path_counter = 0;

    uint player_path_counter = 0;

    bool editing_mode = false;
    bool recently_shifted_modes = false;
    bool save_mode = false;
    float recent_timer = 0.0f;
    float recent_timer_max = 2.0f;

    char filename_buffer[512];    
    for (int i = 0; i < 512; i++) {
	filename_buffer[i] = '\0';
    }
    char *default_filename = "new";
    uint filename_counter = 0;

    bool load_mode = false;

    while (!WindowShouldClose())    // Detect window close button or ESC key
    {
	float dt = GetFrameTime();
	float dtToUse = min(dt, target_ms_per_frame);
	// sound stuff
	if (!IsSoundPlaying(rainSound)) {
	    PlaySound(rainSound);
	}

	if (IsKeyPressed(KEY_E) && !save_mode) {
	    editing_mode = !editing_mode;
	    recently_shifted_modes = true;
	    recent_timer = 0.0f;
	}

	if (IsKeyPressed(KEY_S) && editing_mode && !save_mode) {
	    // need to perform a save
	    // prompt for filename?
	    // that comes later
	    save_mode = true;
	    memcpy(filename_buffer, default_filename, strlen(default_filename));
	    filename_counter = strlen(default_filename) - 1;
	    while ((GetCharPressed()) != 0) {
		// clear the input buffer
	    }
	}

	if (IsKeyPressed(KEY_L) && editing_mode && !save_mode) {
	    load_mode = true;
	    while ((GetCharPressed()) != 0) {
		// clear the input bufffer
	    }
	}

	if (load_mode) {
	    if (IsKeyPressed(KEY_BACKSPACE)) {
		
		filename_buffer[filename_counter] = '\0';
		if (filename_counter > 0) {
		    filename_counter--;
		}
	    }
	    int key_pressed = 0;
	    while ((key_pressed = GetCharPressed()) != 0) {
		filename_buffer[filename_counter] = key_pressed;
		filename_counter++;
	    }
	    
	    if (IsKeyPressed(KEY_ENTER)) {
		// do a save
		FILE *save_fp;
		
		errno_t err = fopen_s(&save_fp, filename_buffer, "rb+");
		if (err) {
		    printf("something went wrong\n");
		}

		// I think we need to write individually
		fread((void*)&(tilemap_t.tiles_across), sizeof(tilemap_t.tiles_across), 1, save_fp);
		//fseek(save_fp, sizeof(tilemap_t.tiles_across), SEEK_CUR);
		fread((void*)&(tilemap_t.tiles_down), sizeof(tilemap_t.tiles_down), 1, save_fp);
		//fseek(save_fp, sizeof(tilemap_t.tiles_down), SEEK_CUR);
		fread((void*)&(tilemap_t.tile_width), sizeof(tilemap_t.tile_width), 1, save_fp);
		//fseek(save_fp, sizeof(tilemap_t.tile_width), SEEK_CUR);
		fread((void*)&(tilemap_t.tile_height), sizeof(tilemap_t.tile_height), 1, save_fp);
		//fseek(save_fp, sizeof(tilemap_t.tile_height), SEEK_CUR);
		fread((void*)(tilemap_t.tilemap), tilemap_t.tiles_across*tilemap_t.tiles_down*sizeof(TILETYPE), 1, save_fp);
		
		fclose(save_fp);
		load_mode = false;
	    }
	}

	if (save_mode) {

	    if (IsKeyPressed(KEY_BACKSPACE)) {
		
		filename_buffer[filename_counter] = '\0';
		if (filename_counter > 0) {
		    filename_counter--;
		}
	    }
	    int key_pressed = 0;
	    while ((key_pressed = GetCharPressed()) != 0) {
		filename_buffer[filename_counter] = key_pressed;
		filename_counter++;
	    }

	    
	    if (IsKeyPressed(KEY_ENTER)) {
		// do a save
		FILE *save_fp;
		
		errno_t err = fopen_s(&save_fp, filename_buffer, "wb+");
		if (err) {
		    printf("something went wrong\n");
		}

		// I think we need to write individually
		fwrite((void*)&(tilemap_t.tiles_across), sizeof(tilemap_t.tiles_across), 1, save_fp);
		fwrite((void*)&(tilemap_t.tiles_down), sizeof(tilemap_t.tiles_down), 1, save_fp);
		fwrite((void*)&(tilemap_t.tile_width), sizeof(tilemap_t.tile_width), 1, save_fp);
		fwrite((void*)&(tilemap_t.tile_height), sizeof(tilemap_t.tile_height), 1, save_fp);
		fwrite((void*)(tilemap_t.tilemap), tilemap_t.tiles_across*tilemap_t.tiles_down*sizeof(TILETYPE), 1, save_fp);
		
		fclose(save_fp);
		
		save_mode = false;
	    }
	    
	}


	// really need an 'update world' function here

        
        
        
        float player_speed = 20.0f;
        float max_speed = 150.0f;
        
        player_attacking = false;
       
        struct Vector2 mouse_pos = {.x = GetMouseX(), .y = GetMouseY()};


       
        if (IsMouseButtonPressed(0)) {

	    if (editing_mode) {
		TileMapResult tilemap_to_change = world_space_to_tilemap(mouse_pos.x, mouse_pos.y, tile_width, tile_height);
		tilemap_t.tilemap[tilemap_to_change.y * tiles_across + tilemap_to_change.x] = (tilemap_t.tilemap[tilemap_to_change.y * tiles_across + tilemap_to_change.x] + 1) % NUMTILES;
	    } else {
	    
		player_path_counter = 0;
		player_destination = (Vector2){.x = mouse_pos.x, .y = mouse_pos.y};
	    
		// maybe not the most efficient thing to do?
		TileMapResult tempTileMapDest = world_space_to_tilemap(player_destination.x, player_destination.y, tile_width, tile_height);
		TileMapResult tempTileMapCurrent = world_space_to_tilemap(player_position.x, player_position.y, tile_width, tile_height);
		node_array.count = 0;
		int floodResult = flood_fill_to_destination(tempTileMapCurrent, tempTileMapDest, tilemap, &node_array, tiles_across, tiles_down);
		if (floodResult != -1) {

		    reconstruct_path(&player_path_array, node_array, tempTileMapCurrent, floodResult, tiles_across);
		} else {
		    // set should move = false?

		}
	    }
            
            
            
        }	
	if (IsMouseButtonPressed(1)) {
	    player_attacking = true;
            
        }

	
        
	bool player_should_move = true;
    
        if (Vector2Distance(player_position, player_destination) > 3.0f && player_should_move) {
	    TileMapResult player_path_tile = index_to_tilemap(player_path_array.indices[player_path_counter], tiles_across);
	    Vector2 player_path_vec = tilemap_to_world_space(player_path_tile.x, player_path_tile.y , tile_width, tile_height);
	    player_heading = Vector2Subtract(player_path_vec, player_position);
	    player_heading = Vector2Normalize(player_heading);
	    if (Vector2Distance(player_position, player_path_vec) > 3.0f) {
		player_position = Vector2Add(player_position, Vector2Scale(player_heading, dtToUse * 50.0f));
	    }
	    if (Vector2Distance(player_position, player_path_vec) < 3.0f && player_path_counter < player_path_array.len) {
		player_path_counter++;
	    }
		

	
        } else {
	    player_velocity = (Vector2){.x = 0.0f, .y = 0.0f};
	}

	TileMapResult player_tilemap = world_space_to_tilemap(player_position.x, player_position.y, tile_width, tile_height);
	TileMapResult mouse_tilemap = world_space_to_tilemap(mouse_pos.x, mouse_pos.y, tile_width, tile_height);
	TileMapResult destination_tilemap = world_space_to_tilemap(player_destination.x, player_destination.y, tile_width, tile_height);

	if ((Vector2Distance(player_position, enemy_position) < player_attack_radius)
	    && player_attacking) {
	    enemy_health--;
	}

	
            
        //dt -= dtToUse;
        //game_timer += dtToUse;
            
        
        
        TileMapResult enemy_path_tile = index_to_tilemap(path_array.indices[enemy_path_counter], tiles_across);


	
	Vector2 enemy_path_vec = tilemap_to_world_space(enemy_path_tile.x, enemy_path_tile.y, tile_width, tile_height);
	Vector2 enemy_heading_vec = Vector2Subtract(enemy_path_vec, enemy_position);

	TileMapResult enemy_current_tile = world_space_to_tilemap(enemy_position.x, enemy_position.y, tile_width, tile_height);
	enemy_heading_vec = Vector2Normalize(enemy_heading_vec);
	if (Vector2Distance(enemy_position, enemy_path_vec) > 3.0f) {
	    enemy_position = Vector2Add(enemy_position, Vector2Scale(enemy_heading_vec, dtToUse * 50.0f));
	}
	if (Vector2Distance(enemy_position, enemy_path_vec) < 3.0f && enemy_path_counter < path_array.len) {
	    enemy_path_counter++;
	}

	RaycastResult enemy_raycast = raycast(tilemap_t, enemy_position, Vector2Normalize(Vector2Subtract(player_position, enemy_position)), player_position, screen_height, screen_width, screen_height);
        
       

        // Draw
        //----------------------------------------------------------------------------------
     
        BeginDrawing();
        
        
     
        ClearBackground(RED);
        
        
        
        
        
       
        
        
     
        
        DrawFPS(30, 30);

	draw_tilemap(tilemap_t);
	for (int i = 0; i <= path_array.len; i++) {
	    TileMapResult debugTile = index_to_tilemap(path_array.indices[i], tiles_across);
	    DrawRectangleLines(debugTile.x * tile_width, debugTile.y * tile_height, tile_width, tile_height, YELLOW);
	}
	
	DrawRectangleLines(enemy_current_tile.x * tile_width, enemy_current_tile.y * tile_height, tile_width, tile_height, RED);
	DrawRectangleLines(player_tilemap.x * tile_width, player_tilemap.y * tile_height, tile_width, tile_height, RED);
	DrawRectangleLines(mouse_tilemap.x * tile_width, mouse_tilemap.y * tile_height, tile_width, tile_height, YELLOW);
	DrawRectangleLines(destination_tilemap.x * tile_width, destination_tilemap.y * tile_height, tile_width, tile_height, PINK);

	DrawCircle(player_position.x, player_position.y, 10, RAYWHITE);

	
	DrawCircle(enemy_position.x, enemy_position.y, 10, BLUE);
        
        
        DrawCircleLines(mouse_pos.x, mouse_pos.y, 5, LIGHTGRAY);

	if (enemy_raycast.hit) {
	    DrawLine(enemy_position.x, enemy_position.y, enemy_raycast.position.x, enemy_raycast.position.y, RED);
	} else {
	    DrawLine(enemy_position.x, enemy_position.y, enemy_raycast.position.x, enemy_raycast.position.y, YELLOW);
	}

	for (int i = 0; i < enemy_health; i++) {
	    DrawCircle(10 + i*5, 10, 3, RAYWHITE);
	    //
	}


	// debug drawing
	if (recently_shifted_modes) {
	    recent_timer += dtToUse;
	    if (editing_mode) {
		DrawText("editing mode", 10, 10, 14, BLACK);
	    } else {
		DrawText("not editing mode", 10, 10, 14, BLACK);
	    }

	    if (recent_timer >= recent_timer_max) {
		recently_shifted_modes = false;
	    }
	}
            
        if (save_mode || load_mode) {
	    DrawText(filename_buffer, screen_width - 100, 10, 14, BLACK);
	}
            
           
                
               
             
      

        EndDrawing();
        //----------------------------------------------------------------------------------
    }

    // De-Initialization
    //--------------------------------------------------------------------------------------
    CloseWindow();        // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    
    return 0;
}
