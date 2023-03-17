#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>

#include "cout.h"

#define FONT_FILEPATH "../Lato-Regular.ttf"
#define TEXT_BUF_SIZE 100

typedef enum { false, true } bool;

typedef uint32_t color_t;

#define SPREAD_COLOR(color)                                                    \
  (color >> 3 * 8) & 0xFF, (color >> 2 * 8) & 0xFF, (color >> 1 * 8) & 0xFF,   \
      (color >> 0 * 8) & 0xFF

#define UNSPREAD_COLOR(r, g, b, a)                                             \
  (r << 3 * 8) | (g << 2 * 8) | (b << 1 * 8) | (a << 0 * 8)

static int exit_code = 0;
#define SET_EXIT_CODE(e)                                                       \
  { exit_code = e; }

#define EXIT_WITH(e)                                                           \
  {                                                                            \
    SET_EXIT_CODE(e);                                                          \
    goto quit;                                                                 \
  }

#define EXIT() EXIT_WITH(1)

typedef struct Vector2D_s {
  float x;
  float y;

} Vector2D;

SDL_Color colorToSdlColor(const color_t color) {
  return (SDL_Color){SPREAD_COLOR(color)};
}

SDL_Rect createSdlRect(const int32_t x, const int32_t y, const int32_t w,
                       const int32_t h) {
  return (SDL_Rect){
      .x = x,
      .y = y,
      .w = w,
      .h = h,
  };
}

void drawBackground(SDL_Renderer *const renderer) {
  if (SDL_SetRenderDrawColor(renderer, SPREAD_COLOR(BACKGROUND_COLOR))) {
    SDL_Log("Could not render background: %s", SDL_GetError());
  }
  if (SDL_RenderClear(renderer)) {
    SDL_Log("Could not render background: %s", SDL_GetError());
  }
}

void renderSurface(SDL_Renderer *const renderer, SDL_Surface *const surface,
                   const Vector2D *pos) {
  SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
  if (!texture) {
    SDL_Log("SDL_CreateTextureFromSurface: %s\n", SDL_GetError());
    return;
  };

  SDL_Rect rect = createSdlRect(pos->x, pos->y, surface->w, surface->h);
  SDL_RenderCopy(renderer, texture, NULL, &rect);
  SDL_DestroyTexture(texture);
}

void renderText(SDL_Renderer *const renderer, const char *const text,
                color_t color, const Vector2D *const pos,
                TTF_Font *const font) {
  SDL_Color sdl_color = colorToSdlColor(color);
  SDL_Surface *surface = TTF_RenderText_Solid(font, text, sdl_color);
  if (!surface) {
    SDL_Log("TTF_RenderText_Solid: %s\n", TTF_GetError());
    return;
  };
  renderSurface(renderer, surface, pos);
  SDL_FreeSurface(surface);
}

void writeScore(const uint64_t score, const uint64_t highscore,
                SDL_Renderer *const renderer, TTF_Font *const score_font) {
  char score_text[TEXT_BUF_SIZE];
  sprintf(score_text, "Score: %lu", score);
  renderText(renderer, score_text, TEXT_COLOR, &(Vector2D){.x = 10, .y = 10},
             score_font);
  char highscoreText[TEXT_BUF_SIZE];
  sprintf(highscoreText, "Best: %lu", highscore);
  renderText(renderer, highscoreText, TEXT_COLOR, &(Vector2D){.x = 10, .y = 30},
             score_font);
}

Vector2D vecMult(const Vector2D *const vec, const float scalar) {
  return (Vector2D){.x = vec->x * scalar, .y = vec->y * scalar};
}

void multToVec(Vector2D *const vec, const float scalar) {
  vec->x = vec->x * scalar;
  vec->y = vec->y * scalar;
}

void addToVec(Vector2D *const a, const Vector2D *const b) {
  a->x += b->x;
  a->y += b->y;
}

Vector2D addVec(const Vector2D *const a, const Vector2D *const b) {
  return (Vector2D){
      .x = a->x + b->x,
      .y = a->y + b->y,
  };
}

typedef struct Bar_s {
  Vector2D pos;
  int32_t vel;
} Bar;

