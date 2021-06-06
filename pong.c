/*******************************************************************
  Project main function template for MicroZed based MZ_APO board
  designed by Petr Porazil at PiKRON

  pong.c      - main file

  @Daniel Koval, Victor Remel


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
#include <netinet/in.h>
#include <netdb.h>
#include <strings.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>

#include "mzapo_parlcd.h"
#include "mzapo_phys.h"
#include "mzapo_regs.h"
#include "font_types.h"
//#include "serialize_lock.h"

#define HEIGHT_SCREEN 320
#define WIDTH_SCREEN 480
#define M_PI 3.1415
#define PORT 8088
#define SA struct sockaddr

void draw_on_screen(unsigned char *parlcd_mem_base);
void tcp_implements_server(int sockfd);
void tcp_connect_server();
void tcp_implements_client(int sockfd);
void tcp_connect_client();
int char_width(font_descriptor_t *fdes, int ch);

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
int scale = 4;
uint32_t player1_score = 1073741824;
uint32_t player2_score = 1;
uint32_t score = 0;
/**
 * Always reads from stdin waits for w or s or ↑(8) or ↓(2)
 */
void init_termios()
{
  static struct termios oldt, newt;
  tcgetattr(STDIN_FILENO, &oldt);
  /*now the settings will be copied*/
  newt = oldt;

  /*ICANON normally takes care that one line at a time will be processed
    that means it will return if it sees a "\n" or an EOF or an EOL*/
  newt.c_lflag &= ~(ICANON);
  newt.c_cc[VMIN] = 0; // bytes until read unblocks.
  newt.c_cc[VTIME] = 0;

  /*Those new settings will be set to STDIN
    TCSANOW tells tcsetattr to change attributes immediately. */
  tcsetattr(STDIN_FILENO, TCSANOW, &newt);
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
_Bool move_ball(ball_t *ball, stick_t player1, stick_t player2, unsigned char *mem_base)
{
  if ((*ball).x <= 0) //up
  {
    ball->speed_x *= -1;
  }
  else if ((*ball).x >= HEIGHT_SCREEN - 20) //down
  {
    ball->speed_x *= -1;
  }

  else if ((*ball).y <= 0) // sides
  {
    draw_led_line(player2_score, mem_base);
    player2_score<<=1;
    *(volatile uint32_t *)(mem_base + SPILED_REG_LED_RGB1_o) = 0xff0000;
    return true;
  }
  else if ((*ball).y >= WIDTH_SCREEN) // sides
  {
    draw_led_line(player1_score, mem_base);
    *(volatile uint32_t *)(mem_base + SPILED_REG_LED_RGB2_o) = 0xff0000;
    player1_score/=2;
    printf("%d\n", player1_score);
    return true;
  }
  else if ((*ball).y <= player1.y + player1.width && (*ball).x >= player1.x && (*ball).x <= player1.x + player1.height)
  { // player stick
    ball->speed_y *= -1;
    *(volatile uint32_t *)(mem_base + SPILED_REG_LED_RGB1_o) = 0x00ff00;
  }
  else if ((*ball).y >= player2.y - 20 && (*ball).x >= player2.x && (*ball).x <= player2.x + player2.height)
  { // player stick
    ball->speed_y *= -1;
    *(volatile uint32_t *)(mem_base + SPILED_REG_LED_RGB2_o) = 0x00ff00;
  }

  ball->x += ball->speed_x;
  ball->y += ball->speed_y;
  return false;
}



void draw_led_line(int score_temp, unsigned char *mem_base){
  score +=  score_temp;
  *(volatile uint32_t*)(mem_base + SPILED_REG_LED_LINE_o) = score;
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
void move_stick(stick_t *stick, int direction)
{

  stick->x += 5 * direction;
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

void tcp_implements_server(int sockfd)
{
  char buff[80];
  int n;
  int ret;
  //infinite loop while(true)
  for (;;)
  {
    bzero(buff, 80);
    ret = read(sockfd, buff, sizeof(buff));
    if (ret == -1)
      printf("Failed read. In line %d , in file %s\n", __LINE__, __FILE__);

    bzero(buff, 80);
    n = 0;
    while ((buff[n++] = getchar()) != '\n')
      ;
    ret = write(sockfd, buff, sizeof(buff));
    if (ret == -1)
      printf("Failed write. In line %d , in file %s\n", __LINE__, __FILE__);

    if (strncmp("exit", buff, 4) == 0)
    {
      printf("Server Exit...\n");
      break;
    }
  }
}

void tcp_connect_server()
{
  int sockfd, connfd; //len;
  struct sockaddr_in servaddr, cli;
  socklen_t len;

  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd == -1)
  {
    printf("socket creation failed...\n");
    exit(0);
  }
  else
  {
    printf("Socket successfully created..\n");
  }
  bzero(&servaddr, sizeof(servaddr));

  //assign IP,PORT
  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
  servaddr.sin_port = htons(PORT);

  if ((bind(sockfd, (SA *)&servaddr, sizeof(servaddr))) != 0)
  {
    printf("socket bind failed...\n");
    exit(0);
  }
  else
  {
    printf("Socket successfully binded...\n");
  }

  if ((listen(sockfd, 5)) != 0)
  {
    printf("Listen failed...\n");
    exit(0);
  }
  else
  {
    printf("Server listening..\n");
  }
  len = sizeof(cli);

  //accept the data packet from client and verification
  connfd = accept(sockfd, (SA *)&cli, &len);
  if (connfd < 0)
  {
    printf("Server accept failed...\n");
    exit(0);
  }
  else
  {
    printf("Server accept the client..\n");
  }

  tcp_implements_server(connfd);

  close(sockfd);
}

void tcp_implements_client(int sockfd)
{
  char buff[80];
  int n;
  int ret;

  for (;;)
  {
    bzero(buff, sizeof(buff));
    printf("Enter the string: ");
    n = 0;
    while ((buff[n++] = getchar()) != '\n')
      ;

    ret = write(sockfd, buff, sizeof(buff));
    if (ret == -1)
      printf("Failed write. In line %d , in file %s\n", __LINE__, __FILE__);

    bzero(buff, sizeof(buff));

    ret = read(sockfd, buff, sizeof(buff));
    if (ret == -1)
      printf("Failed read. In line %d , in file %s\n", __LINE__, __FILE__);

    printf("From Server: %s", buff);
    if ((strncmp(buff, "exit", 4)) == 0)
    {
      printf("Client Exut...\n");
      break;
    }
  }
}

void tcp_connect_client()
{
  int sockfd;
  struct sockaddr_in servaddr;

  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd == -1)
  {
    printf("Socket creation failed..\n");
    exit(0);
  }
  else
  {
    printf("Socket successfully created..\n");
  }
  bzero(&servaddr, sizeof(servaddr));
  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
  servaddr.sin_port = htons(PORT);

  if (connect(sockfd, (SA *)&servaddr, sizeof(servaddr)) != 0)
  {
    printf("Connection with the server failed..\n");
    exit(0);
  }
  else
  {
    printf("Connected to the server..\n");
  }

  tcp_implements_client(sockfd);

  close(sockfd);
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
        serialize_lock(0);
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
  unsigned char *mem_base;
  printf("Hello\n");

  parlcd_mem_base = map_phys_address(PARLCD_REG_BASE_PHYS, PARLCD_REG_SIZE, 0);
  if (parlcd_mem_base == NULL)
  {
    fprintf(stderr, "Mapping fails!\n");
    exit(1);
  }
  parlcd_hx8357_init(parlcd_mem_base);
  mem_base = map_phys_address(SPILED_REG_BASE_PHYS, SPILED_REG_SIZE, 0);

  //side border and stick in middle
  draw_center_stick_and_borders(parlcd_mem_base);
  //ball on screen
  draw_ball(ball);

  _Bool out_of_board = false;
  char c;
  init_termios();
  int cfmakeraw(struct termios * termios_p);
  while (c != 113) // q end game
  {
    *(volatile uint32_t *)(mem_base + SPILED_REG_LED_RGB1_o) = 0x000000;
    *(volatile uint32_t *)(mem_base + SPILED_REG_LED_RGB2_o) = 0x000000;
    clear_map(parlcd_mem_base);
    int ret = read(0, &c, 1);
    if (c == 113)
    {
      break;
    }

    switch (c)
    {
    case 119: //klavesa 'w'
      move_stick(&stick_player1, -1);
      break;
    case 56: // klavesa '8'
      move_stick(&stick_player2, -1);
      break;
    case 115: //klavesa 's'
      move_stick(&stick_player1, 1);
      break;
    case 50: // klavesa '2'
      move_stick(&stick_player2, 1);
      break;
    default:
      c = 0;
      break;
    }

    if (ret == 0)
    {
      c = 0;
      move_stick(&stick_player1, 0);
      move_stick(&stick_player2, 0);
    }
    draw_stick(stick_player1);
    draw_stick(stick_player2);

    out_of_board = move_ball(&ball, stick_player1, stick_player2, mem_base);



    if (out_of_board)
    {
      ball.x = HEIGHT_SCREEN / 2;
      ball.y = WIDTH_SCREEN / 2;
    }

    draw_center_stick_and_borders(parlcd_mem_base);

    draw_ball(ball);
    draw_on_screen(parlcd_mem_base);
  }

  //    struct timespec loop_delay = {.tv_sec = 0, .tv_nsec = 120 * 1000 * 1000};

  clear_map(parlcd_mem_base);

  /*
  int p = 10;
  //char str[] = "Goodbye world";
  //char *ch = str;
  //fdes = &font_winFreeSystem14x16;
  draw_text(ch, 0, 0, fdes );
  */
  draw_on_screen(parlcd_mem_base);
  sleep(5);
  clear_map(parlcd_mem_base);
  draw_on_screen(parlcd_mem_base);
  printf("Goodbye\n");
  /*
  draw_player_score(color);
  */
  *(volatile uint32_t *)(mem_base + SPILED_REG_LED_RGB1_o) = 0x000000;
  *(volatile uint32_t *)(mem_base + SPILED_REG_LED_RGB2_o) = 0x000000;
  *(volatile uint32_t*)(mem_base + SPILED_REG_LED_LINE_o) = 0;

  //serialize_unlock();
  return 0;
}
