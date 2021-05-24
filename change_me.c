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
#include <math.h>
#include <stdbool.h>
#include <string.h>
#include <termios.h>

#include "mzapo_parlcd.h"
#include "mzapo_phys.h"
#include "mzapo_regs.h"
#include "font_types.h"
#include "serialize_lock.h"

#define HEIGHT_SCREEN 320
#define WIDTH_SCREEN 480
#define M_PI 3.1415

void draw_on_screen(unsigned char *parlcd_mem_base);

unsigned short *fb;

typedef struct
{
  int x, y;          // position on screen
  int width, height; // ball width and height
  int dx, dy;        // position on screen
  int speed_x;
  int speed_y;
} ball_t;

typedef struct
{
  int x, y;
  int width, height;

} stick_t;

ball_t ball_s; // from font_rom8x16.c 0xfe (square)
stick_t stick; // from font_rom8x16.c 0xdb
font_descriptor_t *fdes;
int score[2] = {0};
int scale = 4;

void start_game()
{
  ball_s.x = 320 / 2; // ball musi byt v centru ale nejsem si jisty
  ball_s.y = 480 / 2;
  //ball.width =
}

void init_termios(){
   static struct termios oldt, newt;

 /*tcgetattr gets the parameters of the current terminal
     STDIN_FILENO will tell tcgetattr that it should write the settings
    of stdin to oldt*/
  tcgetattr( STDIN_FILENO, &oldt);
    /*now the settings will be copied*/
  newt = oldt;

    /*ICANON normally takes care that one line at a time will be processed
    that means it will return if it sees a "\n" or an EOF or an EOL*/
  newt.c_lflag &= ~(ICANON);
  newt.c_cc[VMIN] = 0; // bytes until read unblocks.
  newt.c_cc[VTIME] = 0;

    /*Those new settings will be set to STDIN
    TCSANOW tells tcsetattr to change attributes immediately. */
  tcsetattr( STDIN_FILENO, TCSANOW, &newt);
}
// value == jasnost vzdy const 255
// hue mozna taky const
unsigned int hsv2rgb_lcd(int hue, int saturation, int color)
{
  hue = 240; // modry
  float f = ((hue % 60) / 60.0);
  //int p = (255 * (255 - saturation)) / 255; // hodnota minima
  //int q = (255*(255-(saturation*f)))/255;       // hodnota odpovida klesajicim castam
  //int t = (255 * (255 - (saturation * (1.0 - f)))) / 255; // hodnota odpovida rostoucim (viz obrazek)
  unsigned int r, g, b;
  r = g = b = color;

  r >>= 3;
  g >>= 2;
  b >>= 3;
  return (((r & 0x1f) << 11) | ((g & 0x3f) << 5) | (b & 0x1f)); // <0,255>
}

void draw_player1_score(unsigned short color) {}
void draw_player2_score(unsigned short color) {}
void draw_number_of_strokes() {}

void draw_pixel(int x, int y, unsigned color)
{
  fb[(x * WIDTH_SCREEN) + y] = color;
}

void draw_pixel_big(int x, int y, unsigned short color)
{
  int i, j;
  for (i = 0; i < scale; i++)
  {
    for (j = 0; j < scale; j++)
    {
      draw_pixel(x + i, y + j, color);
    }
  }
}

int width_ball(int ball)
{
  int width;
  if (!fdes->width)
  {
    width = fdes->maxwidth;
  }
  else
  {
    width = fdes->width[ball - fdes->firstchar];
  }
  return width;
}

//vytiskne ball na souradnice x, y barvou color
/*void draw_ball(int x, int y, ball_t  ball, unsigned short color){
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
         draw_pixel(x+scale*j, y+scale*i, color);
        }
        val<<=1;
      }
      ptr++;
    }
  }

  int d = 4;

  ball.x += 4;

}*/

void draw_center_stick_and_borders(unsigned char *parlcd_mem_base)
{
  for (int x = 0; x < HEIGHT_SCREEN; x++)
  {
    for (int y = 0; y < WIDTH_SCREEN; y++)
    {
      if (x == 0 || x == 1 || x == HEIGHT_SCREEN - 1 || x == HEIGHT_SCREEN - 2 || y == 0 || y == 1 || y == WIDTH_SCREEN - 1 || y == WIDTH_SCREEN - 2 || y == (WIDTH_SCREEN + 1) / 2 || y == ((WIDTH_SCREEN + 1) / 2) - 1 || y == ((WIDTH_SCREEN + 1) / 2) + 1)
      {
        draw_pixel(x, y, hsv2rgb_lcd(255, 255, 255));
      }
    }
  }
}

