// pipes.c  — fixed & refactored
#include <tonc.h>
#include <string.h>
#include <stdlib.h>
#include "pipes.h"
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


static void set_single_piece(OBJ_ATTR *o, int x, int y, u16 palbank, u16 charblock, u16 tile_index)
{
    // ATTR0_SQUARE | ATTR0_4BPP: 8x8, 4bpp
    // ATTR1_SIZE_8: size 8x8
    // ATTR2_BUILD(tile_index, palbank, charblock): correct tile + palette + charblock
    // ATTR2_PRIO(1): render above background
    obj_set_attr(o,
        ATTR0_SQUARE | ATTR0_4BPP | (y & 0xFF),
        ATTR1_SIZE_8 | (x & 0x1FF),
        ATTR2_BUILD(0, palbank, charblock) | ATTR2_PRIO(0)
    );
}


static void set_pipe_sprites_for_pipe(int i, int posX, int posY)
{
    // Each corner uses its own tile index in charblock 1
    set_single_piece(pipe_tl[i], posX, posY, 1, 1, 0); // topleft
    set_single_piece(pipe_tr[i], posX+8, posY, 2, 1, 1); // topright
    set_single_piece(pipe_bl[i], posX, posY+8, 3, 1, 2); // bottomleft
    set_single_piece(pipe_br[i], posX+8, posY+8, 4, 1, 3); // bottomright
}

// --- public API --------------------------------------------------------------
void pipes_init()
{
    // load palettes into OBJ palette memory (use pal banks 1..4)
    memcpy(&pal_obj_mem[16], game_pipes_topleftPal, 32);      // palbank 1
    memcpy(&pal_obj_mem[32], game_pipes_toprightPal, 32);    // palbank 2
    memcpy(&pal_obj_mem[48], game_pipes_bottomleftPal, 32);// palbank 3
    memcpy(&pal_obj_mem[64], game_pipes_bottomrightPal, 32);// palbank 4

    // load tiles into OBJ VRAM (charblocks 1..4)
    memcpy(&tile_mem_obj[1][0], game_pipes_topleftTiles, game_pipes_topleftTilesLen);      // 32 bytes
    memcpy(&tile_mem_obj[1][1], game_pipes_toprightTiles, game_pipes_toprightTilesLen);    // next tile
    memcpy(&tile_mem_obj[1][2], game_pipes_bottomleftTiles, game_pipes_bottomleftTilesLen);
    memcpy(&tile_mem_obj[1][3], game_pipes_bottomrightTiles, game_pipes_bottomrightTilesLen);

    // assign obj_buffer slots (1..16 used for pipe pieces)
    // Pipe 0 uses slots 1..4, Pipe 1 uses 5..8, Pipe 2 uses 9..12, Pipe 3 uses 13..16
    for(int i = 0; i < NUM_PIPES; i++)
    {
        int base = 1 + i * 4;
        pipe_tl[i] = &obj_buffer[base + 0];
        pipe_tr[i] = &obj_buffer[base + 1];
        pipe_bl[i] = &obj_buffer[base + 2];
        pipe_br[i] = &obj_buffer[base + 3];
    }

    // pipe_posY[0] = 23;
    // pipe_posY[1] = 39;
    // pipe_posY[2] = 56;
    // pipe_posY[3] = 107;

    // set initial positions: horizontally stacked at the same X (they move together)
    pipes_posX = RESET_X;
    for(int i = 0; i < NUM_PIPES; i++)
        set_pipe_sprites_for_pipe(i, pipes_posX, pipe_posY[i]);
}


// update attr1 X for all pipe pieces (called on movement)
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

// update attr0 Y for a pipe (used after randomizing Y)
static void update_pipe_attr0_Y(int i)
{
    int y = pipe_posY[i] & 0xFF;
    pipe_tl[i]->attr0 = (pipe_tl[i]->attr0 & ~0xFF) | y;
    pipe_tr[i]->attr0 = (pipe_tr[i]->attr0 & ~0xFF) | y;
    pipe_bl[i]->attr0 = (pipe_bl[i]->attr0 & ~0xFF) | ((y + 8) & 0xFF);
    pipe_br[i]->attr0 = (pipe_br[i]->attr0 & ~0xFF) | ((y + 8) & 0xFF);
}

