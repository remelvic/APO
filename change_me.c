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

ball_t ball;     // from font_rom8x16.c 0xfe (square)
stick_t stick;   // from font_rom8x16.c 0xdb
font_descriptor_t *fdes;
int score[2] = {0};
int scale = 4;


void start_game(){
  ball.x = 320/2; // ball musi byt v centru ale nejsem si jisty 
  ball.y = 480/2;
  //ball.width = 
}

// value == jasnost vzdy const 255
// hue mozna taky const
unsigned int hsv2rgb_lcd(int hue, int saturation){
  hue = 240; // modry
  float f = ((hue%60)/60.0);
  int p = (255*(255-saturation))/255;           // hodnota minima
  int q = (255*(255-(saturation*f)))/255;       // hodnota odpovida klesajicim castam
  int t = (255*(255-(saturation*(1.0-f))))/255; // hodnota odpovida rostoucim (viz obrazek)
  unsigned int r,g,b;
  r = t; g = p; b = 255;
  r>>=3;
  g>>=2;
  b>>=3;
  return (((r&0x1f)<<11)|((g&0x3f)<<5)|(b&0x1f)); // <0,255>
}


void draw_player1_score(unsigned short color){

}

void draw_player2_score(unsigned short color){

}

void draw_number_of_strokes(){

}

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

int width_ball(int ch){
  int width;
  if (!fdes->width){
    width = fdes->maxwidth;
  } else {
    width = fdes->width[ch-fdes->firstchar];
  }
  return width;
}


void draw_ball(int x, int y, char ch, unsigned short color){
  int w = width_ball(ch);
  const font_bits_t *ptr;
  if ((ch >= fdes->firstchar) && (ch-fdes->firstchar < fdes->size)) {
    if (fdes->offset) {
      ptr = &fdes->bits[fdes->offset[ch-fdes->firstchar]];
    } else {
      int bw = (fdes->maxwidth+15)/16;
      ptr = &fdes->bits[(ch-fdes->firstchar)*bw*fdes->height];
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

void draw_stick(){
  int x;
}



int main(int argc, char *argv[])
{ 
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
      parlcd_write_data(parlcd_mem_base, hsv2rgb_lcd(j, 255)); // zobrazeni vsech pixelu
    }
  }
  sleep(5);

  //buffer na jeden obrazek
  fb = (unsigned short *)malloc(HEIGHT_SCREEN*WIDTH_SCREEN*2);

  // do obrazku budeme malovat

  /*
  draw_player1_score(color);
  draw_number_of_strokes();
  draw_player2_score(color);
  draw_stick();
  draw_ball();
  */
  return 0;
}