void draw_on_screen(unsigned char *parlcd_mem_base)
{
  parlcd_write_cmd(parlcd_mem_base, 0x2c);
  for (size_t k = 0; k < HEIGHT_SCREEN * WIDTH_SCREEN; k++)
  {
    parlcd_write_data(parlcd_mem_base, fb[k]);
  }
}

//draw ball on screen
void draw_ball(ball_t ball)
{
  int d = 15;

  for (int y = 0; y < WIDTH_SCREEN; y++)
  {
    for (int x = 0; x < HEIGHT_SCREEN; x++)
    {
      if (x >= ball.x - d && ball.x <= ball.x + d)
      {
        double dist = sqrt((double)(x - ball.x - d / 2) * (x - ball.x - d / 2) + (y - ball.y - d / 2) * (y - ball.y - d / 2));
        if (dist < (d / 2 + 0.5))
        {
          draw_pixel(x, y, hsv2rgb_lcd(255, 255, 255));
        }
      }
    }
  }
}

_Bool move_ball(ball_t *ball, stick_t player1, stick_t player2)
{
  if ((*ball).x <= 0) //up
  {
    ball->speed_x *= -1;
  }
  else if ((*ball).x >= HEIGHT_SCREEN - 20) //down
  {
    ball->speed_x *= -1;
  }

  else if ((*ball).y <= 0 || (*ball).y >= WIDTH_SCREEN) // sides
  {
    return true;
  }
  else if (((*ball).y <= player1.y + player1.width && (*ball).x >= player1.x && (*ball).x <= player1.x + player1.height) || ((*ball).y >= player2.y - 20 && (*ball).x >= player2.x && (*ball).x <= player2.x + player2.height))
  { // player stick
    ball->speed_y *= -1;
  }

  ball->x += ball->speed_x;
  ball->y += ball->speed_y;
  return false;
}

//draw players sticks
void draw_stick(stick_t stick)
{
  for (int y = 0; y < WIDTH_SCREEN; y++)
  {
    for (int x = 0; x < HEIGHT_SCREEN; x++)
    {
      if ((x >= stick.x && y >= stick.y && x <= stick.x + stick.height && y <= stick.y + stick.width))
      {
        draw_pixel(x, y, hsv2rgb_lcd(255, 255, 255));
      }
    }
  }
}

void move_stick(stick_t *stick, int direction){
  stick->x += 5*direction;
}

size_t char_offset(font_descriptor_t *fdes, char ch)
{
  if ((ch >= fdes->firstchar) && ((ch - fdes->firstchar) < fdes->size))
  {
    return (ch - fdes->firstchar) * fdes->height;
  }
  else
  {
    return 0;
  }
}

void draw_char(int x, int y, font_descriptor_t *fdes, char ch)
{
  int width = char_width(fdes, ch);
  int height = fdes->height;
  size_t bitmap_offset = char_offset(fdes, ch);
  int lcd_row = y;
  for (int font_row = 0; font_row < height; font_row++)
  {
    uint16_t bits = *(fdes->bits + bitmap_offset + font_row);
    for (int rs = 0; rs < scale; rs++)
    {
      int lcd_col = x;
      for (int font_col = 0; font_col < width; font_col++)
      {
        int draw = (bits << font_col) & 0x8000;
        for (int cs = 0; cs < scale; cs++)
        {
          if (draw)
            draw_pixel(lcd_col, lcd_row, hsv2rgb_lcd(255, 255, 255));
          lcd_col++;
        }
      }
      lcd_row++;
    }
  }
}
void draw_text(char *str, int x, int y, font_descriptor_t *fdes)
{
  for (int i = 0; i < strlen(str); i++)
  {
    draw_char(x, y, fdes, str[i]);
    x += char_width(fdes, str[i]);
  }
}

