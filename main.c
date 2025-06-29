/*
 * Written by
 *  __          _            ____  _____
   / /   ____ _(_)___  _  __( __ )/ ___/
  / /   / __ `/ / __ \| |/_/ __  / __ \ 
 / /___/ /_/ / / / / />  </ /_/ / /_/ / 
/_____/\__,_/_/_/ /_/_/|_|\____/\____/  
                                        
 * https://github.com/Lainx86/
 *
 * Simple snake game programmed in C.
 * Alot of this is spaghetti code and kinda unreadable, but this is my frist project in C soooo...
 * I'm unsure if this is C99 compatible, definitely C23 tho.
 * Runs in the terminal: in the project's directory, run `make` and then `./snake_tui`.
 *
 * Licensed under the MIT License (https://mit-license.org/)
 * Program finished 29/06/2025. (dd/mm/yyyy)
 */

#include <stdio.h>
#include <unistd.h>
#include <locale.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <curses.h>  // requires installing ncurses package on Arch (and linking with gcc using -lncurses)

#define COLUMNS    20
#define ROWS       11  // set an odd value for this
#define TICK_TIME  250000  // time between each ingame clock tick in microseconds

enum CellState {
  EMPTY = 0,
  SNAKE = 1,
  APPLE = 2,
};

/* We add certain identical values to the values of the enum fields.
 * This is intentional, as at a given orientation, there is always one that we cannot switch to,
 * the opposite one to us. If we are headed north, we cannot change to south. */ 
enum SnakeDirection {
  NORTH = 0,
  SOUTH = 180,
  WEST = 270,
  EAST = 90,
};

enum CellState grid[ROWS][COLUMNS];
int apple[2];  // grid coordinates for current apple
int score = 0;

typedef struct snake_node {  // we use a linked list type structure to describe the snake entity
  int pos[2];  // (x;y) coords
  int next_pos[2];
  struct snake_node *next;
} snakeNode_t;

snakeNode_t *head = NULL;

static void free_all() {
snakeNode_t *current = head;

  while (current != NULL) {
    snakeNode_t *tmp = current;
    current = current->next;
    free(tmp);
  }

}

static void int_handler(int wtv) {
  free_all();

  endwin();
  printf("Exited. See you next time! ヾ(＾ ∇ ＾)\n");
  exit(0);
}

static int update_snake()
{
  snakeNode_t *current = head;
  while (current != NULL) {
    grid[current->pos[1]][current->pos[0]] = SNAKE;
    current = current->next;
  }
  return 0;
}

static int update_apple() {
  grid[apple[1]][apple[0]] = APPLE;
  return 0;
}

static int check_apple() {
  if (!memcmp(apple, head->pos, sizeof(apple))) {
    score++;
    grid[apple[1]][apple[0]] = EMPTY;

    int new_apple[] = {rand() % COLUMNS, rand() % ROWS};
    memcpy(apple, new_apple, sizeof(new_apple));

    snakeNode_t *current = head;

    while (current->next != NULL) {
      current = current->next;
    }

    current->next = (snakeNode_t *)malloc(sizeof(snakeNode_t));
    int new_node[] = {current->pos[0]-1, current->pos[1]};
    memcpy(current->next->pos, new_node, sizeof(new_node));
    current->next->next = NULL;
  }

  return 0;
}

static int move_snake(enum SnakeDirection orientation) 
{
  snakeNode_t *current = head;

  switch (orientation) {
    case EAST:
      int east_pos[2] = {current->pos[0]+1,current->pos[1]};
      memcpy(current->next_pos, east_pos, sizeof(east_pos));
      break;
    case NORTH:
      int north_pos[2] = {current->pos[0],current->pos[1]-1};
      memcpy(current->next_pos, north_pos, sizeof(north_pos));
      break;
    case SOUTH:
      int south_pos[2] = {current->pos[0],current->pos[1]+1};
      memcpy(current->next_pos, south_pos, sizeof(south_pos));
      break;
    case WEST:
      int west_pos[2] = {current->pos[0]-1,current->pos[1]};
      memcpy(current->next_pos, west_pos, sizeof(west_pos));
      break;
  }

  while (current != NULL) {
    if (current->next != NULL) {
      memcpy(current->next->next_pos, current->pos, sizeof(current->pos));
    }
    memcpy(current->pos, current->next_pos, sizeof(current->next_pos));
    current = current->next;
  }


  for (int i = 0; i < ROWS; i++) {  // start by clearing the snake from the grid
    for (int j = 0; j < COLUMNS; j++) {
      if (grid[i][j] != APPLE) {
        grid[i][j] = EMPTY;
      }
    }
  }

  update_snake();

  return 0;
}

