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

/**
 * Always reads from stdin waits for w or s or ↑(8) or ↓(2)
 */
void init_termios(){
  static struct termios oldt, newt;
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



/**
 * This function works with the color of our elements (sticks and ball)
 *
 * @param hue - always 255 
 * @param saturation - always 255
 * @param color 
 *
 * @return color 
 */
unsigned int hsv2rgb_lcd(int hue, int saturation, int color)
{
  hue = 240; // blue
  unsigned int r, g, b;
  r = g = b = color;

  r >>= 3;
  g >>= 2;
  b >>= 3;
  return (((r & 0x1f) << 11) | ((g & 0x3f) << 5) | (b & 0x1f)); // <0,255>
}

/**
 * This function writes out the goals scored counter
 *
 * @param color - color counter
 */
void draw_player_score(unsigned short color) {}

/**
 * The function gives the pixels a color, thereby writing them out (they appear on the screen)
 *
 * @param x  - width
 * @param y - height 
 * @param color 
 */
void draw_pixel(int x, int y, unsigned color)
{
  fb[(x * WIDTH_SCREEN) + y] = color;
}

/**
  * The function "enlarges" the pixel so that it is visible on the screen
  *
  * @param x - width 
  * @param y - height 
  * @param color
  */
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


/**
 * The function will draw a center line and sticks on the left and right side
 *
 * @param parlcd_mem_base
 */
void draw_center_stick_and_borders(unsigned char *parlcd_mem_base)
{
  for (int x = 0; x < HEIGHT_SCREEN; x++)
  {
    for (int y = 0; y < WIDTH_SCREEN; y++)
    {
      if (x == 0 || x == 1 || x == HEIGHT_SCREEN - 1 || x == HEIGHT_SCREEN - 2 || y == 0 || y == 1 ||
        y == WIDTH_SCREEN - 1 || y == WIDTH_SCREEN - 2 || y == (WIDTH_SCREEN + 1) / 2 ||
        y == ((WIDTH_SCREEN + 1) / 2) - 1 || y == ((WIDTH_SCREEN + 1) / 2) + 1)
      {
        draw_pixel(x, y, hsv2rgb_lcd(255, 255, 255));
      }
    }
  }
}

/**
 * Lets us draw on the screen 
 *
 * @param parlcd_mem_base
 */
void draw_on_screen(unsigned char *parlcd_mem_base)
{
  parlcd_write_cmd(parlcd_mem_base, 0x2c);
  for (size_t k = 0; k < HEIGHT_SCREEN * WIDTH_SCREEN; k++)
  {
    parlcd_write_data(parlcd_mem_base, fb[k]);
  }
}

//draw ball on screen
/**
 * The function draws a ball on the screen
 * 
 * @param ball - we transfer the speed of the ball, its size and position
 */
void draw_ball(ball_t ball)
{
  int diameter = 15;

  for (int y = 0; y < WIDTH_SCREEN; y++)
  {
    for (int x = 0; x < HEIGHT_SCREEN; x++)
    {
      if (x >= ball.x - diameter && ball.x <= ball.x + diameter)
      {
        double dist = sqrt((double)(x - ball.x - diameter / 2) * (x - ball.x - diameter / 2) + 
        (y - ball.y - diameter / 2) * (y - ball.y - diameter / 2));
        if (dist < (diameter / 2 + 0.5))
        {
          draw_pixel(x, y, hsv2rgb_lcd(255, 255, 255));
        }
      }
    }
  }
}

/**
 * The function processes the movement of the ball in all directions
 * 
 * @param ball - we transfer the speed of the ball, its size and position
 * @param player1 - we pass the dimensions and position of the first stick
 * @param player2 - we pass the dimensions and position of the second stick
 *
 * @return if the ball moves left or right, then the function will return true; otherwise, it will change the position
 *         of the ball on the screen and return false 
 */
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
  else if (((*ball).y <= player1.y + player1.width && (*ball).x >= player1.x && (*ball).x <= player1.x + player1.height) ||
    ((*ball).y >= player2.y - 20 && (*ball).x >= player2.x && (*ball).x <= player2.x + player2.height))
  { // player stick
    ball->speed_y *= -1;
  }

  ball->x += ball->speed_x;
  ball->y += ball->speed_y;
  return false;
}

//draw players sticks
/**
 * The function draws our sticks on the screen
 *
 * @param stick - passing the dimensions and parameters of the stick
 */
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

/**
 * The function will moves our sticks
 *
 * @param stick - in this case, we get the coordinates of the stick
 * @param direction - where will we move our stick
 */
void move_stick(stick_t *stick, int direction){
  stick->x += 5*direction;
}

/**
 * The function draws the character ch at x and y coordinates
 *
 * @param x - width
 * @param y - height 
 * @param fdes - representation of our symbol
 * @param ch - our symbol that we want to draw
 */
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

/**
 * The function writes out the whole text 
 *
 * @param str - our line that we want to write out 
 * @param x - wigth 
 * @param y - height 
 * @param fdes - "storage" of representations of our symbols
 */
void draw_text(char *str, int x, int y, font_descriptor_t *fdes)
{
  for (int i = 0; i < strlen(str); i++)
  {
    draw_char(x, y, fdes, str[i]);
    x += char_width(fdes, str[i]);
  }
}

/**
 * The function determines the width of the selected character based on the "stock" of representations fdes
 *
 * @param fdes - "storage" of representations of our symbols
 * @param ch - character
 *
 * @return the width of our character
 */
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

/**
 * The function clears the screen after finishing the game (After pressing the q button)
 * 
 * @param parlcd_mem_base
 */
void clear_map(unsigned char *parlcd_mem_base)
{
  for (int i = 0; i < HEIGHT_SCREEN; i++)
  {
    for (int j = 0; j < WIDTH_SCREEN; j++)
    {
      draw_pixel(i, j, hsv2rgb_lcd(255, 255, 0));
    }
  }
}

/**
 * The main function that launches our game
 */
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

  _Bool out_of_board = false;
  char c;
  init_termios();
  int cfmakeraw(struct termios *termios_p);
  while (out_of_board == false && c != 113) // q end game
  {
    clear_map(parlcd_mem_base);
    int ret = read(0, &c, 1);
    if (c == 119) //klavesa 'w'
    {
    //printf("1. %d\n",ret);
      move_stick(&stick_player1,-1);
      c=0;
    }

    if (c == 56) // klavesa '8' 
    {
      move_stick(&stick_player2,-1);
      c = 0;
    }

    if (c == 115) //klavesa 's'
    {
      //printf("2. %d\n",ret);
     move_stick(&stick_player1,1);
    c=0;
    }

    if (c == 50) // klavesa '2'
    {
      move_stick(&stick_player2,1);
      c = 0;
    }
   if (ret==0){
        //printf("3. %d\n",ret);

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
  
  clear_map(parlcd_mem_base);

  int p = 10;
  char str[] = "Goodbye world";
  char *ch = str;
  fdes = &font_winFreeSystem14x16;
  //draw_text(ch, 0, 0, fdes );

  draw_on_screen(parlcd_mem_base);
  sleep(5);
  clear_map(parlcd_mem_base);
  draw_on_screen(parlcd_mem_base);
  printf("Goodbye\n");
  /*
  draw_player_score(color);
  */

  //serialize_unlock();
  return 0;
}
