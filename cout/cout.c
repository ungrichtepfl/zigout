#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>

/****** GAME CONFIG ******/
#define SAVE_HIGHSCORE 1
#define HIGHSCORE_FILE_NAME "highscore.txt"

#define SCALING 1
#define DEFAULT_WINDOW_WIDTH 1200
#define DEFAULT_WINDOW_HEIGHT 900
#define WINDOW_WIDTH (DEFAULT_WINDOW_WIDTH * SCALING)
#define WINDOW_HEIGHT (DEFAULT_WINDOW_HEIGHT * SCALING)
#define BACKGROUND_COLOR 0x181818FF
#define TEXT_COLOR 0xDCDCDCFF

#define FPS 60
#define FRAME_TARGET_TIME_MS (1000.0 / FPS)
#define DELTA_TIME_SEC (1.0 / FPS)

#define PROJ_SPEED 350
#define PROJ_WIDTH 30
#define PROJ_HEIGHT 30
#define PROJ_COLOR 0xE6E6E6FF

#define BAR_HEIGHT 20
#define BAR_WIDTH 80
#define BAR_START_X (WINDOW_WIDTH / 2.0 - BAR_WIDTH / 2.0)
#define BAR_START_Y (7 * WINDOW_HEIGHT / 8.0)
#define BAR_SPEED                                                              \
  (PROJ_SPEED - 1) // smaller than PROJ_SPEED to prevent Proj sticking to Bar
#define BAR_COLOR 0xFF4040FF

#define TARGET_X_SPACING 10
#define TARGET_Y_SPACING 10
#define TARGET_Y_NUMBER (10 * SCALING)
#define TARGET_X_NUMBER (10 * SCALING)
#define TARGET_WIDTH BAR_WIDTH
#define TARGET_HEIGHT BAR_HEIGHT
#define TARGET_SPACE_HEIGHT                                                    \
  (TARGET_Y_SPACING * (TARGET_Y_NUMBER - 1) + TARGET_HEIGHT * TARGET_Y_NUMBER)
#define TARGET_SPACE_WIDTH                                                     \
  (TARGET_X_SPACING * (TARGET_X_NUMBER - 1) + TARGET_WIDTH * TARGET_X_NUMBER)
#define TARGET_NUMBER (TARGET_Y_NUMBER * TARGET_X_NUMBER)
#define TARGET_Y_PADDING (WINDOW_HEIGHT / 10)
#define TARGET_X_PADDING ((WINDOW_WIDTH - TARGET_SPACE_WIDTH) / 2)
#define TARGET_SCORE 100

#define PARTICLE_NUMBER 1000
#define PARTICLE_TO_EMIT 30
#define PARTICLE_TO_EMIT_VARIABILITY (PARTICLE_TO_EMIT / 4 * 2)
#define PARTICLE_SIZE 10
#define PARTICLE_SIZE_VARIABLILIY (PARTICLE_SIZE - 1)
#define PARTICLE_SPEED 5
#define PARTICLE_SPEED_VARIABILITY (PARTICLE_SPEED - 1)
#define PARTICLE_LIFETIME_SEC 2
#define PARTICLE_LIFETIME_SEC_VARIABILITY 1.5

/****** GENERAL DATA TYPES *********/

typedef enum { false, true } bool;

typedef uint32_t color_t;

/****** MACRO DEFINITIONS **********/

#define FONT_FILEPATH "../Lato-Regular.ttf"
#define TEXT_BUF_SIZE 100

#define SIGN(x) (x >= 0 ? 1 : -1)
#define FCLAMP(x, lower, upper) fmax(lower, fmin(x, upper))

#define SPREAD_COLOR(color)                                                    \
  (color >> 3 * 8) & 0xFF, (color >> 2 * 8) & 0xFF, (color >> 1 * 8) & 0xFF,   \
      (color >> 0 * 8) & 0xFF

