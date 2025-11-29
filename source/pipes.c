// pipes.c — fixed & refactored
#include <tonc.h>
#include <string.h>
#include <stdlib.h>
#include "pipes.h"
#include "bytes.h"
#include "../graphics/game_pipes_topright.h"
#include "../graphics/game_pipes_topleft.h"
#include "../graphics/game_pipes_bottomright.h"
#include "../graphics/game_pipes_bottomleft.h"

extern OBJ_ATTR obj_buffer[128];
extern bool byte_updated;
extern bool pipe_passed;
extern int game_frame_counter;
extern unsigned char game_byte;
extern unsigned char random_byte;
extern const char* current_gate;

// configuration
int pipes_posX = 264;
static const int RESET_X = 264;
static const int OFFSCREEN_LEFT = -40;
int pipe_speed = 1;

#define PIPE_DISPLAY_BASE 36
#define NUM_PIPES 4
static unsigned char pipe_correct_value;  // Single correct value for the set
static unsigned char pipe_incorrect_value;  // Single decoy value for the set
static int display_y[2];  // Y positions for the two displays
static bool is_correct[2];  // Which display has the correct value

static OBJ_ATTR *pipe_tl[NUM_PIPES];
static OBJ_ATTR *pipe_tr[NUM_PIPES];
static OBJ_ATTR *pipe_bl[NUM_PIPES];
static OBJ_ATTR *pipe_br[NUM_PIPES];

// Y positions for each pipe (top tile Y)
static int pipe_posY[NUM_PIPES] = {23, 23, 23, 23};

static void draw_pipe_values(int posX)
{
    // Draw correct value at display_y[0] if is_correct[0], else decoy
    unsigned char value0 = is_correct[0] ? pipe_correct_value : pipe_incorrect_value;
    draw_byte_bits(value0, posX - 4, display_y[0], PIPE_DISPLAY_BASE);

    // Draw correct value at display_y[1] if is_correct[1], else decoy
    unsigned char value1 = is_correct[1] ? pipe_correct_value : pipe_incorrect_value;
    draw_byte_bits(value1, posX - 4, display_y[1], PIPE_DISPLAY_BASE + 8);
}

static void clear_pipe_values()
{
    clear_byte_bits(0, 0, PIPE_DISPLAY_BASE);      // Clear first byte
    clear_byte_bits(0, 0, PIPE_DISPLAY_BASE + 8);  // Clear second byte
}

static void set_single_piece(OBJ_ATTR *o, int x, int y, u16 palbank, u16 tile_index)
{
    obj_set_attr(o,
        ATTR0_SQUARE | ATTR0_4BPP | (y & 0xFF),
        ATTR1_SIZE_8 | (x & 0x1FF),
        ATTR2_BUILD(tile_index, palbank, 0) | ATTR2_PRIO(0)
    );
}

static void set_pipe_sprites_for_pipe(int i, int posX, int posY)
{
    set_single_piece(pipe_tl[i], posX, posY, 1, 1);
    set_single_piece(pipe_tr[i], posX+8, posY, 2, 2);
    set_single_piece(pipe_bl[i], posX, posY+8, 3, 3);
    set_single_piece(pipe_br[i], posX+8, posY+8, 4, 4);
}

void pipes_init()
{
    memcpy(&pal_obj_mem[16], game_pipes_topleftPal, 32);
    memcpy(&pal_obj_mem[32], game_pipes_toprightPal, 32);
    memcpy(&pal_obj_mem[48], game_pipes_bottomleftPal, 32);
    memcpy(&pal_obj_mem[64], game_pipes_bottomrightPal, 32);

    memcpy(&tile_mem_obj[0][1], game_pipes_topleftTiles, game_pipes_topleftTilesLen);
    memcpy(&tile_mem_obj[0][2], game_pipes_toprightTiles, game_pipes_toprightTilesLen);
    memcpy(&tile_mem_obj[0][3], game_pipes_bottomleftTiles, game_pipes_bottomleftTilesLen);
    memcpy(&tile_mem_obj[0][4], game_pipes_bottomrightTiles, game_pipes_bottomrightTilesLen);

    for(int i = 0; i < NUM_PIPES; i++)
    {
        int base = 1 + i * 4;
        pipe_tl[i] = &obj_buffer[base + 0];
        pipe_tr[i] = &obj_buffer[base + 1];
        pipe_bl[i] = &obj_buffer[base + 2];
        pipe_br[i] = &obj_buffer[base + 3];
    }
    pipes_posX = RESET_X;
    for(int i = 0; i < NUM_PIPES; i++)
        set_pipe_sprites_for_pipe(i, pipes_posX, pipe_posY[i]);
}

