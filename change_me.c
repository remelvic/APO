/*******************************************************************
  Project main function template for MicroZed based MZ_APO board
  designed by Petr Porazil at PiKRON

  change_me.c      - main file

  include your name there and license for distribution.

  Remove next text: This line should not appear in submitted
  work and project name should be change to match real application.
  If this text is there I want 10 points subtracted from final
  evaluation.

 *******************************************************************/

#define _POSIX_C_SOURCE 200112L

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include <unistd.h>

#include "mzapo_parlcd.h"
#include "mzapo_phys.h"
#include "mzapo_regs.h"

#define SCREEN_WIDTH 320
#define SCREE_HEIGHT 480  

typedef struct{
  int x,y; // position on screen 
  int width,height; // ball width and height
  int dx, dy; // position on screen   

}ball_t;

typedef struct {
  int x,y;
  int width, height;

}stick_t;

ball_t ball;
stick_t stick;
int score[2] = {0};

void start_game(){
  ball.x = 320/2; // ball musi byt v centru ale nejsem si jisty 
  ball.y = 480/2;
  ball.width = 
}
void draw_player1_score(){

}

void draw_player2_score(){

}

void draw_number_of_strokes(){

}

void draw_ball(){

}

void draw_stick(){

}

int main(int argc, char *argv[])
{
  draw_player1_score();
  draw_number_of_strokes();
  draw_player2_score();
  draw_stick();
  draw_ball();
  return 0;
}
