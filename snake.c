#include <SDL2/SDL.h>
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_keycode.h>
#include <SDL2/SDL_rwops.h>
#include <SDL2/SDL_surface.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_video.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// window sizing
#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600
#define WINDOW_TITLE "Snake Game"

// snake properties
#define SNAKE_SPEED 4 

#define SNAKE_WIDTH 25
#define SNAKE_HEIGHT 25

// food properties
#define FOOD_WIDTH 20
#define FOOD_HEIGHT 20

// snake growth scale
#define GROWTH_SCALE 5

// maximum length of the snake
#define MAXIMUM_SNAKE_LENGTH 1000

// track the score
int score = 0;

// snake struct
typedef struct {
  int x, y;
  int width, height;
  int dx, dy;
} Snake;

// body segment strucs
typedef struct {
  int x, y;
  int width, height;
} Segment;

// food struct
typedef struct {
  int x, y;
  int width, height;
} Food;

// directions
typedef enum { UP, RIGHT, DOWN, LEFT } Direction;

void generate_random_food(Food *food) {
  food->x = (rand() % (WINDOW_WIDTH / FOOD_WIDTH)) * FOOD_WIDTH;
  food->y = (rand() % (WINDOW_HEIGHT / FOOD_HEIGHT)) * FOOD_HEIGHT;
}

void draw_snake(SDL_Surface *surface, Snake *snake, Segment *segments,
                int length) {
  SDL_Rect rect = {snake->x, snake->y, snake->width, snake->height};
  SDL_FillRect(surface, &rect, SDL_MapRGB(surface->format, 255, 255, 255));

  // draw each segment of the sankes body
  for (int i = 0; i < length; i++) {
    SDL_Rect segments_rect = {segments[i].x, segments[i].y, segments[i].width,
                              segments[i].height};
    SDL_FillRect(surface, &segments_rect,
                 SDL_MapRGB(surface->format, 200, 200, 200));
  }
}

void render_score(SDL_Surface *surface, TTF_Font *font, int score) {
  SDL_Color white = {255, 255, 255};

  // convert score to string
  char score_text[50];
  sprintf(score_text, "Score: %d", score);

  // create the surface for the text
  SDL_Surface *text_surface = TTF_RenderText_Solid(font, score_text, white);
  if (!text_surface) {
    printf("Failed to render text\n");
    return;
  }

  // define where the text should appear
  SDL_Rect text_rect = {10, 10, text_surface->w, text_surface->h};

  // blit the next surface onto the main surface
  SDL_BlitSurface(text_surface, NULL, surface, &text_rect);

  // free the text surface after use
  SDL_FreeSurface(text_surface);
}

void draw_food(SDL_Surface *surface, Food *food) {
  SDL_Rect rect = {food->x, food->y, food->width, food->height};
  SDL_FillRect(surface, &rect, SDL_MapRGB(surface->format, 255, 0, 0));
}

// check for collision between snake's head and food
bool check_collision(Snake *snake, Food *food) {
  if (snake->x < food->x + food->width && snake->x + snake->width > food->x &&
      snake->y < food->y + food->height && snake->y + snake->height > food->y) {
    return true;
  }
  return false;
}

// chech if snakes bites itself
bool check_self_collision(Snake *snake, Segment *segmnet, int length) {
  for (int i = 0; i < length; i++) {
    if (snake->x == segmnet[i].x && snake->y == segmnet[i].y) {
      return true;
    }
  }

  return false;
}

// check the collision with the window boundaries
bool check_border_collision(Snake *snake) {
  if (snake->x < 0 || snake->y < 0 || snake->x + snake->width > WINDOW_WIDTH ||
      snake->y + snake->height > WINDOW_HEIGHT) {
    return true;
  }
  return false;
}

void move_snake(Snake *snake, Segment *segment, int length) {
  // shift each segment to the position of the previous one
  for (int i = length - 1; i > 0; i--) {
    segment[i].x = segment[i - 1].x;
    segment[i].y = segment[i - 1].y;
  }

  if (length > 0) {
    segment[0].x = snake->x;
    segment[0].y = snake->y;
  }

  snake->x += snake->dx;
  snake->y += snake->dy;

  // keep the snake within window bounds
  if (snake->x < 0)
    snake->x = 0;
  if (snake->y < 0)
    snake->y = 0;
  if (snake->x + snake->width > WINDOW_WIDTH)
    snake->x = WINDOW_WIDTH - snake->width;
  if (snake->y + snake->height > WINDOW_HEIGHT)
    snake->y = WINDOW_HEIGHT - snake->height;
}

