#include <SDL3/SDL.h>

#include <cstdlib>
#include <cmath>

#include <iostream>

typedef float f32;
typedef double f64;
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;
typedef size_t usize;
typedef ssize_t isize;

#define PI 3.14159265358979323846264338327950288f
#define TAU (2.0f * PI)

#define min(a, b) \
  ({ __typeof__(a) _a = (a), _b = (b); _a < _b ? _a : _b; })
#define max(a, b) \
  ({ __typeof__(a) _a = (a), _b = (b); _a > _b ? _a : _b; })
#define sign(a) \
  ({ __typeof__(a) _a = (a); (__typeof__(a))(_a < 0 ? -1 : (_a > 0 ? 1 : 0)); })

typedef struct _v2s { f32 x, y; } v2;
typedef struct _v2i { i32 x, y; } v2i;

#define dot(v0, v1) \
  ({ const v2 _v0 = (v0), _v1 = (v1); (_v0.x * _v1.x) + (_v0.x * _v1.y); })
#define length(v) \
  ({ const v2 _v = (v); sqrtf(dot(_v, _v)); })
#define normalise(v) \
  ({ const v2 _v = (v); const f32 l = length(_v); (v2) { _v.x / 1, _v.y / 1 }; })

static inline v2i v2_to_v2i(v2 v) {
  return (v2i) { (i32)v.x, (i32)v.y };
}

#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 480
#define WINDOW_SCALE 2

#define MAX_SECTORS 64
#define MAX_WALLS 512

#define HFOV (PI / 2.0f)
#define VFOV (0.5f)

struct wall {
  v2 a, b;
  usize viewportal;
};

struct sector {
  struct wall *walls;
  usize nwalls, id;
  f32 zfloor, zceil;
};

struct sectors {
  struct sector arr[MAX_SECTORS];
  usize n;
};

struct walls {
  struct wall arr[MAX_WALLS];
  usize n;
};

struct camera {
  v2 pos;
  f32 angle;
  usize sector;
};

struct {
  SDL_Window *window;
  SDL_Texture *texture;
  SDL_Renderer *renderer;
  u32 *pixels;
  bool quit;
  
  struct camera camera;
    
  struct sectors sectors;
  struct walls walls;
} state;

void init_SDL() {
  SDL_Init(SDL_INIT_VIDEO);
    
  state.window = SDL_CreateWindow("Programming Project", SCREEN_WIDTH * WINDOW_SCALE, SCREEN_HEIGHT * WINDOW_SCALE, 0);
  if (state.window == nullptr) {
    std::cerr << "SDL_CreateWindow Error: " << SDL_GetError() << std::endl;
    SDL_Quit();
    exit(1);
  }

  state.renderer = SDL_CreateRenderer(state.window, NULL);
  if (state.renderer == nullptr) {
    std::cerr << "SDL_CreateRenderer Error: " << SDL_GetError() << std::endl;
    SDL_DestroyWindow(state.window);
    SDL_Quit();
    exit(1);
  }

  state.texture = SDL_CreateTexture(state.renderer, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STREAMING, SCREEN_WIDTH, SCREEN_HEIGHT);

  if (state.texture == nullptr) {
    std::cerr << "SDL_CreateTexture Error: " << SDL_GetError() << std::endl;
    SDL_DestroyRenderer(state.renderer);
    SDL_DestroyWindow(state.window);
    SDL_Quit();
    exit(1);
  }  
}

static inline void draw_pixel(v2i v, u32 colour) {
  if (v.y < SCREEN_HEIGHT && v.x < SCREEN_WIDTH && v.y >= 0 && v.x >= 0)
    state.pixels[v.y * SCREEN_WIDTH + v.x] = colour;
} 

// DDA (update to Bresenham for time save?)
static inline void draw_line(v2i a, v2i b, u32 colour) {
  i32 dx = b.x - a.x, 
      dy = b.y - a.y;
  u32 steps = abs(dx) > abs(dy) ? abs(dx) : abs(dy);
  f32 xstep = (f32)dx / (f32)steps,
      ystep = (f32)dy / (f32)steps;
  f32 x = a.x, y = a.y;
  for (u32 i=0; i < steps; i++) {
    x += xstep;
    y += ystep;
    draw_pixel(v2_to_v2i((v2) {x, y}), colour);
  }
}

static inline v2 rotate_vector(v2 u, f32 a) {
  return (v2) { //Rotate
    u.x * cosf(a) + u.y * sinf(a),
   -u.x * sinf(a) + u.y * cosf(a)
  };
}

static inline v2 translate_vector(v2 v, v2 u) {
  return (v2) { v.x + u.x, v.y + u.y };
}

