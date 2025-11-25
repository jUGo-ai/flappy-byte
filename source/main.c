#include <tonc.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>
#include "ball.h"
#include "pipes.h"
#include "../graphics/game_background_ver2.h"

OBJ_ATTR obj_buffer[128];

int main(void) {
    // ------------------------------
    // Display & background setup
    // ------------------------------
    srand(time(NULL));
    REG_DISPCNT = DCNT_MODE0 | DCNT_BG0 | DCNT_OBJ | DCNT_OBJ_1D;

    memcpy(pal_bg_mem, game_background_ver2Pal, game_background_ver2PalLen);
    memcpy(&tile_mem[0][0], game_background_ver2Tiles, game_background_ver2TilesLen);

    for(int y=0; y<20; y++)
        for(int x=0; x<30; x++)
            se_mem[30][y*32 + x] = game_background_ver2Map[y*30 + x];

    // REG_BG0CNT = BG_CBB(0) | BG_SBB(30) | BG_8BPP | BG_REG_32x32;
    REG_BG0CNT = BG_CBB(0) | BG_SBB(30) | BG_8BPP | BG_REG_32x32 | BG_PRIO(1);

    // ------------------------------
    // OAM & sprites setup
    // ------------------------------
    oam_init(obj_buffer, 128);
    ball_init();
    pipes_init();      // <--- initialize pipes
    

    bool key_press = false;
    bool game_started = false;   
    int game_frame_counter = 0;
    int game_second_counter = 0;

    while(1) {
        vid_vsync();
        key_poll();
        game_frame_counter++;

        if (!game_started) {
            if (key_hit(KEY_A)) {
                game_started = true;
                key_press = true;
            }

            oam_copy(oam_mem, obj_buffer, 128);
            continue;   
        }

        // Game started — normal update
        if (key_hit(KEY_A))
            key_press = true;

        if (game_frame_counter >= 60){
            game_frame_counter = 0;
            game_second_counter++;

            // if (pipe_speed < 3 && game_second_counter % 15 == 0){
            //     pipe_speed++;
            // }
        }

        // ------------------------------
        // Update game objects
        // ------------------------------
        ball_update(key_press);
        pipes_update();            // <--- move pipes

        // Collision with pipes
        if(pipes_check_collision(BALL_X, ball_Y))
        {
            game_started = false;
            reset_ball();
            reset_pipes();
            oam_copy(oam_mem, obj_buffer, 128);
            continue;
        }

        // Collision with ceiling or floor
        if(ball_Y <= 23 || ball_Y >= 151)
        {
            game_started = false;
            reset_ball();
            reset_pipes();
            oam_copy(oam_mem, obj_buffer, 128);
            continue;
        }

        
        // ------------------------------
        oam_copy(oam_mem, obj_buffer, 128);

        key_press = false;
    }
}

