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
#include "font_types.h"

#define HEIGHT_SCREEN 320
#define WIDTH_SCREEN 480
#define M_PI 3.1415


void draw_on_screen(unsigned char *parlcd_mem_base);

unsigned short *fb;

typedef struct{
  int x,y; // position on screen 
  int width,height; // ball width and height
  int dx, dy; // position on screen   

}ball_t;

typedef struct {
  int x,y;
  int width, height;

}stick_t;

ball_t ball_s;     // from font_rom8x16.c 0xfe (square)
stick_t stick;   // from font_rom8x16.c 0xdb
font_descriptor_t *fdes;
int score[2] = {0};
int scale = 4;


void start_game(){
  ball_s.x = 320/2; // ball musi byt v centru ale nejsem si jisty 
  ball_s.y = 480/2;
  //ball.width = 
}

// value == jasnost vzdy const 255
// hue mozna taky const
unsigned int hsv2rgb_lcd(int hue, int saturation){
  hue = 240; // modry
  float f = ((hue%60)/60.0);
  int p = (255*(255-saturation))/255;           // hodnota minima
  //int q = (255*(255-(saturation*f)))/255;       // hodnota odpovida klesajicim castam
  int t = (255*(255-(saturation*(1.0-f))))/255; // hodnota odpovida rostoucim (viz obrazek)
  unsigned int r,g,b;
  r = g = b = 0;
  
  r>>=3;
  g>>=2;
  b>>=3;
  return (((r&0x1f)<<11)|((g&0x3f)<<5)|(b&0x1f)); // <0,255>
}


void draw_player1_score(unsigned short color){}
void draw_player2_score(unsigned short color){}
void draw_number_of_strokes(){}

void draw_pixel(int x, int y, unsigned color){
  if (x >= 0 && x < WIDTH_SCREEN && y >=0 && y < HEIGHT_SCREEN){
    fb[x+WIDTH_SCREEN*y] = color;
  }
}

void draw_pixel_big(int x, int y, unsigned short color){
  int i,j;
  for (i=0; i<scale; i++) {
    for (j=0; j<scale; j++) {
      draw_pixel(x+i, y+j, color);
    }
  }
}

int width_ball(int ball){
  int width;
  if (!fdes->width){
    width = fdes->maxwidth;
  } else {
    width = fdes->width[ball-fdes->firstchar];
  }
  return width;
}

//vytiskne ball na souradnice x, y barvou color
void draw_ball(int x, int y, char ball, unsigned short color){
  int w = width_ball(ball);
  const font_bits_t *ptr;
  if ((ball >= fdes->firstchar) && (ball-fdes->firstchar < fdes->size)) {
    if (fdes->offset) {
      ptr = &fdes->bits[fdes->offset[ball-fdes->firstchar]];
    } else {
      int bw = (fdes->maxwidth+15)/16;
      ptr = &fdes->bits[(ball-fdes->firstchar)*bw*fdes->height];
    }
    int i, j;
    for (i=0; i<fdes->height; i++) {
      font_bits_t val = *ptr;
      for (j=0; j<w; j++) {
        if ((val&0x8000)!=0) {
         draw_pixel_big(x+scale*j, y+scale*i, color);
        }
        val<<=1;
      }
      ptr++;
    }
  }
}

void draw_stick_and_borders( unsigned char *parlcd_mem_base){
  for (int x = 0; x < HEIGHT_SCREEN; x++)
{
  for (int y = 0; y < WIDTH_SCREEN; y++)
  {
    if (x == 0 || x == 1|| x == HEIGHT_SCREEN -1 || x == HEIGHT_SCREEN -2 || y == 0 || y == 1 || y == WIDTH_SCREEN-1 || y == WIDTH_SCREEN-2 || y == WIDTH_SCREEN/2 || y == (WIDTH_SCREEN/2) -1)
    {
   fb[(x*WIDTH_SCREEN)+y] = 0xffffff; 
    }
    
  }
  }
  
draw_on_screen(parlcd_mem_base);

 


}

void draw_on_screen(unsigned char *parlcd_mem_base){
     parlcd_write_cmd(parlcd_mem_base, 0x2c);
for (size_t k = 0;  k < HEIGHT_SCREEN*WIDTH_SCREEN; k++)
{
  parlcd_write_data(parlcd_mem_base, fb[k]);
}
}


int main(int argc, char *argv[])
{ 
  fb = (unsigned short *)malloc(HEIGHT_SCREEN*WIDTH_SCREEN*2);
  unsigned char *parlcd_mem_base;
  int i,j; // i = 0 .. 320(height) j = 0 .. 480 
  printf("Hello\n");

  //mapovani fizicke pameti do Virtualniho prostoru
  parlcd_mem_base = map_phys_address(PARLCD_REG_BASE_PHYS, PARLCD_REG_SIZE, 0);
  if (parlcd_mem_base == NULL){
    fprintf(stderr,"Mapping fails!\n");
    exit (1);
  }

  //cerny obrazek?
  parlcd_hx8357_init(parlcd_mem_base); // inicializace dispaly a zpusobu komunikaci

  parlcd_write_cmd(parlcd_mem_base, 0x2c); // kdyz malovani dokonceno, zobrazime obrazek
  for(i = 0; i < HEIGHT_SCREEN; i++){
    for (j = 0; j < WIDTH_SCREEN; j++){

      parlcd_write_data(parlcd_mem_base, hsv2rgb_lcd(j, 0)); // zobrazeni vsech pixelu
      
    } 
    }

    draw_stick_and_borders(parlcd_mem_base);




  sleep(5);

  struct timespec loop_delay = {.tv_sec = 0, .tv_nsec = 120 * 1000 * 1000};
  //buffer na jeden obrazek
  
  // do obrazku budeme malovat
  int k;
  int ptr = 0;
  float x = 1;
  float y = 1;
  char ball = '@';
  unsigned int col = hsv2rgb_lcd(4,255);
/*
  for (k = 0; k <= 80; k+=5){
    float alfa = ((10+k)*M_PI)/180.0;
    float vx = 32*(M_PI/2.0-alfa);     //vektor rychlosti zatim stejny jako u flying_letters
    float vy = 32*(2.0*alfa/M_PI);
    while (x < WIDTH_SCREEN && y > 0){
      for (ptr = 0; ptr < WIDTH_SCREEN*HEIGHT_SCREEN; ptr++){
        fb[ptr] = 0;
      }
      draw_ball((int)x, 250-(int)y,ball, col);
      x+=vx;
      y+=vy;
      vx = vx*0.97;
      vy = vy*0.97 - 1.0;
      parlcd_write_cmd(parlcd_mem_base, 0x2c);
      for (ptr = 0; ptr < HEIGHT_SCREEN*WIDTH_SCREEN; ptr++){
        parlcd_write_data(parlcd_mem_base, fb[ptr]);
      }
      clock_nanosleep(CLOCK_MONOTONIC, 0, &loop_delay, NULL);
    }
  }*/
  
  ptr = 0;
  parlcd_write_cmd(parlcd_mem_base, 0x2c);
  for (int i = 0; i < HEIGHT_SCREEN; i++){
    for (int j = 0; j < WIDTH_SCREEN; j++){
      fb[ptr] = 0;
      parlcd_write_data(parlcd_mem_base, fb[ptr++]);
    }
  }
  printf("Goodbye\n");
  /*
  draw_player1_score(color);
  draw_number_of_strokes();
  draw_player2_score(color);
  draw_stick();
  draw_ball();
  */
  return 0;
}
