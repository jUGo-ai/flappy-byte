#include <tonc.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>
#include "ball.h"
#include "pipes.h"
#include "bytes.h"
#include "../graphics/game_ball.h"
#include "../graphics/game_background_ver2.h"
#include "../graphics/homescreen_state1.h"
#include "../graphics/homescreen_state2.h"
#include "../graphics/game_background_ver2_restart.h"

OBJ_ATTR obj_buffer[128];
bool byte_updated = false;
bool pipe_passed = false;
int game_frame_counter = 0;
extern unsigned char random_byte;
extern const char* current_gate;
bool homescreen_state = false;  // false = state1, true = state2
bool restart_state = false;
int animation_counter = 0;
bool first_time = true;  // Tracks if this is the initial game open

void load_homescreen_state1() {
    memcpy(pal_bg_mem, homescreen_state1Pal, homescreen_state1PalLen);
    memcpy(&tile_mem[0][0], homescreen_state1Tiles, homescreen_state1TilesLen);
    for(int y=0; y<20; y++)
        for(int x=0; x<30; x++)
            se_mem[30][y*32 + x] = homescreen_state1Map[y*30 + x];
}

void load_homescreen_state2() {
    memcpy(pal_bg_mem, homescreen_state2Pal, homescreen_state2PalLen);
    memcpy(&tile_mem[0][0], homescreen_state2Tiles, homescreen_state2TilesLen);
    for(int y=0; y<20; y++)
        for(int x=0; x<30; x++)
            se_mem[30][y*32 + x] = homescreen_state2Map[y*30 + x];
}

void load_game_background() {
    memcpy(pal_bg_mem, game_background_ver2Pal, game_background_ver2PalLen);
    memcpy(&tile_mem[0][0], game_background_ver2Tiles, game_background_ver2TilesLen);
    for(int y=0; y<20; y++)
        for(int x=0; x<30; x++)
            se_mem[30][y*32 + x] = game_background_ver2Map[y*30 + x];
}

void load_restart_background() {
    memcpy(pal_bg_mem, game_background_ver2_restartPal, game_background_ver2_restartPalLen);
    memcpy(&tile_mem[0][0], game_background_ver2_restartTiles, game_background_ver2_restartTilesLen);
    for(int y=0; y<20; y++)
        for(int x=0; x<30; x++)
            se_mem[30][y*32 + x] = game_background_ver2_restartMap[y*30 + x];
}

int main(void) {
    // ------------------------------
    // Display & background setup
    // ------------------------------
    srand(REG_TM0D);
    REG_DISPCNT = DCNT_MODE0 | DCNT_BG0 | DCNT_OBJ | DCNT_OBJ_1D;

    // Load initial homescreen background (state1)
    load_homescreen_state1();

    REG_BG0CNT = BG_CBB(0) | BG_SBB(30) | BG_8BPP | BG_REG_32x32 | BG_PRIO(1);

    // ------------------------------
    // OAM & sprites setup
    // ------------------------------
    oam_init(obj_buffer, 128);
    ball_init();
    pipes_init();
    
    bool key_press = false;
    bool game_started = false;
    int game_second_counter = 0;
    // int flickering_byte = 0;

    while(1) {
        vid_vsync();
        key_poll();
        
        game_frame_counter++;

        if (!game_started) {
            if (first_time) {
                // Homescreen animation
                animation_counter++;
                if (animation_counter >= 30) {
                    animation_counter = 0;
                    homescreen_state = !homescreen_state;
                    if (homescreen_state) {
                        load_homescreen_state2();
                    } else {
                        load_homescreen_state1();
                    }
                }
            } else {
                // Restart animation (slower to reduce glitches)
                animation_counter++;
                if (animation_counter >= 60) {  // Slower toggle (2 seconds)
                    animation_counter = 0;
                    restart_state = !restart_state;
                    if (restart_state) {
                        load_game_background();  // Full reload
                    } else {
                        load_restart_background();  // Full reload
                    }
                }
                // Hide sprites during restart to avoid overlaps
                oam_copy(oam_mem, obj_buffer, 128);  // Update OAM but sprites are hidden
            }

            if (key_hit(KEY_A)) {
                load_game_background();  // Full reload for game start
                pal_bg_mem[1] = game_ballPal[1];  // Set ball color only for game

                game_started = true;
                first_time = false;
                key_press = true;

                game_byte = generate_next_byte();
                byte_logic();          // initialize random_byte and current_gate for this round
                random_pipes();        // now calculate pipe correct/decoy based on current gate/byte
                draw_byte_bits(game_byte, 30, 12, -1);
                oam_copy(oam_mem, obj_buffer, 128);
            }

            
            continue;
        }

        // Game started — normal update
        if (key_hit(KEY_A)){
            key_press = true;
        }

        // if(game_started && key_hit(KEY_A)){
        //     game_started = false;
        //     game_frame_counter = 0;
        //     game_second_counter = 0;
        //     flickering_byte = 0;
        //     byte_updated = false;
        //     pipe_passed = false;
        //     game_byte = generate_next_byte();
            
        //     clear_byte_bits(30, 12, -1);
        //     clear_byte_bits(150, 12, -1);
        //     clear_gate();
        //     reset_ball();
        //     reset_pipes();
        //     oam_copy(oam_mem, obj_buffer, 128);
        //     continue;
        // }

        if (game_frame_counter >= 60){
            game_frame_counter = 0;
            game_second_counter++;
            if (pipe_speed < 3 && game_second_counter % 60 == 0){
                pipe_speed++;
            }
        }

        // ------------------------------
        // Update game objects
        // ------------------------------
        ball_update(key_press);
        pipes_update();

        // Collision with pipes
        int collision_result = pipes_check_collision(BALL_X, ball_Y);
        if (collision_result == 1 || collision_result == 3 || ball_Y <= 23 || ball_Y >= 151)
        {
            game_started = false;
            game_frame_counter = 0;
            game_second_counter = 0;
            // flickering_byte = 0;
            byte_updated = false;
            pipe_passed = false;
            game_byte = generate_next_byte();
            
            clear_byte_bits(30, 12, -1);
            clear_byte_bits(150, 12, -1);
            clear_gate();
            reset_ball();
            reset_pipes();
            oam_copy(oam_mem, obj_buffer, 128);
            continue;
        }

        if (pipes_posX <= 21 && !byte_updated) {
            apply_byte_logic();
            byte_logic();
            byte_updated = true;
            // flickering_byte = 0;
        }
        
        pipe_passed = true;

        // if (flickering_byte <= 30) {
        //     byte_logic();
        // }

        // flickering_byte++;
        oam_copy(oam_mem, obj_buffer, 128);

        key_press = false;
    }
}

// Helper functions for clean background loading
