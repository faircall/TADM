#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "raylib.h"
#include "raymath.h"

#define RAYMATH_IMPLEMENTATION

#define min(a,b) (a < b? a : b)

/*
 *TODO: 
 * COMBAT (second) pass:
 * fix fleeing
 * heavy/light attacks?
 * we want blocking
 * we maybe want dodging (double click?)
 


 * make a 'vision cone' function 

 * long term passage of time- keep track of 'years', and split to seasons
 * track temperature/weather for survival aspect
 
 *footsteps/sound propagation

 *brain for enemy

 *maybe we want enemies to be able to get 'lost' in unfamiliar areas

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

typedef enum {
    RESTING,
    PATROLLING,
    SEEKING,
    ATTACKING,
    FLEEING,
    NUMBRAINSTATES,
} BRAINSTATE;

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
    Vector2 velocity;
    uint path_counter;
    float speed;
    float max_speed;
    bool attacking;
    Path path;
    bool should_move;
    int health;
    float attack_timer;
    float attack_timer_max;
} Player;

typedef struct {
    Vector2 position;
    Vector2 destination;
    Vector2 heading;
    uint path_counter;
    float speed;
    float max_speed;
    bool attacking;
    Path path;
    BRAINSTATE brain_state;
    int health;
    float attack_radius;
    Vector2 last_known_player_position;
    float attack_timer;
    float rest_timer;
    float attack_timer_max;
    bool has_seeking_path;
    bool alive;
    float fleeing_timer;
    float fleeing_timer_max;
} Enemy;

typedef struct {
    bool hit;
    Vector2 position;
} RaycastResult;

// can't see a reason to not make these global for now
const int screen_width = 1280;
const int screen_height = 800;

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

float angle_from_vec2(Vector2 vec)
{
    // so which quadrant are we in
    Vector2 normalized = Vector2Normalize(vec);
    float angle_x = acos(normalized.x);
    float angle_y = asin(normalized.y);


    // acos is between 0 and pi, so it tells us if we're in quad 1/4 or 2/3
    if (angle_x < PI/2) {
	// quadrant 1
	if (angle_y >= 0) {
	    return angle_x;
	}
	else {
	    // quadrant 4
	    return angle_y;
	}
    } else {
	// quadrant 2
	if (angle_y >= 0) {
	    return angle_x;
	}
	else {
	    // quadrant 3
	    return (PI - angle_y);
	
	}
    }

}

float deg_to_rad(float deg)
{
    return (deg / 180.0f) * PI;
}

float rad_to_deg(float rad)
{
    return (rad * 180.0f) / PI;
}

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

bool pathfind_to_destination(Path *path, TileMap tilemap, NodeArray *node_array, Vector2 position, Vector2 destination)
{
    // should we instead take an entity so we always set its path_counter to zero?
    // or incorporate that into the path itself?
    node_array->count = 0;
    int flood_result = flood_fill_to_destination(world_space_to_tilemap(position.x, position.y, tilemap.tile_width, tilemap.tile_height),
						 world_space_to_tilemap(destination.x, destination.y, tilemap.tile_width, tilemap.tile_height),
						 tilemap.tilemap,
						 node_array, tilemap.tiles_across, tilemap.tiles_down);
    if (flood_result != -1) {
	reconstruct_path(path, *node_array, world_space_to_tilemap(position.x, position.y, tilemap.tile_width, tilemap.tile_height),
			 flood_result, tilemap.tiles_across);
	return true;
    } else {
	return false;
    }
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


// smart thing to do would be a bresenham line algo
// instead over tiles
RaycastResult raycast(TileMap tilemap, Vector2 start, Vector2 direction, Vector2 target, float length, int screen_width, int screen_height)
{
    RaycastResult result = {.position = {.x = start.x, .y = start.y},
	.hit = false};

    TileMapResult target_tile = world_space_to_tilemap(target.x, target.y, tilemap.tile_width, tilemap.tile_height);

    // no need to do subpixel, surely
    for (float t = 0.0f; t += 1.0f; t <= length) {
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

BRAINSTATE update_brain(TileMap tilemap, NodeArray *node_array, Enemy *enemy, Player *player, float dt)
{
    // so we want knowledge of the world,
    // which is contained in the tilemap, the player, and the enemy
    // eventually will contain all entities in the world, the geometry...
    // sounds as well I suppose
    // and a lightmap (which is just a tilemap in disguise, at this stage)

    // if you can see the player, that will take priority over everything
    // attempt a raycast to player

    // how will we do a vision cone, exactly?
    // cast a ray from enemy position in the direction that it's heading,
    // and do a fov based on that
    // float angle_of_ray_x = acos(enemy->heading.x);
    // float angle_of_ray_y = asin(enemy->heading.y);
    float ray_angle_to_use = angle_from_vec2(enemy->heading);
    bool can_see_player = false;
    
    // update some statuses based on what we can see initially
    for (float angle = -15.0f; angle < 15.0f; angle += 2.0f) {
	// switching sin and cos breaks the program at runtime...?
	Vector2 ray_direction = {.x = cos(ray_angle_to_use + deg_to_rad(angle)), .y = sin(ray_angle_to_use + deg_to_rad(angle))};
	RaycastResult enemy_raycast = raycast(tilemap, enemy->position, ray_direction, player->position, screen_height, screen_width, screen_height);
	if (enemy_raycast.hit) {
	    // we can see him and he's close
	    can_see_player = true;
	    enemy->last_known_player_position = player->position;
	    if (Vector2Distance(enemy->position, player->position) < enemy->attack_radius) {
		// there should be a swing, rather than instant damage
		if (enemy->brain_state != ATTACKING) {
		    enemy->brain_state = ATTACKING;
		    enemy->attack_timer = 0.0f;
		}
	    } else {
		// we can see him but he's far
		enemy->brain_state = SEEKING;
		enemy->last_known_player_position = player->position;
	    }
	    DrawLine(enemy->position.x, enemy->position.y, enemy_raycast.position.x, enemy_raycast.position.y, RED);
	} else {
	    DrawLine(enemy->position.x, enemy->position.y, enemy_raycast.position.x, enemy_raycast.position.y, YELLOW);
	}
    }

    int health_to_flee = 3;


    switch (enemy->brain_state) {
    case RESTING: {
	    enemy->rest_timer = (enemy->rest_timer + dt);
	    // think about a proper value
	    if (enemy->rest_timer > 2.0f) {
		enemy->brain_state = PATROLLING;
		enemy->destination = (Vector2){.x = 32.0f, .y = 320.0f};
		enemy->path_counter = 0;
		bool pathfind_success = pathfind_to_destination(&(enemy->path), tilemap, node_array, enemy->position, enemy->destination);
		// TODO: handle pathfind fail

	    }
	    break;
    }
    case PATROLLING: {	
	    TileMapResult enemy_path_tile = index_to_tilemap(enemy->path.indices[enemy->path_counter], tilemap.tiles_across);


	
	    Vector2 enemy_path_vec = tilemap_to_world_space(enemy_path_tile.x, enemy_path_tile.y, tilemap.tile_width, tilemap.tile_height);
	    enemy->heading = Vector2Subtract(enemy_path_vec, enemy->position);

	    TileMapResult enemy_current_tile = world_space_to_tilemap(enemy->position.x, enemy->position.y, tilemap.tile_width, tilemap.tile_height);
	    enemy->heading = Vector2Normalize(enemy->heading);
	    if (Vector2Distance(enemy->position, enemy_path_vec) > 3.0f) {
		enemy->position = Vector2Add(enemy->position, Vector2Scale(enemy->heading, dt * 50.0f));
	    }
	    if (Vector2Distance(enemy->position, enemy_path_vec) < 3.0f && enemy->path_counter < enemy->path.len) {
		enemy->path_counter++;
	    }
	    break;
	}
    case SEEKING:
	{
	    if (can_see_player) {
		// go straight towards player
		enemy->heading = Vector2Normalize(Vector2Subtract(player->position, enemy->position));
		enemy->position = Vector2Add(enemy->position, Vector2Scale(enemy->heading, dt * 50.0f));
	    } else {
		// should do a proper pathfind here so we don't clip geometry
		enemy->heading = Vector2Normalize(Vector2Subtract(enemy->last_known_player_position, enemy->position));
		enemy->position = Vector2Add(enemy->position, Vector2Scale(enemy->heading, dt * 50.0f));
		if (Vector2Distance(enemy->last_known_player_position, enemy->position) < 5.0f) {
		    enemy->brain_state = RESTING;
		    enemy->rest_timer = 0.0f;
		}
		//if (enemy->has_seeking_path) {
		    // continue on path
		//} else {
		    // make a path

		//}
	    }
	    // move
	    break;
	}
    case ATTACKING:
	if (can_see_player) {
	    enemy->attack_timer = (enemy->attack_timer + dt);
	    if (enemy->attack_timer >= enemy->attack_timer_max) {
		enemy->attack_timer = 0.0f;
		if (Vector2Distance(enemy->position, player->position) < (enemy->attack_radius)) {
		    // make this damage variable later etc
		    player->health = player->health - 1;
		}
	    }
	} else {
	    enemy->brain_state = SEEKING;
	}
	if (enemy->health <= health_to_flee) {
	    
	    enemy->brain_state = FLEEING;
	}
	break;
    case FLEEING:

	enemy->heading = Vector2Normalize(Vector2Subtract(enemy->last_known_player_position, enemy->position));
	enemy->heading = Vector2Scale(enemy->heading, -1.0f);
	if (!out_of_screen(Vector2Add(enemy->position, Vector2Scale(enemy->heading, dt * 50.0f)), screen_width, screen_height)) {	    
	    enemy->position = Vector2Add(enemy->position, Vector2Scale(enemy->heading, dt * 50.0f));
	}
	if (!can_see_player) {
	    enemy->fleeing_timer += dt;
	    if (enemy->fleeing_timer > enemy->fleeing_timer_max) {
		enemy->rest_timer = 0.0f;
		enemy->brain_state = RESTING;
	    }
	}
	break;
    }
    // if we want, this will let us know the enemies last brain state
    // but maybe instead we have an error state?
    return enemy->brain_state;    
}

int main(int argc, char **argv)
{

    
    float game_timer = 0.0f;
    
    SetWindowState(FLAG_VSYNC_HINT);
    InitWindow(screen_width, screen_height, "Through A Dark Meadow");
    HideCursor();

    SetTargetFPS(60);               // Set our game to run at 60 frames-per-second

    InitAudioDevice();
    
    float target_ms_per_frame = 1.0f/60.0f;

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

    Player player;
    player.position = (Vector2){.x = screen_width/2.0f, .y = screen_height/2.0f};
    player.destination = (Vector2){.x = player.position.x, .y = player.position.y};
    player.velocity = (Vector2){.x = 0.0f, .y = 0.0f};
    player.heading = (Vector2){.x = 0.0f, .y = 0.0f};
    player.attacking = false;
    player.path.indices = malloc(sizeof(uint) * tiles_across * tiles_down);
    player.path.max = tiles_across * tiles_down;
    player.path.len = 0;
    player.path_counter = 0;
    for (int i = 0; i < player.path.max; i++) {
	player.path.indices[i] = 0;
    }
    player.health = 10;
    player.attack_timer = 0.0f;
    // maybe half a second?
    player.attack_timer_max = 0.5f;
	

    Enemy enemy;
    enemy.position = (Vector2){.x = 600.0f, .y = 400.0f};
    enemy.health = 10;
    enemy.attacking = false;
    enemy.destination = (Vector2){.x = 32.0f, .y = 320.0f};
    enemy.path.indices = malloc(sizeof(uint) * tiles_across * tiles_down);
    enemy.path.max = tiles_across * tiles_down;
    enemy.path.len = 0;
    enemy.path_counter = 0;
    for (int i = 0; i < enemy.path.max; i++) {
	enemy.path.indices[i] = 0;
    }
    enemy.attack_radius = 20.0f;
    enemy.attack_timer_max = 1.0f;
    enemy.attack_timer = 0.0f;
    enemy.has_seeking_path = false;
    enemy.brain_state = PATROLLING;
    enemy.rest_timer = 0.0f;
    enemy.fleeing_timer = 0.0f;
    enemy.fleeing_timer_max = 3.0f;

    int flood_result = flood_fill_to_destination(world_space_to_tilemap(enemy.position.x, enemy.position.y, tile_width, tile_height),
						 world_space_to_tilemap(enemy.destination.x, enemy.destination.y, tile_width, tile_height),
						 tilemap,
						 &node_array, tiles_across, tiles_down);
    if (flood_result != -1) {
	reconstruct_path(&enemy.path, node_array, world_space_to_tilemap(enemy.position.x, enemy.position.y, tile_width, tile_height),
		     flood_result, tiles_across);
    }

    while (!WindowShouldClose())    // Detect window close button or ESC key
    {
	float dt = GetFrameTime();
	float dt_to_use = min(dt, target_ms_per_frame);
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

        
        
        
        float player_speed = 150.0f;
        float max_speed = 150.0f;
        

       
        struct Vector2 mouse_pos = {.x = GetMouseX(), .y = GetMouseY()};


       
        if (IsMouseButtonPressed(0)) {

	    if (editing_mode) {
		TileMapResult tilemap_to_change = world_space_to_tilemap(mouse_pos.x, mouse_pos.y, tile_width, tile_height);
		tilemap_t.tilemap[tilemap_to_change.y * tiles_across + tilemap_to_change.x] = (tilemap_t.tilemap[tilemap_to_change.y * tiles_across + tilemap_to_change.x] + 1) % NUMTILES;
	    } else {
	    
		player.path_counter = 0;
		player.destination = (Vector2){.x = mouse_pos.x, .y = mouse_pos.y};
		bool pathfind_success = pathfind_to_destination(&(player.path), tilemap_t, &node_array, player.position, player.destination);
		// TODO: handle bad pathfind
	    }
            
            
            
        }	
	if (IsMouseButtonPressed(1)) {
	    player.attacking = true;
            
        }

	
        // TODO: when should this be false?
	player.should_move = true;
    
        if (Vector2Distance(player.position, player.destination) > 3.0f && player.should_move) {
	    TileMapResult player_path_tile = index_to_tilemap(player.path.indices[player.path_counter], tiles_across);
	    Vector2 player_path_vec = tilemap_to_world_space(player_path_tile.x, player_path_tile.y , tile_width, tile_height);
	    player.heading = Vector2Subtract(player_path_vec, player.position);
	    player.heading = Vector2Normalize(player.heading);
	    if (Vector2Distance(player.position, player_path_vec) > 3.0f) {
		player.position = Vector2Add(player.position, Vector2Scale(player.heading, dt_to_use * player_speed));
	    }
	    if (Vector2Distance(player.position, player_path_vec) < 3.0f && player.path_counter < player.path.len) {
		player.path_counter++;
	    }
		

	
        } else {
	    player.velocity = (Vector2){.x = 0.0f, .y = 0.0f};
	}

	TileMapResult player_tilemap = world_space_to_tilemap(player.position.x, player.position.y, tile_width, tile_height);
	TileMapResult mouse_tilemap = world_space_to_tilemap(mouse_pos.x, mouse_pos.y, tile_width, tile_height);
	TileMapResult destination_tilemap = world_space_to_tilemap(player.destination.x, player.destination.y, tile_width, tile_height);

	if (player.attacking) {
	    player.attack_timer += dt_to_use;
	    if (player.attack_timer >= player.attack_timer_max) {
		player.attack_timer = 0.0f;
		player.attacking = false;
		if ((Vector2Distance(player.position, enemy.position) < player_attack_radius)) {		    
		    enemy.health--;
		}
	    }
	}


	
            
        //dt -= dt_to_use;
        //game_timer += dt_to_use;
            
        



        
       

        // Draw
        //----------------------------------------------------------------------------------
     
        BeginDrawing();
        
        
     
        ClearBackground(RED);
        
        
        
        
        
       
        
        
     
        
        DrawFPS(30, 30);

	draw_tilemap(tilemap_t);
	for (int i = 0; i <= enemy.path.len; i++) {
	    TileMapResult debugTile = index_to_tilemap(enemy.path.indices[i], tiles_across);
	    DrawRectangleLines(debugTile.x * tile_width, debugTile.y * tile_height, tile_width, tile_height, YELLOW);
	}
	
	//DrawRectangleLines(enemy_current_tile.x * tile_width, enemy_current_tile.y * tile_height, tile_width, tile_height, RED);
	DrawRectangleLines(player_tilemap.x * tile_width, player_tilemap.y * tile_height, tile_width, tile_height, RED);
	DrawRectangleLines(mouse_tilemap.x * tile_width, mouse_tilemap.y * tile_height, tile_width, tile_height, YELLOW);
	DrawRectangleLines(destination_tilemap.x * tile_width, destination_tilemap.y * tile_height, tile_width, tile_height, PINK);

	DrawCircle(player.position.x, player.position.y, 10, RAYWHITE);

	
	DrawCircle(enemy.position.x, enemy.position.y, 10, BLUE);
        
        
        DrawCircleLines(mouse_pos.x, mouse_pos.y, 5, LIGHTGRAY);


	update_brain(tilemap_t, &node_array, &enemy, &player, dt_to_use); 

	for (int i = 0; i < enemy.health; i++) {
	    DrawCircle(10 + i*5, 10, 3, RAYWHITE);
	    //
	}
	for (int i = 0; i < player.health; i++) {
	    DrawCircle(10 + i*5, 30, 3, RED);
	    //
	}


	// debug drawing
	if (recently_shifted_modes) {
	    recent_timer += dt_to_use;
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

	// add some debug info for enemy brain status
	switch (enemy.brain_state) {
	case (RESTING):
	    DrawText("enemy resting", screen_width - 100, 30, 14, BLACK);
	    break;
	case (PATROLLING):
	    DrawText("enemy patrolling", screen_width - 100, 30, 14, BLACK);
	    break;
	case (SEEKING):
	    DrawText("enemy seeking", screen_width - 100, 30, 14, BLACK);
	    break;
	case (ATTACKING):
	    DrawText("enemy attacking", screen_width - 100, 30, 14, BLACK);
	    break;
	case (FLEEING):
	    DrawText("enemy fleeing", screen_width - 100, 30, 14, BLACK);
	    break;
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