void calculate_pipe_values()
{
    unsigned char correct = game_byte;
    if (strcmp(current_gate, "NOT") == 0) {
        correct = ~game_byte;
    } else if (strcmp(current_gate, "AND") == 0) {
        correct &= random_byte;
    } else if (strcmp(current_gate, "OR") == 0) {
        correct |= random_byte;
    } else if (strcmp(current_gate, "XOR") == 0) {
        correct ^= random_byte;
    }
    pipe_correct_value = correct;
    pipe_incorrect_value = ~correct;  // Decoy: bitwise NOT

    // Randomly assign correct to one of the two displays
    bool correct_at_first = (rand() % 2) == 0;
    is_correct[0] = correct_at_first;
    is_correct[1] = !correct_at_first;
}

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

static void update_pipe_attr0_Y(int i)
{
    int y = pipe_posY[i] & 0xFF;
    pipe_tl[i]->attr0 = (pipe_tl[i]->attr0 & ~0xFF) | y;
    pipe_tr[i]->attr0 = (pipe_tr[i]->attr0 & ~0xFF) | y;
    pipe_bl[i]->attr0 = (pipe_bl[i]->attr0 & ~0xFF) | ((y + 8) & 0xFF);
    pipe_br[i]->attr0 = (pipe_br[i]->attr0 & ~0xFF) | ((y + 8) & 0xFF);
}

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
            display_y[0] = 23;  // Correct/decoy at top of first opening
            display_y[1] = 123; // Correct/decoy at top of second opening
            break;
        case 1:
            pipe_posY[0] = 59;
            pipe_posY[1] = 111;
            pipe_posY[2] = 127;
            pipe_posY[3] = 143;
            display_y[0] = 23;  // Correct/decoy at top of first opening
            display_y[1] = 75; // Correct/decoy at top of second opening
            break;
        case 2:
        default:
            pipe_posY[0] = 23;
            pipe_posY[1] = 39;
            pipe_posY[2] = 55;
            pipe_posY[3] = 107;
            display_y[0] = 71;  // Correct/decoy at top of first opening (based on your description)
            display_y[1] = 123; // Correct/decoy at top of second opening
            break;
    }

    for(int i = 0; i < NUM_PIPES; i++)
        update_pipe_attr0_Y(i);
    update_all_pipes_attr1_X(pipes_posX);
}

void random_pipes()
{
    pipe_randomizer(rand() % 3);
    calculate_pipe_values();  // Calculate single correct/decoy for the set
    draw_pipe_values(pipes_posX - 24);  // Draw the two bytes
}

void pipes_update()
{
    if(game_frame_counter % 1 == 0){
        pipes_posX -= pipe_speed;
        update_all_pipes_attr1_X(pipes_posX);

        // Update X for the 16 display sprites
        int new_x = pipes_posX - 24;
        for(int j = 0; j < 16; j++)
            obj_buffer[PIPE_DISPLAY_BASE + j].attr1 = (obj_buffer[PIPE_DISPLAY_BASE + j].attr1 & ~0x1FF) | ((new_x + (j % 8) * 8) & 0x1FF);
    }

    if(pipes_posX <= OFFSCREEN_LEFT)
    {
        random_pipes();
        byte_updated = false;
    }
}

void reset_pipes(){
    pipes_posX = RESET_X;
    pipe_speed = 1;
    random_pipes();
    update_all_pipes_attr1_X(pipes_posX);
    clear_pipe_values();
}

int pipes_check_collision(int ball_x, int ball_y)
{
    int ball_left = ball_x;
    int ball_right = ball_x + 8;
    int ball_top = ball_y;
    int ball_bottom = ball_y + 8;

    // Check pipe blocks (die)
    for(int i = 0; i < NUM_PIPES; i++)
    {
        int x0 = pipes_posX;
        int x1 = pipes_posX + 16;
        if ((ball_right > x0 && ball_left < x1 && ball_bottom > pipe_posY[i] && ball_top < pipe_posY[i] + 8) ||
            (ball_right > x0 && ball_left < x1 && ball_bottom > pipe_posY[i] + 8 && ball_top < pipe_posY[i] + 16))
            return 1;  // Hit pipe
    }

    // Check the two openings at display_y (8-pixel height for simplicity)
    int x0 = pipes_posX;
    int x1 = pipes_posX + 16;
    if (ball_right > x0 && ball_left < x1) {
        if (ball_bottom > display_y[0] && ball_top < display_y[0] + 36) {
            return is_correct[0] ? 2 : 3;  // Correct or decoy at first Y
        }
        if (ball_bottom > display_y[1] && ball_top < display_y[1] + 36) {
            return is_correct[1] ? 2 : 3;  // Correct or decoy at second Y
        }
    }
    return 0;  // No collision
}