// type: 0..2 selects a preset pattern for the 4 pipes
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
            pipe_posY[2] = 56;
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

// #include <tonc.h>
// #include <string.h>
// #include <stdlib.h>
// #include "pipes.h"
// #include "../graphics/game_pipes_topright.h"
// #include "../graphics/game_pipes_topleft.h"
// #include "../graphics/game_pipes_bottomright.h"
// #include "../graphics/game_pipes_bottomleft.h"

// extern OBJ_ATTR obj_buffer[128];

// // 4x8x8 pieces for 4 pipes
// OBJ_ATTR *pipe1_topleft;
// OBJ_ATTR *pipe1_topright;
// OBJ_ATTR *pipe1_bottomleft;
// OBJ_ATTR *pipe1_bottomright;

// OBJ_ATTR *pipe2_topleft;
// OBJ_ATTR *pipe2_topright;
// OBJ_ATTR *pipe2_bottomleft;
// OBJ_ATTR *pipe2_bottomright;

// OBJ_ATTR *pipe3_topleft;
// OBJ_ATTR *pipe3_topright;
// OBJ_ATTR *pipe3_bottomleft;
// OBJ_ATTR *pipe3_bottomright;

// OBJ_ATTR *pipe4_topleft;
// OBJ_ATTR *pipe4_topright;
// OBJ_ATTR *pipe4_bottomleft;
// OBJ_ATTR *pipe4_bottomright;

// // positions
// int pipes_posX = 240;

// int pipe1_posY = 23;
// int pipe2_posY = 23;
// int pipe3_posY = 23;
// int pipe4_posY = 23;

// int pipe_speed = 1;

// // Initialize all pipes
// void pipes_init()
// {
//     // Load palettes into OBJ palette memory
//     memcpy(&pal_obj_mem[16], game_pipes_topleftPal, game_pipes_topleftPalLen);
//     memcpy(&pal_obj_mem[32], game_pipes_toprightPal, game_pipes_toprightPalLen);
//     memcpy(&pal_obj_mem[48], game_pipes_bottomleftPal, game_pipes_bottomleftPalLen);
//     memcpy(&pal_obj_mem[64], game_pipes_bottomrightPal, game_pipes_bottomrightPalLen);

//     // Load tiles into OBJ VRAM
//     memcpy(&tile_mem_obj[1][0], game_pipes_topleftTiles, game_pipes_topleftTilesLen);
//     memcpy(&tile_mem_obj[2][0], game_pipes_toprightTiles, game_pipes_toprightTilesLen);
//     memcpy(&tile_mem_obj[3][0], game_pipes_bottomleftTiles, game_pipes_bottomleftTilesLen);
//     memcpy(&tile_mem_obj[4][0], game_pipes_bottomrightTiles, game_pipes_bottomrightTilesLen);

//     // Assign sprite slots
//     pipe1_topleft = &obj_buffer[1];
//     pipe1_topright = &obj_buffer[2];
//     pipe1_bottomleft = &obj_buffer[3];
//     pipe1_bottomright = &obj_buffer[4];

//     pipe2_topleft = &obj_buffer[5];
//     pipe2_topright = &obj_buffer[6];
//     pipe2_bottomleft = &obj_buffer[7];
//     pipe2_bottomright = &obj_buffer[8];

//     pipe3_topleft = &obj_buffer[9];
//     pipe3_topright = &obj_buffer[10];
//     pipe3_bottomleft = &obj_buffer[11];
//     pipe3_bottomright = &obj_buffer[12];

//     pipe4_topleft = &obj_buffer[13];
//     pipe4_topright = &obj_buffer[14];
//     pipe4_bottomleft = &obj_buffer[15];
//     pipe4_bottomright = &obj_buffer[16];

