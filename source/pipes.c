// pipes.c  — fixed & refactored
#include <tonc.h>
#include <string.h>
#include <stdlib.h>
#include "pipes.h"
#include <time.h>
#include "../graphics/game_pipes_topright.h"
#include "../graphics/game_pipes_topleft.h"
#include "../graphics/game_pipes_bottomright.h"
#include "../graphics/game_pipes_bottomleft.h"

extern OBJ_ATTR obj_buffer[128];

// configuration
int pipes_posX = 240;
// static const int SCREEN_RIGHT = 240;
static const int RESET_X = 240;
static const int OFFSCREEN_LEFT = -16;
int pipe_speed = 1;

// We have 4 pipes, each pipe uses 4 OBJ_ATTRs (tl,tr,bl,br)
#define NUM_PIPES 4

static OBJ_ATTR *pipe_tl[NUM_PIPES];
static OBJ_ATTR *pipe_tr[NUM_PIPES];
static OBJ_ATTR *pipe_bl[NUM_PIPES];
static OBJ_ATTR *pipe_br[NUM_PIPES];

// Y positions for each pipe (top tile Y)
static int pipe_posY[NUM_PIPES] = {23, 23, 23, 23};


static void set_single_piece(OBJ_ATTR *o, int x, int y, u16 palbank, u16 tile_index)
{
    // ATTR0_SQUARE | ATTR0_4BPP: 8x8, 4bpp
    // ATTR1_SIZE_8: size 8x8
    // ATTR2_BUILD(tile_index, palbank, 0): Use tile_index in OBJ charblock 0 (default), with specified palbank and prio 0
    // Removed invalid 'charblock' param—OBJ only uses charblock 4 (index 0 in tonc).
    obj_set_attr(o,
        ATTR0_SQUARE | ATTR0_4BPP | (y & 0xFF),
        ATTR1_SIZE_8 | (x & 0x1FF),
        ATTR2_BUILD(tile_index, palbank, 0) | ATTR2_PRIO(0)
    );
}


static void set_pipe_sprites_for_pipe(int i, int posX, int posY)
{
    // Each corner uses its own tile index in charblock 0 (starting from 1 to avoid ball's tile at 0)
    set_single_piece(pipe_tl[i], posX, posY, 1, 1); // topleft -> tile 1
    set_single_piece(pipe_tr[i], posX+8, posY, 2, 2); // topright -> tile 2
    set_single_piece(pipe_bl[i], posX, posY+8, 3, 3); // bottomleft -> tile 3
    set_single_piece(pipe_br[i], posX+8, posY+8, 4, 4); // bottomright -> tile 4
}

// --- public API --------------------------------------------------------------
void pipes_init()
{
    // load palettes into OBJ palette memory (use pal banks 1..4) — this is already correct
    memcpy(&pal_obj_mem[16], game_pipes_topleftPal, 32);      // palbank 1
    memcpy(&pal_obj_mem[32], game_pipes_toprightPal, 32);    // palbank 2
    memcpy(&pal_obj_mem[48], game_pipes_bottomleftPal, 32);// palbank 3
    memcpy(&pal_obj_mem[64], game_pipes_bottomrightPal, 32);// palbank 4

    // load tiles into OBJ VRAM (charblock 0, starting from tile 1 to avoid ball's tile at 0)
    // Changed from tile_mem_obj[1][n] to [0][n] — [1] is BG VRAM, not OBJ!
    memcpy(&tile_mem_obj[0][1], game_pipes_topleftTiles, game_pipes_topleftTilesLen);      // tile 1
    memcpy(&tile_mem_obj[0][2], game_pipes_toprightTiles, game_pipes_toprightTilesLen);    // tile 2
    memcpy(&tile_mem_obj[0][3], game_pipes_bottomleftTiles, game_pipes_bottomleftTilesLen); // tile 3
    memcpy(&tile_mem_obj[0][4], game_pipes_bottomrightTiles, game_pipes_bottomrightTilesLen); // tile 4

    // assign obj_buffer slots (1..16 used for pipe pieces) — unchanged
    // Pipe 0 uses slots 1..4, Pipe 1 uses 5..8, Pipe 2 uses 9..12, Pipe 3 uses 13..16
    for(int i = 0; i < NUM_PIPES; i++)
    {
        int base = 1 + i * 4;
        pipe_tl[i] = &obj_buffer[base + 0];
        pipe_tr[i] = &obj_buffer[base + 1];
        pipe_bl[i] = &obj_buffer[base + 2];
        pipe_br[i] = &obj_buffer[base + 3];
    }
    
    random_pipes();  
    pipes_posX = RESET_X;
    for(int i = 0; i < NUM_PIPES; i++)
        set_pipe_sprites_for_pipe(i, pipes_posX, pipe_posY[i]);

    // pipe_posY[0] = 23;
    // pipe_posY[1] = 39;
    // pipe_posY[2] = 56;
    // pipe_posY[3] = 107;

    // set initial positions: horizontally stacked at the same X (they move together)
    pipes_posX = RESET_X;
    
}


