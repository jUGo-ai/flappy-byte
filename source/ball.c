#include <tonc.h>
#include <string.h>
#include <stdbool.h>
#include "ball.h"
#include "../graphics/game_ball.h"

#define BALL_X 25
#define GRAVITY 0.2
#define JUMP_VELOCITY -2

extern OBJ_ATTR obj_buffer[128]; // reference the global buffer
OBJ_ATTR *ball;

int ball_Y = 80;
float ball_velocity = 0;

void ball_init() {
    ball = &obj_buffer[0];   // first slot

    // 4BPP uses only half of pal_obj_mem (16 colors)
    memcpy(&pal_obj_mem[0], game_ballPal, 32);
    memcpy(&tile_mem_obj[0][0], game_ballTiles, game_ballTilesLen);

    obj_set_attr(ball,
        ATTR0_SQUARE | ATTR0_4BPP | (ball_Y & 0xFF),   // Changed 8BPP -> 4BPP
        ATTR1_SIZE_8 | (BALL_X & 0x1FF),
        ATTR2_BUILD(0, 0, 0)
    );
}

void ball_update(bool press_jump) {
    if (press_jump)
        ball_velocity = JUMP_VELOCITY;

    // Boundary checks
    if (ball_Y + ball_velocity <= 23) {
        ball_Y = 23;
        ball_velocity = 0;
    }
    if (ball_Y + ball_velocity >= 150) { // 160-8 sprite
        ball_Y = 151;
        ball_velocity = 0;
    }

    ball_velocity += GRAVITY;
    ball_Y += ball_velocity;

    // Update sprite position
    ball->attr0 = (ball->attr0 & ~0xFF) | (ball_Y & 0xFF);
}

void reset_ball(){
    ball_Y = 80;
    ball_velocity = 0;
    ball->attr0 = (ball->attr0 & ~0xFF) | (ball_Y & 0xFF);
    
}