//     // Set initial positions
//     pipe_reset_positions();
// }

// // Helper to set all 4 pieces for a single pipe
// void set_pipe_sprites(OBJ_ATTR *tl, OBJ_ATTR *tr, OBJ_ATTR *bl, OBJ_ATTR *br, int posX, int posY)
// {
//     // Top-left
//     obj_set_attr(tl, ATTR0_SQUARE | ATTR0_4BPP | (posY & 0xFF),
//                  ATTR1_SIZE_8 | (posX & 0x1FF), ATTR2_ID(0) | ATTR2_PALBANK(1));
//     // Top-right (+8 X)
//     obj_set_attr(tr, ATTR0_SQUARE | ATTR0_4BPP | (posY & 0xFF),
//                  ATTR1_SIZE_8 | ((posX+8) & 0x1FF), ATTR2_ID(0) | ATTR2_PALBANK(2));
//     // Bottom-left
//     obj_set_attr(bl, ATTR0_SQUARE | ATTR0_4BPP | ((posY+8) & 0xFF),
//                  ATTR1_SIZE_8 | (posX & 0x1FF), ATTR2_ID(0) | ATTR2_PALBANK(3));
//     // Bottom-right (+8 X)
//     obj_set_attr(br, ATTR0_SQUARE | ATTR0_4BPP | ((posY+8) & 0xFF),
//                  ATTR1_SIZE_8 | ((posX+8) & 0x1FF), ATTR2_ID(0) | ATTR2_PALBANK(4));
// }

// // Reset all pipes to start positions
// void pipe_reset_positions()
// {
//     set_pipe_sprites(pipe1_topleft, pipe1_topright, pipe1_bottomleft, pipe1_bottomright, pipes_posX, pipe1_posY);
//     set_pipe_sprites(pipe2_topleft, pipe2_topright, pipe2_bottomleft, pipe2_bottomright, pipes_posX, pipe2_posY);
//     set_pipe_sprites(pipe3_topleft, pipe3_topright, pipe3_bottomleft, pipe3_bottomright, pipes_posX, pipe3_posY);
//     set_pipe_sprites(pipe4_topleft, pipe4_topright, pipe4_bottomleft, pipe4_bottomright, pipes_posX, pipe4_posY);
// }

// void pipe_randomizer(int type)
// {
//     pipes_posX = 240;

//     switch(type)
//     {
//         case 0:
//             pipe1_posY = 59;
//             pipe1_topleft->attr0 = (pipe1_topleft->attr0 & ~0xFF) | (pipe1_posY & 0xFF);
//             pipe1_topright->attr0 = (pipe1_topright->attr0 & ~0xFF) | (pipe1_posY & 0xFF);
//             pipe1_bottomleft->attr0 = (pipe1_bottomleft->attr0 & ~0xFF) | ((pipe1_posY+8) & 0xFF);
//             pipe1_bottomright->attr0 = (pipe1_bottomright->attr0 & ~0xFF) | ((pipe1_posY+8) & 0xFF);

//             pipe2_posY = 75;
//             pipe2_topleft->attr0 = (pipe2_topleft->attr0 & ~0xFF) | (pipe2_posY & 0xFF);
//             pipe2_topright->attr0 = (pipe2_topright->attr0 & ~0xFF) | (pipe2_posY & 0xFF);
//             pipe2_bottomleft->attr0 = (pipe2_bottomleft->attr0 & ~0xFF) | ((pipe2_posY+8) & 0xFF);
//             pipe2_bottomright->attr0 = (pipe2_bottomright->attr0 & ~0xFF) | ((pipe2_posY+8) & 0xFF);

//             pipe3_posY = 91;
//             pipe3_topleft->attr0 = (pipe3_topleft->attr0 & ~0xFF) | (pipe3_posY & 0xFF);
//             pipe3_topright->attr0 = (pipe3_topright->attr0 & ~0xFF) | (pipe3_posY & 0xFF);
//             pipe3_bottomleft->attr0 = (pipe3_bottomleft->attr0 & ~0xFF) | ((pipe3_posY+8) & 0xFF);
//             pipe3_bottomright->attr0 = (pipe3_bottomright->attr0 & ~0xFF) | ((pipe3_posY+8) & 0xFF);

