#include <tonc.h>
#include "../graphics/game_background.h"
#include "../graphics/game_banner_1.h"
#include "../graphics/game_banner_2.h"
#include "../graphics/game_banner_3.h"
#include "../graphics/game_banner_4.h"
#include "../graphics/game_banner_5.h"
#include "../graphics/game_banner_6.h"
#include "../graphics/game_banner_7.h"
#include "../graphics/bird.h"



// ------------------------------------------------------
// Basic constants
// ------------------------------------------------------
#define MODE3_FB ((u16*)0x06000000)
#define SCREEN_WIDTH  240
#define SCREEN_HEIGHT 160
#define BIRD_X        50
#define BIRD_SIZE     8
#define PIPE_WIDTH    20
#define GAP_HEIGHT    50
#define PIPE_SPEED    2
#define GRAVITY       1
#define JUMP_VELOCITY -7
#define SPRITE_SIZE   32
#define TRANSPARENT_COLOR 0x7C1F

// ------------------------------------------------------
// Game state variables
// ------------------------------------------------------
int bird_y      = 80;
int bird_vel    = 0;
int pipe_x      = SCREEN_WIDTH;
int pipe_gap_y  = 60;

// ------------------------------------------------------
// Helper to draw a filled rectangle in Mode 3
// ------------------------------------------------------
void draw_rect(int x, int y, int w, int h, u16 color)
{
    for(int iy = 0; iy < h; iy++)
        for(int ix = 0; ix < w; ix++)
        {
            int px = x + ix;
            int py = y + iy;
            if(px >= 0 && px < SCREEN_WIDTH && py >= 0 && py < SCREEN_HEIGHT)
                vid_mem[py * SCREEN_WIDTH + px] = color;
        }
}

// ------------------------------------------------------
// Reset pipe when it goes off-screen
// ------------------------------------------------------
void reset_pipe()
{
    pipe_x = SCREEN_WIDTH;
    pipe_gap_y = 30 + qran_range(0, SCREEN_HEIGHT - GAP_HEIGHT - 30);
}

const unsigned int* bannerFrames[] = {
    game_banner_1Bitmap,
    game_banner_2Bitmap,
    game_banner_3Bitmap,
    game_banner_4Bitmap,
    game_banner_5Bitmap,
    game_banner_6Bitmap,
    game_banner_7Bitmap
};

// ------------------------------------------------------
// Main game loop
// ------------------------------------------------------
int main(void)
{
    REG_DISPCNT = DCNT_MODE3 | DCNT_BG2;
    irq_init(NULL);
    irq_add(II_VBLANK, NULL);


    const unsigned int* bg_src = game_backgroundBitmap;

    for (int y = 16; y < SCREEN_HEIGHT; y++)     // from 16 to 159
    {
        for (int x = 0; x < SCREEN_WIDTH; x += 2)
        {
            unsigned int pixpair = *bg_src++;
            MODE3_FB[y * SCREEN_WIDTH + x]     = pixpair & 0xFFFF;
            MODE3_FB[y * SCREEN_WIDTH + x + 1] = pixpair >> 16;
        }
    }

    int drawX = 120;
    int drawY = 80;

    const unsigned int* bird_src = birdBitmap;  // matches 'unsigned int birdBitmap[512]'

    for (int y = 0; y < SPRITE_SIZE; y++)
    {
        for (int x = 0; x < SPRITE_SIZE; x += 2)
        {
            unsigned int pixpair = *bird_src++;

            unsigned short left  = pixpair & 0xFFFF;
            unsigned short right = pixpair >> 16;

            if (left != TRANSPARENT_COLOR)
                MODE3_FB[(y + drawY) * SCREEN_WIDTH + (x + drawX)] = left;
            if (right != TRANSPARENT_COLOR)
                MODE3_FB[(y + drawY) * SCREEN_WIDTH + (x + drawX) + 1] = right;
        }
    }



    int frame = 0;
    int frameDelay = 0;
    while(1){

        const unsigned int* src = bannerFrames[frame];
        for (int y = 0; y < 16; y++)
            for (int x = 0; x < 240; x += 2)
            {
                unsigned int pixpair = *src++;
                MODE3_FB[y * SCREEN_WIDTH + x]     = pixpair & 0xFFFF;
                MODE3_FB[y * SCREEN_WIDTH + x + 1] = pixpair >> 16;
            }

        // Wait a bit (e.g., 10 frames)
        vid_vsync();
        if (++frameDelay > 10) {
            frameDelay = 0;
            frame = (frame + 1) % 6; // Loop frames 0–3
        }
    }
    // {
    //     // Input (A = flap)
    //     key_poll();
    //     if(key_hit(KEY_A))
    //         bird_vel = JUMP_VELOCITY;

    //     // Physics
    //     bird_vel += GRAVITY;
    //     bird_y   += bird_vel;
    //     if(bird_y < 0) { bird_y = 0; bird_vel = 0; }
    //     if(bird_y > SCREEN_HEIGHT - BIRD_SIZE)
    //         bird_y = SCREEN_HEIGHT - BIRD_SIZE;

    //     // Pipe movement
    //     pipe_x -= PIPE_SPEED;
    //     if(pipe_x + PIPE_WIDTH < 0)
    //         reset_pipe();

    //     // Collision check
    //     bool hit = false;
    //     if(BIRD_X + BIRD_SIZE > pipe_x && BIRD_X < pipe_x + PIPE_WIDTH)
    //     {
    //         if(bird_y < pipe_gap_y || bird_y + BIRD_SIZE > pipe_gap_y + GAP_HEIGHT)
    //             hit = true;
    //     }

    //     // Draw frame
    //     m3_fill(CLR_CYAN);                                // background sky
    //     draw_rect(BIRD_X, bird_y, BIRD_SIZE, BIRD_SIZE,   // bird
    //               hit ? CLR_RED : RGB15(31, 23, 0));      // brownish when alive, red if hit
    //     draw_rect(pipe_x, 0, PIPE_WIDTH, pipe_gap_y, CLR_GREEN);
    //     draw_rect(pipe_x, pipe_gap_y + GAP_HEIGHT,
    //               PIPE_WIDTH, SCREEN_HEIGHT - pipe_gap_y - GAP_HEIGHT, CLR_GREEN);

    //     vid_vsync();
    // }
    

    return 0;
}
