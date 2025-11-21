#ifndef BALL_H
#define BALL_H
#define BALL_X 25
#include <stdbool.h>

void ball_init(void);
void ball_update(bool press_jump);
void reset_ball(void);

extern int ball_Y;

#endif