Bar initialBar(void) {
  return (Bar){.pos = (Vector2D){.x = BAR_START_X, .y = BAR_START_Y}, .vel = 0};
}

SDL_Rect createBarRect(const Bar *const bar) {
  return createSdlRect(bar->pos.x, bar->pos.y, BAR_WIDTH, BAR_HEIGHT);
}

void setBarSpeedDir(Bar *const bar, const int32_t direction) {
  bar->vel = direction * BAR_SPEED;
}

void setBarSpeedLeft(Bar *const bar) { setBarSpeedDir(bar, -1); }

void setBarSpeedRight(Bar *const bar) { setBarSpeedDir(bar, 1); }

float clamp(const float x, const float lower, const float upper) {
  return fmax(lower, fmin(x, upper));
}

void updateBar(Bar *const bar) {
  float nx = bar->pos.x + (float)bar->vel * DELTA_TIME_SEC;
  nx = clamp(nx, 0, WINDOW_WIDTH - BAR_WIDTH);
  bar->pos.x = nx;
}

void drawBar(const Bar *const proj, SDL_Renderer *const renderer) {
  SDL_Rect rect = createBarRect(proj);
  SDL_SetRenderDrawColor(renderer, SPREAD_COLOR(BAR_COLOR));
  SDL_RenderFillRect(renderer, &rect);
}

typedef struct Target_s {
  Vector2D pos;
  bool is_alive;
  color_t color;
} Target;

typedef struct LinearColor_s {
  float r;
  float g;
  float b;
  float a;
} LinearColor;

float color_u8_to_f32(const uint8_t x) { return x / 255.0; }

uint8_t color_f32_to_u8(const float x) { return x * 255.0; }

float to_linear(const uint8_t x) {
  const float f = color_u8_to_f32(x);
  if (f <= 0.04045)
    return f / 12.92;
  else
    return pow((f + 0.055) / 1.055, 2.4);
}

LinearColor srgb_to_linear(const uint8_t r, const uint8_t g, const uint8_t b,
                           const uint8_t a) {
  return (LinearColor){
      .r = to_linear(r),
      .g = to_linear(g),
      .b = to_linear(b),
      .a = color_u8_to_f32(a),
  };
}

uint8_t to_srgb(const float x) {
  float f;
  if (x <= 0.0031308)
    f = x * 12.92;
  else
    f = 1.055 * pow(x, 1.0 / 2.4) - 0.055;
  return color_f32_to_u8(f);
}

LinearColor lerp_color(const LinearColor *const color1,
                       const LinearColor *const color2, const float t) {
  const float vec1[] = {color1->r, color1->g, color1->b, color1->a};
  const float vec2[] = {color2->r, color2->g, color2->b, color2->a};
  float res[] = {0, 0, 0, 0};
  for (int i = 0; i < 4; i++)
    res[i] = vec1[i] + (vec2[i] - vec1[i]) * t;
  return (LinearColor){
      .r = res[0],
      .g = res[1],
      .b = res[2],
      .a = res[3],
  };
}

color_t linear_to_srgb(const LinearColor *const color) {
  return UNSPREAD_COLOR(to_srgb(color->r), to_srgb(color->g), to_srgb(color->b),
                        color_f32_to_u8(color->a));
}

color_t lerp_color_gamma_corrected(const color_t color1, const color_t color2,
                                   const float t) {
  const LinearColor c1 = srgb_to_linear(SPREAD_COLOR(color1));
  const LinearColor c2 = srgb_to_linear(SPREAD_COLOR(color2));
  const LinearColor c = lerp_color(&c1, &c2, t);
  return linear_to_srgb(&c);
}