#define SET_ALPHA(color, alpha) (~(~color | 0xFF) | alpha)

#define UNSPREAD_COLOR(r, g, b, a)                                             \
  (r << 3 * 8) | (g << 2 * 8) | (b << 1 * 8) | (a << 0 * 8)

static int exit_code = 0;
#define SET_EXIT_CODE(e)                                                       \
  do {                                                                         \
    exit_code = e;                                                             \
  } while (0)

#define EXIT_WITH(e)                                                           \
  do {                                                                         \
    SET_EXIT_CODE(e);                                                          \
    goto quit;                                                                 \
  } while (0)

#define EXIT() EXIT_WITH(1)

/******* GAME MECHANICS ********/

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
  SDL_Texture *const texture = SDL_CreateTextureFromSurface(renderer, surface);
  if (!texture) {
    SDL_Log("SDL_CreateTextureFromSurface: %s\n", SDL_GetError());
    return;
  };

  const SDL_Rect rect = createSdlRect(pos->x, pos->y, surface->w, surface->h);
  SDL_RenderCopy(renderer, texture, NULL, &rect);
  SDL_DestroyTexture(texture);
}

void renderText(SDL_Renderer *const renderer, const char *const text,
                color_t color, const Vector2D *const pos,
                TTF_Font *const font) {
  SDL_Color sdl_color = colorToSdlColor(color);
  SDL_Surface *const surface = TTF_RenderText_Solid(font, text, sdl_color);
  if (!surface) {
    SDL_Log("TTF_RenderText_Solid: %s\n", TTF_GetError());
    return;
  };
  renderSurface(renderer, surface, pos);
  SDL_FreeSurface(surface);
}

void renderXYCenteredText(SDL_Renderer *const renderer, const char *const text,
                          color_t color, TTF_Font *const font) {
  const SDL_Color sdl_color = colorToSdlColor(color);
  SDL_Surface *const surface = TTF_RenderText_Solid(font, text, sdl_color);
  if (!surface) {
    SDL_Log("TTF_RenderText_Solid: %s\n", TTF_GetError());
    return;
  };
  const Vector2D pos = {
      .x = ((float)(uint32_t)WINDOW_WIDTH - surface->w) / 2,
      .y = ((float)(uint32_t)WINDOW_HEIGHT - surface->h) / 2,
  };
  renderSurface(renderer, surface, &pos);
  SDL_FreeSurface(surface);
}

void renderYCenteredText(SDL_Renderer *const renderer, const char *const text,
                         const color_t color, TTF_Font *const font,
                         const uint32_t x_pos) {
  const SDL_Color sdl_color = colorToSdlColor(color);
  SDL_Surface *const surface = TTF_RenderText_Solid(font, text, sdl_color);
  if (!surface) {
    SDL_Log("TTF_RenderText_Solid: %s\n", TTF_GetError());
    return;
  };
  const Vector2D pos = {.x = x_pos,
                        .y = ((float)(uint32_t)WINDOW_HEIGHT - surface->h) / 2};
  renderSurface(renderer, surface, &pos);
  SDL_FreeSurface(surface);
}

