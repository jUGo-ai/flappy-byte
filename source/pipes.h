// pipes.h
#ifndef PIPES_H
#define PIPES_H

#include <tonc.h>
#include <stdbool.h>

#define PIPE_WIDTH   16
#define PIPE_GAP     50
#define PIPE_SPEED   1

// Expose the pipe X position so main.c can check it
extern int pipes_posX;
extern int pipe_speed;

// Initialize pipe sprites and starting position
void pipes_init(void);

// Update pipe movement each frame
void pipes_update(void);

// Randomize pipe vertical offsets and reposition them to the right side
void random_pipes(void);

void reset_pipes(void);


bool pipes_check_collision(int ball_x, int ball_y);

#endif