void initializeTargets(Target targets[TARGET_NUMBER]) {
  const int32_t dx = TARGET_SPACE_WIDTH / TARGET_X_NUMBER;
  const int32_t dy = TARGET_SPACE_HEIGHT / TARGET_Y_NUMBER;
  // Shift the targets to the right so that they are centered:
  const int32_t align_dx = (dx - TARGET_WIDTH) / (TARGET_X_NUMBER - 1);
  const int32_t align_dy = (dy - TARGET_HEIGHT) / (TARGET_Y_NUMBER - 1);

  const color_t red = 0xFF2E2EFF;
  const color_t green = 0x2EFF2EFF;
  const color_t blue = 0x2E2EFFFF;
  const float level = 0.5;

  for (uint32_t idx = 0; idx < TARGET_NUMBER; idx++) {
    const uint32_t idx_x = idx % TARGET_X_NUMBER;
    const uint32_t idx_y = idx / TARGET_X_NUMBER;
    const uint32_t pos_x = TARGET_X_PADDING + (dx + align_dx) * idx_x;
    const uint32_t pos_y = TARGET_Y_PADDING + (dy + align_dy) * idx_y;

    const float t = (float)idx_y / TARGET_Y_NUMBER;
    color_t target_color;
    if (t < level)
      target_color = lerp_color_gamma_corrected(red, green, t / level);
    else
      target_color =
          lerp_color_gamma_corrected(green, blue, (t - level) / (1 - level));
    targets[idx].pos = (Vector2D){
        .x = pos_x,
        .y = pos_y,
    };
    targets[idx].is_alive = true;
    targets[idx].color = target_color;
  }
}

SDL_Rect createTargetRect(const Target *const target) {
  return createSdlRect(target->pos.x, target->pos.y, TARGET_WIDTH,
                       TARGET_HEIGHT);
}

void drawTargets(const Target targets[TARGET_NUMBER],
                 SDL_Renderer *const renderer) {
  for (int i = 0; i < TARGET_NUMBER; i++) {
    if (targets[i].is_alive) {
      const SDL_Rect rect = createTargetRect(&targets[i]);
      SDL_SetRenderDrawColor(renderer, SPREAD_COLOR(targets[i].color));
      SDL_RenderFillRect(renderer, &rect);
    }
  }
}

typedef struct Particle_s {
  Vector2D pos;
  color_t color;
  float angle; // between [0,2*pi)
  int32_t size;
  int32_t speed;
  int32_t time_alive_sec; // < 0 indicates not active
  int32_t max_time_alive_sec;
} Particle;

void initializeParticles(Particle particles[PARTICLE_NUMBER]) {
  for (int i = 0; i < PARTICLE_NUMBER; i++) {
    particles[i].pos = (Vector2D){0, 0};
    particles[i].color = 0xFF4040FF;
    particles[i].angle = 0;
    particles[i].size = PARTICLE_SIZE;
    particles[i].speed = PARTICLE_SPEED;
    particles[i].time_alive_sec = -1.0;
    particles[i].max_time_alive_sec = PARTICLE_LIFETIME_SEC;
  }
}

SDL_Rect createParticleRect(const Particle *particle) {
  return createSdlRect(particle->pos.x, particle->pos.y, particle->size,
                       particle->size);
}

int COUT_StartGame(void) {
  SDL_Window *window = NULL;
  SDL_Renderer *renderer = NULL;
  TTF_Font *game_font = NULL;
  TTF_Font *score_font = NULL;

  if (SDL_Init(SDL_INIT_VIDEO)) {
    SDL_Log("Unable to initialize SDL: %s", SDL_GetError());
    EXIT();
  }

  if (TTF_Init()) {
    SDL_Log("Unable to initialize SDL_ttf: %s", TTF_GetError());
    EXIT();
  }

  window = SDL_CreateWindow("Cout", 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, 0);
  if (!window) {
    SDL_Log("Unable to create window: %s", SDL_GetError());
    EXIT();
  }

  renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
  if (!renderer) {
    SDL_Log("Unable to create renderer: %s", SDL_GetError());
    EXIT();
  }

  // enable transparent mode
  if (SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND)) {
    SDL_Log("Unable to set blend/transparent mode: %s", SDL_GetError());
    EXIT();
  }

  const Uint8 *keyboard_state = SDL_GetKeyboardState(NULL);

  game_font = TTF_OpenFont(FONT_FILEPATH, 28);
  if (!game_font) {
    SDL_Log("Unable to load font: %s", TTF_GetError());
    EXIT();
  }

  score_font = TTF_OpenFont(FONT_FILEPATH, 20);
  if (!score_font) {
    SDL_Log("Unable to load font: %s", TTF_GetError());
    EXIT();
  }

  // ---- State of the game ---- //
  bool quit = false;
  bool pause = false;
  bool started = false;
  bool reset = false;
  bool won = false;
  bool lost = false;
  uint64_t score = 0;
  uint64_t highscore = 0;
  Bar bar = initialBar();
  // var proj = initialProj();
  Target targets[TARGET_NUMBER];
  initializeTargets(targets);
  Particle particles[PARTICLE_NUMBER];
  initializeParticles(particles);
  // --------------------------- //