static void check_collisions() {
  bool within_rows = (0 < head->pos[1]) && (head->pos[1] < ROWS);
  bool within_columns = (0 < head->pos[0]) && (head->pos[0] < COLUMNS);
  if (!within_rows || !within_columns) {
    endwin();
    free_all();
    printf("You hit your head against the wall! ᕙ( ᗒᗣᗕ )ᕗ | Score: %d\n", score);
    exit(0);
  }
  
  snakeNode_t *current = head->next;

  while (current != NULL) {
    if (!memcmp(head->pos, current->pos, sizeof(head->pos))) {
      endwin();
      free_all();
      printf("You bit you tail! (╥﹏╥) | Score: %d\n", score);
      exit(0);
    }
    current = current->next;
  }
}

// Populate all items in array with 0 values. (dimensions of multidim array defined by COLUMNS and ROWS)
static int populate_grid(enum CellState to_populate[ROWS][COLUMNS])
{
  for (int i = 0; i < ROWS; i++) {
    for (int j = 0; j < COLUMNS; j++) {
      to_populate[i][j] = EMPTY;  // use CellState enum
    }
  }

  int starting_y = (ROWS+1)/2-1;  // get middle row, -1 because we're 0-indexed :)
  int starting_x = 2;  // arbitrary starting value
  int head_pos[2] = {starting_x, starting_y};  // temporary. but required to use `memcpy` later on
  
  head = (snakeNode_t *)malloc(sizeof(snakeNode_t));
  if (head == NULL) {
    return 1;  // something bad happened at malloc ^
  }
  memcpy(head->pos, head_pos, sizeof(head_pos));
  head->next = (snakeNode_t *)malloc(sizeof(snakeNode_t));

  int next_pos[] = {starting_x-1, starting_y};
  memcpy(head->next->pos, next_pos, sizeof(next_pos));
  head->next->next = NULL;

  update_snake();

  to_populate[starting_y][COLUMNS-3] = APPLE;
  int base_apple[] = {COLUMNS-3, starting_y};
  memcpy(apple, base_apple, sizeof(base_apple));
  return 0;
}

static int render_grid(enum CellState to_render[ROWS][COLUMNS]) {
  move(0, 0);  // move to the beginning of the window..
  clrtobot();  // ...and clear to the end
  
  printw("To exit: Ctrl+C\n");

  for (int i = 0; i < ROWS; i++) {
    for (int j = 0; j < COLUMNS; j++) {
      if (to_render[i][j] == EMPTY) {
        printw("□");
      } else if (to_render[i][j] == SNAKE) {
        printw("■");
      } else if (to_render[i][j] == APPLE) {
        printw("●");
      }
    }
    printw("\n");
    refresh();
  }
  refresh();
  return 0;
}

static int main_loop() {
  unsigned tick_count = 0;
  enum SnakeDirection orien_buff = EAST;
  enum SnakeDirection orien = EAST;

  while (1) {  // todo: use dedicated constants like KEY_UP OR KEY_DOWN instead of key codes
    check_apple();
    update_apple();
    check_collisions();
    move_snake(orien);
    render_grid(grid);
    int ch = 0;  // reset input buffer, probably a better way to do this but wtv
    printw("Score: %d  ٩(^ᗜ^ )و ´\n", score);
    refresh();
    ch = getch();  // wait for key press
    if (ch != ERR) {  // check if any key presses were actually made.
      switch (ch) {
        case KEY_LEFT:
          orien_buff = WEST;
          break;
        case KEY_DOWN:
          orien_buff = SOUTH;
          break;
        case KEY_RIGHT:
          orien_buff = EAST;
          break;
        case KEY_UP:
          orien_buff = NORTH;
          break;
      }
    }
    if (abs(orien - orien_buff) == 180 || orien == orien_buff) {  // check if new orientation can be turned to (i.e. we arent making a 180 degree turn)
      orien_buff = orien;  // aka do nothing
    } else {
      orien = orien_buff;
    }
    usleep(TICK_TIME);
    tick_count = tick_count + 1;
  }
  endwin();  // restore terminal
}

int main(int argc, char *argv[])
{
  signal(SIGINT, int_handler);  // handle Ctrl+C by the user

  setlocale(LC_ALL, "en_US.UTF-8");  // set locale to support wide characters we will be using

  initscr();  // init curses mode
  cbreak();  // disable line buffering (to avoid needing newline char)
  noecho();  // hide input from screen
  keypad(stdscr, TRUE);  // enable arrow keys
  nodelay(stdscr, TRUE);  // makes getch() non-blocking
 
  populate_grid(grid); // run once to be able to dynamically change grid dimensions by tweaking ROWS and COLUMNS
  main_loop();
  return 0;
}