// update attr1 X for all pipe pieces (called on movement) — unchanged
static void update_all_pipes_attr1_X(int posX)
{
    for(int i = 0; i < NUM_PIPES; i++)
    {
        pipe_tl[i]->attr1 = (pipe_tl[i]->attr1 & ~0x1FF) | (posX & 0x1FF);
        pipe_tr[i]->attr1 = (pipe_tr[i]->attr1 & ~0x1FF) | ((posX + 8) & 0x1FF);
        pipe_bl[i]->attr1 = (pipe_bl[i]->attr1 & ~0x1FF) | (posX & 0x1FF);
        pipe_br[i]->attr1 = (pipe_br[i]->attr1 & ~0x1FF) | ((posX + 8) & 0x1FF);
    }
}

// update attr0 Y for a pipe (used after randomizing Y) — unchanged
static void update_pipe_attr0_Y(int i)
{
    int y = pipe_posY[i] & 0xFF;
    pipe_tl[i]->attr0 = (pipe_tl[i]->attr0 & ~0xFF) | y;
    pipe_tr[i]->attr0 = (pipe_tr[i]->attr0 & ~0xFF) | y;
    pipe_bl[i]->attr0 = (pipe_bl[i]->attr0 & ~0xFF) | ((y + 8) & 0xFF);
    pipe_br[i]->attr0 = (pipe_br[i]->attr0 & ~0xFF) | ((y + 8) & 0xFF);
}

// type: 0..2 selects a preset pattern for the 4 pipes — unchanged
void pipe_randomizer(int type)
{
    pipes_posX = RESET_X;

    switch(type)
    {
        case 0:
            pipe_posY[0] = 59;
            pipe_posY[1] = 75;
            pipe_posY[2] = 91;
            pipe_posY[3] = 107;
            break;

        case 1:
            pipe_posY[0] = 59;
            pipe_posY[1] = 111;
            pipe_posY[2] = 127;
            pipe_posY[3] = 143;
            break;

        case 2:
        default:
            pipe_posY[0] = 23;
            pipe_posY[1] = 39;
            pipe_posY[2] = 55;
            pipe_posY[3] = 107;
            break;
    }

    // Update sprite Y positions (attr0) to match new pipe_posY[]
    for(int i = 0; i < NUM_PIPES; i++)
        update_pipe_attr0_Y(i);

    // update X (so visible immediately)
    update_all_pipes_attr1_X(pipes_posX);
}

void random_pipes()
{
    pipe_randomizer(rand() % 3);
}

void pipes_update()
{
    pipes_posX -= pipe_speed;
    update_all_pipes_attr1_X(pipes_posX);

    // reset (wrap-around)
    if(pipes_posX <= OFFSCREEN_LEFT)
    {
        // choose new random heights and place back at right
        random_pipes();
    }
}

void reset_pipes(){
    pipes_posX = RESET_X;
    pipe_speed = 1;

    random_pipes();
    update_all_pipes_attr1_X(pipes_posX);
}

bool pipes_check_collision(int ball_x, int ball_y)
{
    int ball_left   = ball_x;
    int ball_right  = ball_x + 8;
    int ball_top    = ball_y;
    int ball_bottom = ball_y + 8;

    for(int i = 0; i < NUM_PIPES; i++)
    {
        int x0 = pipes_posX;
        int x1 = pipes_posX + 8;

        int y0 = pipe_posY[i];
        int y1 = pipe_posY[i] + 8;

        // TL block
        if(ball_right > x0 && ball_left < x0+8 &&
           ball_bottom > y0 && ball_top < y0+8)
            return true;

        // TR block
        if(ball_right > x1 && ball_left < x1+8 &&
           ball_bottom > y0 && ball_top < y0+8)
            return true;

        // BL block
        if(ball_right > x0 && ball_left < x0+8 &&
           ball_bottom > y1 && ball_top < y1+8)
            return true;

        // BR block
        if(ball_right > x1 && ball_left < x1+8 &&
           ball_bottom > y1 && ball_top < y1+8)
            return true;
    }

    return false;
}