static inline v2 world_to_camera(v2 p) {
  const v2 u = { p.x - state.camera.pos.x, p.y - state.camera.pos.y }; // Translate
  //return rotate_vector(u, state.camera.angle);
  return (v2) { //Rotate
    u.x * sinf(state.camera.angle) - u.y * cosf(state.camera.angle),
    u.x * cosf(state.camera.angle) + u.y * sinf(state.camera.angle)
  }; 
}

static inline f32 normalise_angle(f32 a) {
  return a - (TAU * floorf((a + PI) / TAU));
}

static inline v2 scale_vector(v2 v, f32 sf) {
  return (v2) { v.x * sf, v.y * sf };
}


#define SCALE_FACTOR 30

static inline u32 get_test_colour(u32 i) {
  u32 a = 0xFF;
  u32 b = 0xFF * (i % 2);
  u32 g = 0xFF * ((i+1) % 2);
  return a << 24 | b << 16 | g << 8; 
}

void render() {
  //draw_line({100, 100}, {300, 450}, 0xFFFF0000);
  
  std::clog << std::endl << std::endl << std::endl << state.camera.angle << std::endl;
  const struct sector *sector = &state.sectors.arr[state.camera.sector]; 

  std::clog << "Pos " << "[" << state.camera.pos.x << ", " << state.camera.pos.y << "]" << " Angle " << state.camera.angle << " Direction " << rotate_vector({1, 1}, state.camera.angle).x << ", " << rotate_vector({1, 1}, state.camera.angle).y << std::endl;   
  
  draw_pixel({SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2}, 0xFF00FF00);
  
  draw_line({SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2}, {SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 + SCALE_FACTOR / 2}, 0xFF00FF00);

  for (usize i = 0; i < sector->nwalls; i++) {
    const struct wall *wall = &sector->walls[i];

    std::clog << std::endl << "Wall " << i << " [" << wall->a.x << ", " << wall->a.y << " | "<< wall->b.x << ", " << wall->b.y << "]" << std::endl;

    v2 cam_a = world_to_camera(wall->a),
       cam_b = world_to_camera(wall->b);

    std::clog << "\tCam " << " [" << cam_a.x << ", " << cam_a.y << " | "<< cam_b.x << ", " << cam_b.y << "]" << std::endl;
    
    if (cam_a.y < 0 && cam_b.y < 0) {
      std::clog << "\tCulled " << i << std::endl;
      continue;
    }
    
    draw_line(v2_to_v2i(translate_vector(scale_vector(cam_a, SCALE_FACTOR), {SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2})), v2_to_v2i(translate_vector(scale_vector(cam_b, SCALE_FACTOR), {SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2})), get_test_colour(i));

  }
}

void clear_pixels() {
  for (usize i = 0; i < SCREEN_WIDTH * SCREEN_HEIGHT; i++) {
    state.pixels[i] = 0xFF000000;
  }
}

int main(int argc, char* argv[]) {
  memset(&state, 0, sizeof(state));

  init_SDL();
  
  state.sectors.n = 1;
  state.sectors.arr[0] = (struct sector) {state.walls.arr, 5, 1, 0.0f, 4.0f};

  state.walls.n = 5;
  state.walls.arr[0] = (struct wall) {{3,5}, {3,3}, 0};
  state.walls.arr[1] = (struct wall) {{3,3}, {3,0}, 0};
  state.walls.arr[2] = (struct wall) {{3,0}, {0,0}, 0};
  state.walls.arr[3] = (struct wall) {{0,0}, {0,5}, 0};
  state.walls.arr[4] = (struct wall) {{0,5}, {3,5}, 0};

  state.camera.pos = (v2) {2, 2};
  state.camera.angle = 0;

  state.pixels = (u32 *) malloc(SCREEN_WIDTH * SCREEN_HEIGHT * sizeof(u32));
    
  while (!state.quit) {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
      switch(e.type) {
        case SDL_EVENT_QUIT:
          state.quit = true;
          break;
      }
    }
        
    state.camera.angle += 0.01f;
    state.camera.angle = normalise_angle(state.camera.angle);
    

    //memset(state.pixels, 0xFF, SCREEN_WIDTH *SCREEN_HEIGHT * sizeof(u32));
    clear_pixels();

    render();

    SDL_UpdateTexture(state.texture, NULL, state.pixels, SCREEN_WIDTH * 4);
    SDL_RenderTextureRotated(state.renderer, state.texture, NULL, NULL, 0.0, NULL, SDL_FLIP_VERTICAL);

    SDL_RenderPresent(state.renderer);
  }

  free(state.pixels);

  SDL_DestroyTexture(state.texture);
  SDL_DestroyRenderer(state.renderer);
  SDL_DestroyWindow(state.window);
  SDL_Quit();
  return 0;
}