//             pipe4_posY = 107;
//             pipe4_topleft->attr0 = (pipe4_topleft->attr0 & ~0xFF) | (pipe4_posY & 0xFF);
//             pipe4_topright->attr0 = (pipe4_topright->attr0 & ~0xFF) | (pipe4_posY & 0xFF);
//             pipe4_bottomleft->attr0 = (pipe4_bottomleft->attr0 & ~0xFF) | ((pipe4_posY+8) & 0xFF);
//             pipe4_bottomright->attr0 = (pipe4_bottomright->attr0 & ~0xFF) | ((pipe4_posY+8) & 0xFF);
//             break;

//         case 1:
//             pipe1_posY = 59;
//             pipe1_topleft->attr0 = (pipe1_topleft->attr0 & ~0xFF) | (pipe1_posY & 0xFF);
//             pipe1_topright->attr0 = (pipe1_topright->attr0 & ~0xFF) | (pipe1_posY & 0xFF);
//             pipe1_bottomleft->attr0 = (pipe1_bottomleft->attr0 & ~0xFF) | ((pipe1_posY+8) & 0xFF);
//             pipe1_bottomright->attr0 = (pipe1_bottomright->attr0 & ~0xFF) | ((pipe1_posY+8) & 0xFF);

//             pipe2_posY = 111;
//             pipe2_topleft->attr0 = (pipe2_topleft->attr0 & ~0xFF) | (pipe2_posY & 0xFF);
//             pipe2_topright->attr0 = (pipe2_topright->attr0 & ~0xFF) | (pipe2_posY & 0xFF);
//             pipe2_bottomleft->attr0 = (pipe2_bottomleft->attr0 & ~0xFF) | ((pipe2_posY+8) & 0xFF);
//             pipe2_bottomright->attr0 = (pipe2_bottomright->attr0 & ~0xFF) | ((pipe2_posY+8) & 0xFF);

//             pipe3_posY = 127;
//             pipe3_topleft->attr0 = (pipe3_topleft->attr0 & ~0xFF) | (pipe3_posY & 0xFF);
//             pipe3_topright->attr0 = (pipe3_topright->attr0 & ~0xFF) | (pipe3_posY & 0xFF);
//             pipe3_bottomleft->attr0 = (pipe3_bottomleft->attr0 & ~0xFF) | ((pipe3_posY+8) & 0xFF);
//             pipe3_bottomright->attr0 = (pipe3_bottomright->attr0 & ~0xFF) | ((pipe3_posY+8) & 0xFF);

//             pipe4_posY = 143;
//             pipe4_topleft->attr0 = (pipe4_topleft->attr0 & ~0xFF) | (pipe4_posY & 0xFF);
//             pipe4_topright->attr0 = (pipe4_topright->attr0 & ~0xFF) | (pipe4_posY & 0xFF);
//             pipe4_bottomleft->attr0 = (pipe4_bottomleft->attr0 & ~0xFF) | ((pipe4_posY+8) & 0xFF);
//             pipe4_bottomright->attr0 = (pipe4_bottomright->attr0 & ~0xFF) | ((pipe4_posY+8) & 0xFF);
//             break;

//         case 2:
//             pipe1_posY = 23;
//             pipe1_topleft->attr0 = (pipe1_topleft->attr0 & ~0xFF) | (pipe1_posY & 0xFF);
//             pipe1_topright->attr0 = (pipe1_topright->attr0 & ~0xFF) | (pipe1_posY & 0xFF);
//             pipe1_bottomleft->attr0 = (pipe1_bottomleft->attr0 & ~0xFF) | ((pipe1_posY+8) & 0xFF);
//             pipe1_bottomright->attr0 = (pipe1_bottomright->attr0 & ~0xFF) | ((pipe1_posY+8) & 0xFF);