int char_width(font_descriptor_t *fdes, int ch)
{
  int width = 0;
  if ((ch >= fdes->firstchar) && (ch - fdes->firstchar < fdes->size))
  {
    ch -= fdes->firstchar;
    if (!fdes->width)
    {
      width = fdes->maxwidth;
    }
    else
    {
      width = fdes->width[ch];
    }
  }
  return width;
}

void clear_map(int ptr, unsigned char *parlcd_mem_base)
{
  for (int i = 0; i < HEIGHT_SCREEN; i++)
  {
    for (int j = 0; j < WIDTH_SCREEN; j++)
    {
      draw_pixel(i, j, hsv2rgb_lcd(255, 255, 0));
    }
  }
}


int main(int argc, char *argv[])
{
  

  /*if (serialize_lock(1) <= 0) {
    printf("System is occupied\n");

    if (1) {
      printf("Waitting\n");
       Wait till application holding lock releases it or exits 
    /*    serialize_lock(0);
    }
  }*/
  ball_t ball;
  ball.x = HEIGHT_SCREEN / 2;
  ball.y = WIDTH_SCREEN / 2;
  ball.speed_x = 2;
  ball.speed_y = 2;

  stick_t stick_player1;
  stick_player1.x = 100;
  stick_player1.y = 20;
  stick_player1.width = 4;
  stick_player1.height = 100;

  stick_t stick_player2;
  stick_player2.x = 100;
  stick_player2.y = 460;
  stick_player2.width = 4;
  stick_player2.height = 100;

  fb = (unsigned short *)malloc(HEIGHT_SCREEN * WIDTH_SCREEN * 2);
  unsigned char *parlcd_mem_base;
  //int i, j; // i = 0 .. 320(height) j = 0 .. 480
  printf("Hello\n");

  //mapovani fizicke pameti do Virtualniho prostoru
  parlcd_mem_base = map_phys_address(PARLCD_REG_BASE_PHYS, PARLCD_REG_SIZE, 0);
  if (parlcd_mem_base == NULL)
  {
    fprintf(stderr, "Mapping fails!\n");
    exit(1);
  }

  //side border and stick in middle
  draw_center_stick_and_borders(parlcd_mem_base);
  //ball on screen
  draw_ball(ball);

  int ptr = 0;
  _Bool out_of_board = false;
  char c;
  init_termios();
  int cfmakeraw(struct termios *termios_p);
  while (out_of_board == false && c != 113) // q ukonci hru
  {
    clear_map(ptr, parlcd_mem_base);
    int ret = read(0, &c, 1);
    if (c == 119) //klavesa 'w'
    {
    //printf("1. %d\n",ret);
      move_stick(&stick_player1,-1);
      move_stick(&stick_player2,-1);

      c=0;
    }
    if (c == 115) //klavesa 's'
    {
      //    printf("2. %d\n",ret);
     move_stick(&stick_player1,1);
     move_stick(&stick_player2,1);

    c=0;
    }
   if (ret==0){
        // printf("3. %d\n",ret);

     move_stick(&stick_player1,0);
     move_stick(&stick_player2,0);
   }
     draw_stick(stick_player1);
     draw_stick(stick_player2);
    
    out_of_board = move_ball(&ball, stick_player1, stick_player2);
    draw_center_stick_and_borders(parlcd_mem_base);
     
    draw_ball(ball);
    draw_on_screen(parlcd_mem_base);
  }

  //    struct timespec loop_delay = {.tv_sec = 0, .tv_nsec = 120 * 1000 * 1000};
  //buffer na jeden obrazek

  // do obrazku budeme malovat
  //  int k;
  //  float x = 1;
  // float y = 1;
  //unsigned int col = 0xffffff;

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
  clear_map(ptr, parlcd_mem_base);

  int p = 10;
  char str[] = "Goodbye world";
  char *ch = str;
  fdes = &font_winFreeSystem14x16;
  //draw_text(ch, 0, 0, fdes );

  draw_on_screen(parlcd_mem_base);
  sleep(5);
  clear_map(ptr, parlcd_mem_base);
  draw_on_screen(parlcd_mem_base);
  printf("Goodbye\n");
  /*
  draw_player1_score(color);
  draw_number_of_strokes();
  draw_player2_score(color);
  draw_stick();
  draw_ball();
  */

  //serialize_unlock();
  return 0;
}