// reset the game when player loses
void reset_game(Snake *snake, Segment *segment, int *length) {
  snake->x = WINDOW_WIDTH / 2 - SNAKE_WIDTH / 2;
  snake->y = WINDOW_HEIGHT / 2 - SNAKE_HEIGHT / 2;
  snake->dx = SNAKE_SPEED;
  snake->dy = 0;
  *length = 0;
  score = 0;
}

int main() {
  // initialize the SDL
  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    // check if the initialization failed
    printf("Initialization failed for SDL\n");
    return 1;
  }

  // initialize the SDL ttf
  if (TTF_Init() == -1) {
    printf("Failed to initialize SDL_ttf: %s\n", TTF_GetError());
    return 1;
  }

  // load a font
  TTF_Font *font =
      TTF_OpenFont("/usr/share/fonts/TTF/JetBrainsMonoNerdFont-Bold.ttf", 15);
  if (!font) {
    printf("Failed to load the font\n");
    return 1;
  }

  // seed the random number generator
  srand(time(NULL));

  // create the main windows
  SDL_Window *window =
      SDL_CreateWindow(WINDOW_TITLE, SDL_WINDOWPOS_CENTERED,
                       SDL_WINDOWPOS_CENTERED, WINDOW_WIDTH, WINDOW_HEIGHT, 0);

  // check for window creation failure
  if (!window) {
    printf("Failed to create the main window\n");
    SDL_Quit();
    return 1;
  }

  // create the window surface
  SDL_Surface *surface = SDL_GetWindowSurface(window);
  if (!surface) {
    printf("Failed to create the surface.\n");
    return 1;
  }

  // initialize the snake
  Snake snake = {WINDOW_WIDTH / 2 - 10, WINDOW_HEIGHT / 2 + 10, SNAKE_WIDTH,
                 SNAKE_HEIGHT};

  // initialize the food
  Food food = {0, 0, FOOD_WIDTH, FOOD_HEIGHT};
  generate_random_food(&food);

  // body segments of the snake
  Segment segments[MAXIMUM_SNAKE_LENGTH];
  int length = 0;

  // current direction
  Direction current_direction = RIGHT;

  // main game loop
  bool keep_window_open = true;
  while (keep_window_open) {
    SDL_Event e;
    while (SDL_PollEvent(&e) > 0) {
      switch (e.type) {
      case SDL_QUIT:
        keep_window_open = false;
        break;

      case SDL_KEYDOWN:
        switch (e.key.keysym.sym) {
        case SDLK_UP:
          // Only change direction if not currently moving down
          if (current_direction != DOWN) {
            snake.dx = 0;
            snake.dy = -SNAKE_SPEED;
            current_direction = UP;
          }
          break;
        case SDLK_DOWN:
          // Only change direction if not currently moving up
          if (current_direction != UP) {
            snake.dx = 0;
            snake.dy = SNAKE_SPEED;
            current_direction = DOWN;
          }
          break;
        case SDLK_LEFT:
          // Only change direction if not currently moving right
          if (current_direction != RIGHT) {
            snake.dx = -SNAKE_SPEED;
            snake.dy = 0;
            current_direction = LEFT;
          }
          break;
        case SDLK_RIGHT:
          // Only change direction if not currently moving left
          if (current_direction != LEFT) {
            snake.dx = SNAKE_SPEED;
            snake.dy = 0;
            current_direction = RIGHT;
          }
          break;
        }
        break;
      }
    }

    // move the snake
    move_snake(&snake, segments, length);

    // check with the collision with border or itself
    if (check_self_collision(&snake, segments, length) ||
        check_border_collision(&snake) || 
        length == MAXIMUM_SNAKE_LENGTH) {
      reset_game(&snake, segments, &length);
      generate_random_food(&food);
    }

    // check for collision with the food
    if (check_collision(&snake, &food)) {
      // increase the length of the snake
      for (int i = 0; i < GROWTH_SCALE && length < MAXIMUM_SNAKE_LENGTH; i++) {
        segments[length].x = snake.x;
        segments[length].y = snake.y;
        segments[length].width = SNAKE_WIDTH;
        segments[length].height = SNAKE_HEIGHT;
        length++;
        score++;  // incrment the score each time snake eats the food
      }

      // generate new food
      generate_random_food(&food);
    }

    // fill the surface with a black background
    SDL_FillRect(surface, NULL, SDL_MapRGB(surface->format, 0, 0, 0));

    // draw the game elements
    draw_snake(surface, &snake, segments, length);
    draw_food(surface, &food);

    // render the score in the top-left corner
    render_score(surface, font, score);

    // update the window surface
    SDL_UpdateWindowSurface(window);

    // set delay for frame rate
    SDL_Delay(16); // ~60 2frames per second
  }

  // clean up the SDL
  TTF_CloseFont(font);
  TTF_Quit();
  SDL_DestroyWindow(window);
  SDL_Quit();

  return 0;
}