void renderXCenteredText(SDL_Renderer *const renderer, const char *const text,
                         const color_t color, TTF_Font *const font,
                         const uint32_t y_pos) {
  const SDL_Color sdl_color = colorToSdlColor(color);
  SDL_Surface *const surface = TTF_RenderText_Solid(font, text, sdl_color);
  if (!surface) {
    SDL_Log("TTF_RenderText_Solid: %s\n", TTF_GetError());
    return;
  };
  const Vector2D pos = {
      .x = ((float)(uint32_t)WINDOW_WIDTH - surface->w) / 2,
      .y = y_pos,
  };
  renderSurface(renderer, surface, &pos);
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

typedef struct Projectile_s {
  Vector2D pos;
  Vector2D vel;
} Projectile;

typedef struct Bar_s {
  Vector2D pos;
  int32_t vel;
} Bar;

Bar initialBar(void) {
  return (Bar){.pos = (Vector2D){.x = (float)(uint32_t)BAR_START_X,
                                 .y = (float)(uint32_t)BAR_START_Y},
               .vel = 0};
}

SDL_Rect createBarRect(const Bar *const bar) {
  return createSdlRect(bar->pos.x, bar->pos.y, BAR_WIDTH, BAR_HEIGHT);
}

void setBarSpeedDir(Bar *const bar, const int32_t direction) {
  bar->vel = direction * BAR_SPEED;
}

void setBarSpeedLeft(Bar *const bar) { setBarSpeedDir(bar, -1); }

void setBarSpeedRight(Bar *const bar) { setBarSpeedDir(bar, 1); }

void updateBar(Bar *const bar) {
  float nx = bar->pos.x + (float)bar->vel * DELTA_TIME_SEC;
  nx = FCLAMP(nx, 0, WINDOW_WIDTH - BAR_WIDTH);
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
  const float f =
      (x <= 0.0031308) ? x * 12.92 : 1.055 * pow(x, 1.0 / 2.4) - 0.055;
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
  float time_alive_sec; // < 0 indicates not active
  float max_time_alive_sec;
} Particle;

void initializeParticle(Particle *const particle) {
  particle->pos = (Vector2D){0, 0};
  particle->color = 0xFF4040FF;
  particle->angle = 0;
  particle->size = PARTICLE_SIZE;
  particle->speed = PARTICLE_SPEED;
  particle->time_alive_sec = -1.0;
  particle->max_time_alive_sec = PARTICLE_LIFETIME_SEC;
}

void initializeParticles(Particle particles[PARTICLE_NUMBER]) {
  for (int i = 0; i < PARTICLE_NUMBER; i++)
    initializeParticle(&particles[i]);
}

SDL_Rect createParticleRect(const Particle *particle) {
  return createSdlRect(particle->pos.x, particle->pos.y, particle->size,
                       particle->size);
}
void updateParticles(Particle particles[PARTICLE_NUMBER]) {
  for (int i = 0; i < PARTICLE_NUMBER; i++) {
    if (particles[i].time_alive_sec >= 0) {
      particles[i].time_alive_sec += DELTA_TIME_SEC;
      if (particles[i].time_alive_sec >= particles[i].max_time_alive_sec) {
        initializeParticle(&particles[i]);
        continue;
      }
      particles[i].pos.x += particles[i].speed * cos(particles[i].angle);
      particles[i].pos.y += particles[i].speed * sin(particles[i].angle);
      const uint8_t alpha = 0xFF * (1 - particles[i].time_alive_sec /
                                            particles[i].max_time_alive_sec);
      const color_t color = SET_ALPHA(particles[i].color, alpha);
      particles[i].color = color;
    }
  }
}

void drawParticles(const Particle particles[PARTICLE_NUMBER],
                   SDL_Renderer *const renderer) {
  for (int i = 0; i < PARTICLE_NUMBER; i++) {
    if (particles[i].time_alive_sec >= 0) {
      const SDL_Rect rect = createParticleRect(&particles[i]);
      SDL_SetRenderDrawColor(renderer, SPREAD_COLOR(particles[i].color));
      SDL_RenderFillRect(renderer, &rect);
    }
  }
}

void emitParticles(Particle particles[PARTICLE_NUMBER],
                   const Target *const target) {
  size_t emitted = 0;
  const size_t to_emit =
      PARTICLE_TO_EMIT +
      (drand48() - 0.5) * (float)(uint32_t)PARTICLE_TO_EMIT_VARIABILITY;
  for (int i = 0; i < PARTICLE_NUMBER; i++) {
    if (particles[i].time_alive_sec < 0) {
      particles[i].time_alive_sec = 0;
      particles[i].color = target->color;
      particles[i].max_time_alive_sec +=
          (drand48() - 0.5) * PARTICLE_LIFETIME_SEC_VARIABILITY;
      particles[i].speed += (drand48() - 0.5) * PARTICLE_SPEED_VARIABILITY;
      particles[i].size += (drand48() - 0.5) * PARTICLE_SIZE_VARIABLILIY;
      particles[i].pos.x =
          target->pos.x + TARGET_WIDTH / 2.0 - particles[i].size / 2.0;
      particles[i].pos.y =
          target->pos.y + TARGET_HEIGHT / 2.0 - particles[i].size / 2.0;
      particles[i].angle = drand48() * 2 * M_PI;
      emitted += 1;
      if (emitted >= to_emit) {
        break;
      }
    }
  }
}

void updateProj(Projectile *const proj, Target targets[TARGET_NUMBER],
                Particle particles[PARTICLE_NUMBER], const Bar *const bar,
                uint64_t *const score) {
  const Vector2D n_speed = vecMult(&proj->vel, DELTA_TIME_SEC);
  const Vector2D n_pos = addVec(&proj->pos, &n_speed);
  const SDL_Rect barRect = createBarRect(bar);
  const SDL_Rect projRect_x =
      createSdlRect(n_pos.x, proj->pos.y, PROJ_WIDTH, PROJ_HEIGHT);
  const SDL_Rect projRect_y =
      createSdlRect(proj->pos.x, n_pos.y, PROJ_WIDTH, PROJ_HEIGHT);

  bool intersects_target_x = false;
  bool intersects_target_y = false;
  for (int i = 0; i < TARGET_NUMBER; i++) {
    if (targets[i].is_alive) {
      const SDL_Rect targetRect = createTargetRect(&targets[i]);
      intersects_target_x = SDL_HasIntersection(&targetRect, &projRect_x) != 0;
      intersects_target_y = SDL_HasIntersection(&targetRect, &projRect_y) != 0;
      if (intersects_target_x || intersects_target_y) {
        targets[i].is_alive = false;
        (*score) += TARGET_SCORE;
        emitParticles(particles, &targets[i]);
        break;
      }
    }
  }

  const bool intersects_bar_x = SDL_HasIntersection(&barRect, &projRect_x) != 0;
  if (n_pos.x < 0 || n_pos.x + PROJ_WIDTH > WINDOW_WIDTH || intersects_bar_x ||
      intersects_target_x) {
    proj->vel.x = -proj->vel.x;
  }
  const bool intersects_bar_y = SDL_HasIntersection(&barRect, &projRect_y) != 0;
  if (n_pos.y < 0 || n_pos.y + PROJ_HEIGHT > WINDOW_HEIGHT ||
      intersects_bar_y || intersects_target_y) {
    proj->vel.y = -proj->vel.y;
  }
  if (intersects_bar_y) {
    if (abs(bar->vel) > 0) {
      proj->vel.x = SIGN(bar->vel) * fabs(proj->vel.x);
    }
  }
  const Vector2D speed_updated = vecMult(&proj->vel, DELTA_TIME_SEC);
  addToVec(&proj->pos, &speed_updated);
}

bool hasLost(const Projectile *const proj) {
  const Vector2D speed = vecMult(&proj->vel, DELTA_TIME_SEC);
  const Vector2D n_pos = addVec(&proj->pos, &speed);
  return n_pos.y + PROJ_WIDTH > WINDOW_HEIGHT;
}

bool hasWon(Target targets[TARGET_NUMBER]) {
  for (int i = 0; i < TARGET_NUMBER; i++) {
    if (targets[i].is_alive) {
      return false;
    }
  }
  return true;
}

Projectile initialProj() {
  return (Projectile){
      .pos =
          (Vector2D){
              .x = (float)(uint32_t)BAR_START_X + BAR_WIDTH / 2.0 -
                   PROJ_WIDTH / 2.0,
              .y = (float)(uint32_t)BAR_START_Y - PROJ_HEIGHT,
          },
      .vel =
          (Vector2D){
              .x = PROJ_SPEED,
              .y = PROJ_SPEED,
          },
  };
}

SDL_Rect createProjRect(const Projectile *const proj) {
  return createSdlRect(proj->pos.x, proj->pos.y, PROJ_WIDTH, PROJ_HEIGHT);
}

void drawProj(const Projectile *const proj, SDL_Renderer *const renderer) {
  SDL_Rect rect = createProjRect(proj);
  SDL_SetRenderDrawColor(renderer, SPREAD_COLOR(PROJ_COLOR));
  SDL_RenderFillRect(renderer, &rect);
}

int readHighscore(uint64_t *const highscore) {
  FILE *file = fopen(HIGHSCORE_FILE_NAME, "r");
  if (!file)
    return -1;

  char text_buf[TEXT_BUF_SIZE];

  char c;
  size_t i = 0;
  while (c = fgetc(file), c != EOF) {
    text_buf[i] = c;
    i++;
  }
  // Other way to read file:
  // while (fgets(text_buf, TEXT_BUF_SIZE, file)) {
  // }

  *highscore = atoi(text_buf);

  return 0;
}

void saveHighscore(const uint64_t highscore) {
  FILE *file = fopen(HIGHSCORE_FILE_NAME, "w");
  if (!file)
    return;
  char text_buf[TEXT_BUF_SIZE];
  sprintf(text_buf, "%lu", highscore);
  fputs(text_buf, file);
}

int runGame(void) {
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

  /******* State of the game *******/
  bool quit = false;
  bool pause = false;
  bool started = false;
  bool reset = false;
  bool won = false;
  bool lost = false;
  uint64_t score = 0;
  uint64_t highscore = 0;
  Bar bar = initialBar();
  Projectile proj = initialProj();
  Target targets[TARGET_NUMBER];
  initializeTargets(targets);
  Particle particles[PARTICLE_NUMBER];
  initializeParticles(particles);
  /*********************************/

#if SAVE_HIGHSCORE
  if (readHighscore(&highscore)) {
    highscore = 0;
  };
#endif

  drawBackground(renderer);
  drawProj(&proj, renderer);
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
      proj = initialProj();
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
      proj.vel.x = a_pressed ? -PROJ_SPEED : PROJ_SPEED;
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
        updateParticles(particles);

        lost = hasLost(&proj); // must be before proj has been update
        updateProj(&proj, targets, particles, &bar, &score);

        won = hasWon(targets);
      } else {
        if (score > highscore) {
          highscore = score;
        }
      }
    }

    drawBackground(renderer);
    drawProj(&proj, renderer);
    drawBar(&bar, renderer);
    drawTargets(targets, renderer);
    drawParticles(particles, renderer);
    writeScore(score, highscore, renderer, score_font);

    if (!started) {
      renderXYCenteredText(renderer,
                           "Press A or D to move the bar and start the "
                           "game.While playing press SPACE to pause.",
                           TEXT_COLOR, game_font);
      renderXCenteredText(renderer, "Press Q anytime to quit.", TEXT_COLOR,
                          game_font, WINDOW_HEIGHT / 2 + 20 * SCALING);
    } else if (pause) {
      renderXYCenteredText(renderer, "Press SPACE to unpause or Q to quit.",
                           TEXT_COLOR, game_font);
    } else if (won) {
      renderXYCenteredText(renderer,
                           "You won! Press R to restart or Q to quit.",
                           TEXT_COLOR, game_font);
    } else if (lost) {
      renderXYCenteredText(renderer,
                           "You lost! Press R to restart or Q to quit.",
                           TEXT_COLOR, game_font);
    }

    SDL_RenderPresent(renderer);
    SDL_Delay(FRAME_TARGET_TIME_MS);
  }

#if SAVE_HIGHSCORE
  saveHighscore(highscore);
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

int main(void) { return runGame(); }