//             pipe2_posY = 39;
//             pipe2_topleft->attr0 = (pipe2_topleft->attr0 & ~0xFF) | (pipe2_posY & 0xFF);
//             pipe2_topright->attr0 = (pipe2_topright->attr0 & ~0xFF) | (pipe2_posY & 0xFF);
//             pipe2_bottomleft->attr0 = (pipe2_bottomleft->attr0 & ~0xFF) | ((pipe2_posY+8) & 0xFF);
//             pipe2_bottomright->attr0 = (pipe2_bottomright->attr0 & ~0xFF) | ((pipe2_posY+8) & 0xFF);

//             pipe3_posY = 56;
//             pipe3_topleft->attr0 = (pipe3_topleft->attr0 & ~0xFF) | (pipe3_posY & 0xFF);
//             pipe3_topright->attr0 = (pipe3_topright->attr0 & ~0xFF) | (pipe3_posY & 0xFF);
//             pipe3_bottomleft->attr0 = (pipe3_bottomleft->attr0 & ~0xFF) | ((pipe3_posY+8) & 0xFF);
//             pipe3_bottomright->attr0 = (pipe3_bottomright->attr0 & ~0xFF) | ((pipe3_posY+8) & 0xFF);

//             pipe4_posY = 107;
//             pipe4_topleft->attr0 = (pipe4_topleft->attr0 & ~0xFF) | (pipe4_posY & 0xFF);
//             pipe4_topright->attr0 = (pipe4_topright->attr0 & ~0xFF) | (pipe4_posY & 0xFF);
//             pipe4_bottomleft->attr0 = (pipe4_bottomleft->attr0 & ~0xFF) | ((pipe4_posY+8) & 0xFF);
//             pipe4_bottomright->attr0 = (pipe4_bottomright->attr0 & ~0xFF) | ((pipe4_posY+8) & 0xFF);
//             break;
//     }

//     pipe_1->attr0 = (pipe_1->attr0 & ~0xFF) | (pipe1_posY & 0xFF);
//     pipe_2->attr0 = (pipe_2->attr0 & ~0xFF) | (pipe2_posY & 0xFF);
//     pipe_3->attr0 = (pipe_3->attr0 & ~0xFF) | (pipe3_posY & 0xFF);
//     pipe_4->attr0 = (pipe_4->attr0 & ~0xFF) | (pipe4_posY & 0xFF);
// }

// void random_pipes()
// {
//     pipe_randomizer(rand() % 3);
// }

// // Move pipes left
// void pipes_update()
// {
//     pipes_posX -= pipe_speed;

//     OBJ_ATTR *pipes_tl[4] = {pipe1_topleft, pipe2_topleft, pipe3_topleft, pipe4_topleft};
//     OBJ_ATTR *pipes_tr[4] = {pipe1_topright, pipe2_topright, pipe3_topright, pipe4_topright};
//     OBJ_ATTR *pipes_bl[4] = {pipe1_bottomleft, pipe2_bottomleft, pipe3_bottomleft, pipe4_bottomleft};
//     OBJ_ATTR *pipes_br[4] = {pipe1_bottomright, pipe2_bottomright, pipe3_bottomright, pipe4_bottomright};

//     for(int i=0; i<4; i++)
//     {
//         pipes_tl[i]->attr1 = (pipes_tl[i]->attr1 & ~0x1FF) | (pipes_posX & 0x1FF);
//         pipes_tr[i]->attr1 = (pipes_tr[i]->attr1 & ~0x1FF) | ((pipes_posX+8) & 0x1FF);
//         pipes_bl[i]->attr1 = (pipes_bl[i]->attr1 & ~0x1FF) | (pipes_posX & 0x1FF);
//         pipes_br[i]->attr1 = (pipes_br[i]->attr1 & ~0x1FF) | ((pipes_posX+8) & 0x1FF);
//     }

//     if(pipes_posX <= -16)
//         pipes_posX = 240; // reset
// }