#if SAVE_HIGHSCORE
  // highscore = readHighscore();
#endif

  drawBackground(renderer);
  // drawProj(&proj, renderer);
  drawBar(&bar, renderer);
  drawTargets(targets, renderer);

  while (!quit) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {

      switch (event.type) {
      case SDL_QUIT: {
        quit = true;
        break;
      }
      case SDL_KEYDOWN: {
        switch (event.key.keysym.sym) {
        case 'q': {
          quit = true;
          break;
        }
        case ' ': {
          pause = !pause;
          break;
        }
        case 'r': {
          reset = true;
          break;
        }
        default:
          break;
        }
      }
      default:
        break;
      }
    }

    if (reset) {
      bar = initialBar();
      // proj = initialProj();
      initializeTargets(targets);
      initializeParticles(particles);
      started = false;
      reset = false;
      pause = false;
      won = false;
      lost = false;
      score = 0;
    }

    bool a_pressed = keyboard_state[SDL_SCANCODE_A] != 0;
    bool d_pressed = keyboard_state[SDL_SCANCODE_D] != 0;

    if (!started && (a_pressed || d_pressed)) {
      started = true;
      // proj.vel.x = if (a_pressed) - PROJ_SPEED else PROJ_SPEED;
    }

    if (!pause && started) {
      if (!won && !lost) {
        if (a_pressed && !d_pressed) {
          setBarSpeedLeft(&bar);
        } else if (d_pressed && !a_pressed) {
          setBarSpeedRight(&bar);
        } else {
          bar.vel = 0;
        }
        updateBar(&bar);
        // updateParticles(&particles);
        //
        // lost = hasLost(&proj); // must be before proj has been
        // updated updateProj(&proj, &targets, &particles, &bar, &score);
        //
        // won = hasWon(&targets);
      } else {
        if (score > highscore) {
          // highscore = score;
        }
      }
    }

    drawBackground(renderer);
    // drawProj(&proj, renderer);
    drawBar(&bar, renderer);
    drawTargets(targets, renderer);
    // drawParticles(&particles, renderer);
    writeScore(score, highscore, renderer, score_font);

    if (!started) {
      //   renderXYCenteredText(renderer,
      //                        "Press A or D to move the bar and start the "
      //                        "game.While playing press SPACE to pause.",
      //                        &TEXT_COLOR, game_font);
      //   renderXCenteredText(renderer, "Press Q anytime to quit.",
      //   &TEXT_COLOR,
      //                       game_font, WINDOW_HEIGHT / 2 + 20 * SCALING);
      // } else if (pause) {
      //   renderXYCenteredText(renderer, "Press SPACE to unpause or Q to
      //   quit.",
      //                        &TEXT_COLOR, game_font);
      // } else if (won) {
      //   renderXYCenteredText(renderer,
      //                        "You won! Press R to restart or Q to quit.",
      //                        &TEXT_COLOR, game_font);
      // } else if (lost) {
      //   renderXYCenteredText(renderer,
      //                        "You lost! Press R to restart or Q to quit.",
      //                        &TEXT_COLOR, game_font);
    }

    SDL_RenderPresent(renderer);
    SDL_Delay(FRAME_TARGET_TIME_MS);
  }

#if SAVE_HIGHSCORE
  // saveHighscore(highscore);
#endif

quit:
  TTF_CloseFont(game_font);
  TTF_CloseFont(score_font);
  TTF_Quit();
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();
  return exit_code;
}
