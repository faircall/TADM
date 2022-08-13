#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "raylib.h"
#include "raymath.h"

#define RAYMATH_IMPLEMENTATION

#define min(a,b) (a < b? a : b)

/*
 *TODO: 
 * CLEAN the code: pick a style and stick to it
 * -refactor all the messiness
 */

// for the love of christ keep things simple on this one.
// you're not allowed to any graphics code until you have a
// playable world. I MEAN IT
// this is a GAMEPLAY toybox.
// graphics are BAD on this one
// no shaders, no bs till the game is DONE

// premise for this game is: Stalker / Diablo


// brain for enemy

// random idea
// enemies can get possessed by more intelligent daemons

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
    GRASSTILE,
    BRICKTILE,
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

bool node_equals(Node n1, Node n2)
{
    bool result = false;

    if (n1.current == n2.current &&
	n1.predecessor== n2.predecessor) {
	result = true;
    }
    
    return result;
}

uint find_index_of_node(uint rawIndex, NodeArray nodeArray)
{
    for (int i = 0; i < nodeArray.count; i++) {
	if (nodeArray.nodes[i].current == rawIndex) {
	    return i;
	}
    }
}


TileMapResult world_space_to_tilemap(float worldx, float worldy, uint width, uint height)
{
    TileMapResult result;
    result.x = worldx / width;
    result.y = worldy / height;
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

uint tilemap_to_index(TileMapResult tilemapResult, uint stride)
{
    uint result = tilemapResult.x + tilemapResult.y * stride;
    return result;
}

TileMapResult index_to_tilemap(uint index, uint stride)
{
    TileMapResult result;
    result.x = index % stride;
    result.y = index / stride;
    return result;
}

TILETYPE tilemap_to_type(TileMapResult tileMapResult, TILETYPE *tileMap, uint stride)
{
    // is that correct?
    // this should be tiles across not width
    return tileMap[tileMapResult.x + tileMapResult.y*stride];
}

bool free_tile(TileMapResult tileMapResult, TILETYPE *tileMap, uint stride)
{
    // seems we are allowing ourselves to pass over brick tiles?
    bool result = true;
    if (tilemap_to_type(tileMapResult, tileMap, stride) == BRICKTILE) {
	result = false;
    }
    return result;
}

NeighbourResult get_neighbours(Node node, TILETYPE *tileMap, uint tilesAcross, uint tilesDown)
{

    // this could be a lot shorter
    NeighbourResult result;
    TileMapResult nodeTile = index_to_tilemap(node.current, tilesAcross);
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
	    if (free_tile(tiles[i], tileMap, tilesAcross)) {
		result.neighbours[added].current = tilemap_to_index(tiles[i], tilesAcross);
		result.neighbours[added].predecessor = node.current;
		added++;
	    }
	}
	result.count = added;
    } else if (x == tilesAcross && y == tilesDown) {
	TileMapResult t0 = {.x = x - 1, .y = y};
	TileMapResult t1 = {.x = x - 1, .y = y -1};
	TileMapResult t2 = {.x = x, .y = y - 1};
	TileMapResult tiles[3] = {t0, t1, t2};
	int added = 0;
	for (int i = 0; i < 3; i++) {
	    if (free_tile(tiles[i], tileMap, tilesAcross)) {
		result.neighbours[added].current = tilemap_to_index(tiles[i], tilesAcross);
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
	    if (free_tile(tiles[i], tileMap, tilesAcross)) {
		result.neighbours[added].current = tilemap_to_index(tiles[i], tilesAcross);
		result.neighbours[added].predecessor = node.current;
		added++;
	    }
	}
	result.count = added;
    } else if (x == tilesAcross) {
	TileMapResult t0 = {.x = x - 1, .y = y};
	TileMapResult t1 = {.x = x - 1, .y = y + 1};
	TileMapResult t2 = {.x = x, .y = y + 1};

	TileMapResult t3 = {.x = x - 1, .y = y - 1};
	TileMapResult t4 = {.x = x, .y = y - 1};

	TileMapResult tiles[5] = {t0, t1, t2, t3, t4};
	int added = 0;
	for (int i = 0; i < 5; i++) {
	    if (free_tile(tiles[i], tileMap, tilesAcross)) {
		result.neighbours[added].current = tilemap_to_index(tiles[i], tilesAcross);
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
	    if (free_tile(tiles[i], tileMap, tilesAcross)) {
		result.neighbours[added].current = tilemap_to_index(tiles[i], tilesAcross);
		added++;
	    }
	}
	result.count = added;
    } else if (y == tilesDown) {
	TileMapResult t0 = {.x = x - 1, .y = y};
	TileMapResult t1 = {.x = x - 1, .y = y - 1};
	TileMapResult t2 = {.x = x, .y = y - 1};

	TileMapResult t3 = {.x = x + 1, .y = y};
	TileMapResult t4 = {.x = x + 1, .y = y - 1};

	TileMapResult tiles[5] = {t0, t1, t2, t3, t4};
	int added = 0;
	for (int i = 0; i < 5; i++) {
	    if (free_tile(tiles[i], tileMap, tilesAcross)) {
		result.neighbours[added].current = tilemap_to_index(tiles[i], tilesAcross);
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
	    if (free_tile(tiles[i], tileMap, tilesAcross)) {
		result.neighbours[added].current = tilemap_to_index(tiles[i], tilesAcross);
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

Node tilemap_to_terminal_node(TileMapResult tilemap, uint tilesAcross)
{
    // turn a tilemap into a terminal node
    Node result;
    uint index = tilemap_to_index(tilemap, tilesAcross);
    result.current = index;
    result.predecessor = index;

    return result;
}



int flood_fill_to_destination(TileMapResult start, TileMapResult end, TILETYPE *tileMap, NodeArray *nodes, uint tilesAcross, uint tilesDown)
{
    int result = -1;
    
    // clear the NodeArray for use
    nodes->count = 0;
    
    // i don't think height is coming into play

    // we should do
    Node startNode = tilemap_to_terminal_node(start, tilesAcross);
    Node endNode = tilemap_to_terminal_node(end, tilesAcross);
    if (node_equals(startNode, endNode)) {
	//
    }
    
    
    uint nodesAdded = 0;
    uint nodesSearched = 0;
    add_node(startNode, nodes);
    nodesAdded++;

    Node endReconstruct;

    bool destination_found = false;

    // possible off by one error here
    while (!destination_found && nodesSearched < (nodes->max)) {
	Node current = nodes->nodes[nodesSearched];

	if (current.current == endNode.current) {
	    destination_found = true;
	    add_node(current, nodes);
	    nodesAdded++;
	    break;
	}
	
	//
	NeighbourResult neighbourResult = get_neighbours(current, tileMap, tilesAcross, tilesDown);
	for (int i = 0; i < neighbourResult.count; i++) {
	    Node new = neighbourResult.neighbours[i];	    
	    if (!contains_node(*nodes, new, nodesAdded)) {
		add_node(new, nodes);
		nodesAdded++;
	    }
	}
	nodesSearched++;
    }

    if (destination_found) {
	result = nodesSearched;
    }

    return result;
}

void reconstruct_path(Path *path, NodeArray nodes, TileMapResult start, int endIndex, uint tilesAcross)
{
    
    Node currentNode = nodes.nodes[endIndex];
    uint pathLen = 0;
    bool countedPath = false;
    uint startIndex = tilemap_to_index(start, tilesAcross);
    // we need to figure out how long the path will actually be
    while (!countedPath) {
	if (currentNode.current == startIndex) {
	    countedPath = true;
	} else {
	    pathLen++;
	    // we could instead store the position when we place it, would be better
	    currentNode = nodes.nodes[find_index_of_node(currentNode.predecessor, nodes)];
	}
    }
    
    path->len = pathLen;
    currentNode = nodes.nodes[endIndex];
    for (int i = pathLen; i >= 0; i--) {
	path->indices[i] = currentNode.current;
	currentNode = nodes.nodes[find_index_of_node(currentNode.predecessor, nodes)];
    }
    // so we can THEN do a countdown-add to the path, set its length appropriately and use that later
}

int main(int argc, char **argv)
{
    const int screenWidth = 1280;
    const int screenHeight = 720;
    
    float gameTimer = 0.0f;
    
    SetWindowState(FLAG_VSYNC_HINT);
    InitWindow(screenWidth, screenHeight, "Through A Dark Meadow");
    HideCursor();

    SetTargetFPS(60);               // Set our game to run at 60 frames-per-second

    InitAudioDevice();
    
    float targetMsPerFrame = 1.0f/60.0f;

    Vector2 playerPosition = {.x = screenWidth/2.0f, .y = screenHeight/2.0f};
    Vector2 playerDestination = {.x = playerPosition.x, .y = playerPosition.y};
    Vector2 playerVelocity = {.x = 0.0f, .y = 0.0f};
    Vector2 playerHeading= {.x = 0.0f, .y = 0.0f};

    Vector2 enemyPosition = {.x = 600.0f, .y = 400.0f};

    int enemyHealth = 100;

    bool playerAttacking = false;

    float playerAttackRadius = 20.0f;

    Sound rainSound = LoadSound("sound/rain-07.wav");

    uint tileWidth = 32;
    uint tileHeight = 32;    
    uint tilesAcross = screenWidth / tileWidth;
    uint tilesDown = screenHeight / tileHeight;
    
    TILETYPE *tileMap = malloc(sizeof *tileMap * tilesAcross * tilesDown);
    for (int j = 0; j < tilesDown; j++) {
	for (int i = 0; i < tilesAcross; i++) {
	    int randTile = rand() % NUMTILES;
	    tileMap[j*tilesAcross + i] = randTile;
	}
    }

    // some test destinations for the enemy navigation
    
    NodeArray nodeArray;
    nodeArray.count = 0;
    nodeArray.max = tilesAcross * tilesDown;
    nodeArray.nodes = malloc(nodeArray.max * sizeof(Node));

    Path pathArray;
    Path playerPathArray;
    pathArray.indices = malloc(sizeof(uint) * tilesAcross * tilesDown);
    pathArray.max = tilesAcross * tilesDown;
    pathArray.len = 0;

    playerPathArray.indices = malloc(sizeof(uint) * tilesAcross * tilesDown);
    playerPathArray.max = tilesAcross * tilesDown;
    playerPathArray.len = 0;
    
    // Vector2 enemyDestinationTest = {.x = 550.0f, .y = 550.0f};
    Vector2 enemyDestinationTest = {.x = 32.0f, .y = 320.0f};
    int flood_result = flood_fill_to_destination(world_space_to_tilemap(enemyPosition.x, enemyPosition.y, tileWidth, tileHeight),
						 world_space_to_tilemap(enemyDestinationTest.x, enemyDestinationTest.y, tileWidth, tileHeight),
						 tileMap,
						 &nodeArray, tilesAcross, tilesDown);
    if (flood_result != -1) {
	reconstruct_path(&pathArray, nodeArray, world_space_to_tilemap(enemyPosition.x, enemyPosition.y, tileWidth, tileHeight),
		     flood_result, tilesAcross);
    }
    
    uint enemy_path_counter = 0;

    uint player_path_counter = 0;

    while (!WindowShouldClose())    // Detect window close button or ESC key
    {
	// sound stuff
	if (!IsSoundPlaying(rainSound)) {
	    PlaySound(rainSound);
	}
	
        // Update
        //----------------------------------------------------------------------------------
        // TODO: Update your variables here
        //----------------------------------------------------------------------------------
        float dt = GetFrameTime();
        
        
        
        float playerSpeed = 20.0f;
        float maxSpeed = 150.0f;
        
        playerAttacking = false;
       
        struct Vector2 mousePos = {.x = GetMouseX(), .y = GetMouseY()};
       
        if (IsMouseButtonPressed(0)) {
	    player_path_counter = 0;
            playerDestination = (Vector2){.x = mousePos.x, .y = mousePos.y};
	    
	    // maybe not the most efficient thing to do?
	    TileMapResult tempTileMapDest = world_space_to_tilemap(playerDestination.x, playerDestination.y, tileWidth, tileHeight);
	    TileMapResult tempTileMapCurrent = world_space_to_tilemap(playerPosition.x, playerPosition.y, tileWidth, tileHeight);
	    nodeArray.count = 0;
	    int floodResult = flood_fill_to_destination(tempTileMapCurrent, tempTileMapDest, tileMap, &nodeArray, tilesAcross, tilesDown);
	    if (floodResult != -1) {
		reconstruct_path(&playerPathArray, nodeArray, tempTileMapCurrent, floodResult, tilesAcross);
	    } else {
	    }
	    
            
            
            
        }	
	if (IsMouseButtonPressed(1)) {
	    playerAttacking = true;
            
        }

	
        
        float dtToUse = min(dt, targetMsPerFrame);
    
        if (Vector2Distance(playerPosition, playerDestination) > 3.0f) {
	    TileMapResult player_path_tile = index_to_tilemap(playerPathArray.indices[player_path_counter], tilesAcross);
	    Vector2 player_path_vec = tilemap_to_world_space(player_path_tile.x, player_path_tile.y , tileWidth, tileHeight);
	    playerHeading = Vector2Subtract(player_path_vec, playerPosition);
	    playerHeading = Vector2Normalize(playerHeading);
	    if (Vector2Distance(playerPosition, player_path_vec) > 3.0f) {
		playerPosition = Vector2Add(playerPosition, Vector2Scale(playerHeading, dtToUse * 50.0f));
	    }
	    if (Vector2Distance(playerPosition, player_path_vec) < 3.0f && player_path_counter < playerPathArray.len) {
		player_path_counter++;
	    }
		

	    #if 0
            playerHeading = Vector2Subtract(playerDestination, playerPosition);
            playerHeading = Vector2Normalize(playerHeading);
            
            playerVelocity = Vector2Add(playerVelocity, Vector2Scale(playerHeading, playerSpeed));
            if (Vector2Length(playerVelocity) > maxSpeed) {
                playerVelocity = Vector2Normalize(playerVelocity);
                playerVelocity = Vector2Scale(playerVelocity, maxSpeed);
            }
            
            //playerVelocity = Vector2Normalize(playerVelocity);
            
            if (Vector2Length(playerVelocity) > 5.0f) {
            playerPosition = Vector2Add(playerPosition, Vector2Scale(playerVelocity, dtToUse));
            } else {
                if (Vector2Length(playerVelocity) > 2.0f) {
                    playerVelocity = Vector2Add(playerVelocity, Vector2Scale(playerVelocity, -0.5f));
                } else {
                    playerVelocity = (Vector2){.x = 0.0f, .y = 0.0f};
                }
            }
	    #endif
        } else {
	    playerVelocity = (Vector2){.x = 0.0f, .y = 0.0f};
	}

	TileMapResult playerTileMap = world_space_to_tilemap(playerPosition.x, playerPosition.y, tileWidth, tileHeight);
	TileMapResult mouseTileMap = world_space_to_tilemap(mousePos.x, mousePos.y, tileWidth, tileHeight);
	TileMapResult destinationTileMap = world_space_to_tilemap(playerDestination.x, playerDestination.y, tileWidth, tileHeight);

	if ((Vector2Distance(playerPosition, enemyPosition) < playerAttackRadius)
	    && playerAttacking) {
	    enemyHealth--;
	}

	
            
        //dt -= dtToUse;
        //gameTimer += dtToUse;
            
        
        
        TileMapResult enemy_path_tile = index_to_tilemap(pathArray.indices[enemy_path_counter], tilesAcross);


	
	Vector2 enemy_path_vec = tilemap_to_world_space(enemy_path_tile.x, enemy_path_tile.y, tileWidth, tileHeight);
	Vector2 enemy_heading_vec = Vector2Subtract(enemy_path_vec, enemyPosition);

	TileMapResult enemy_current_tile = world_space_to_tilemap(enemyPosition.x, enemyPosition.y, tileWidth, tileHeight);
	enemy_heading_vec = Vector2Normalize(enemy_heading_vec);
	if (Vector2Distance(enemyPosition, enemy_path_vec) > 3.0f) {
	    enemyPosition = Vector2Add(enemyPosition, Vector2Scale(enemy_heading_vec, dtToUse * 50.0f));
	}
	if (Vector2Distance(enemyPosition, enemy_path_vec) < 3.0f && enemy_path_counter < pathArray.len) {
	    enemy_path_counter++;
	}
        
       

        // Draw
        //----------------------------------------------------------------------------------
     
        BeginDrawing();
        
        
     
        ClearBackground(RED);
        
        
        
        
        
       
        
        
     
        
        DrawFPS(30, 30);

	for (int y = 0; y < tilesDown; y++) {
	    for (int x = 0; x < tilesAcross; x++) {
		TILETYPE tile = tileMap[y*tilesAcross + x];
		if (tile == GRASSTILE) {
		    DrawRectangle(x * tileWidth, y * tileHeight, tileWidth, tileHeight, GREEN);
		}
		if (tile == BRICKTILE) {
		    DrawRectangle(x * tileWidth, y * tileHeight, tileWidth, tileHeight, PINK);
		}
		if (tile == MUDTILE) {
		    DrawRectangle(x * tileWidth, y * tileHeight, tileWidth, tileHeight, DARKBROWN);
		}
		if (tile == DIRTTILE) {
		    DrawRectangle(x * tileWidth, y * tileHeight, tileWidth, tileHeight, BEIGE);
		}
	    }
	}
	for (int i = 0; i <= pathArray.len; i++) {
	    TileMapResult debugTile = index_to_tilemap(pathArray.indices[i], tilesAcross);
	    DrawRectangleLines(debugTile.x * tileWidth, debugTile.y * tileHeight, tileWidth, tileHeight, YELLOW);
	}
	
	DrawRectangleLines(enemy_current_tile.x * tileWidth, enemy_current_tile.y * tileHeight, tileWidth, tileHeight, RED);
	DrawRectangleLines(playerTileMap.x * tileWidth, playerTileMap.y * tileHeight, tileWidth, tileHeight, RED);
	DrawRectangleLines(mouseTileMap.x * tileWidth, mouseTileMap.y * tileHeight, tileWidth, tileHeight, YELLOW);
	DrawRectangleLines(destinationTileMap.x * tileWidth, destinationTileMap.y * tileHeight, tileWidth, tileHeight, PINK);

	DrawCircle(playerPosition.x, playerPosition.y, 10, RAYWHITE);

	
	DrawCircle(enemyPosition.x, enemyPosition.y, 10, BLUE);
        
        
        DrawCircleLines(mousePos.x, mousePos.y, 5, LIGHTGRAY);

	for (int i = 0; i < enemyHealth; i++) {
	    DrawCircle(10 + i*5, 10, 3, RAYWHITE);
	    //
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
