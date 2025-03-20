#include <SDL3/SDL.h>

#include <cstdlib>
#include <cmath>

#include <iostream>
#include <fstream>

#define _DEBUG

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

static inline v2 add_v2(v2 u, v2 v) {
  return (v2) { u.x + v.x, u.y + v.y};
}

static inline v2i add_v2i(v2i u, v2i v) {
  return (v2i) { u.x + v.x, u.y + v.y };
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

struct render_queue {
  struct sector* arr[MAX_SECTORS];
  usize size;
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


static inline f32 point_wall_side(v2 p, v2 a, v2 b) {
  return -(((p.x - a.x) * (b.y - a.y)) 
          - ((p.y - a.y) * (b.x - a.x)));
} 

static inline bool inside_sector(v2 p, const struct sector *sector) {
  for (usize i = 0; i < sector->nwalls; i++) {
    const struct wall *wall = &sector->walls[i];
    std::clog << "\tpoint wall side @ " << i << "=" << point_wall_side(p, wall->a, wall->b) << "\n" << std::endl;
    if (point_wall_side(p, wall->a, wall->b) <= 0.0f)
      return false;
  }
  return true;
}

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
  //else 
    //std::clog << "Attepted to draw off-screen @ " << v.x << ", " << v.y << std::endl;
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

static inline v2 world_to_camera(v2 p) {
  const v2 u = { p.x - state.camera.pos.x, p.y - state.camera.pos.y }; // Translate
  return rotate_vector(u, state.camera.angle);
  //return (v2) { //Rotate
  //  u.x * sinf(state.camera.angle) - u.y * cosf(state.camera.angle),
  //  u.x * cosf(state.camera.angle) + u.y * sinf(state.camera.angle)
  //}; 
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
  u32 g = (0xFF/3) * ((i+1) % 4);
  u32 r = (0xFF/2) * (i % 3);
  return a << 24 | b << 16 | g << 8 | r; 
}

static inline v2i transform_to_map(v2 v, f32 sf) {
  const v2 centre = { SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2};
  return v2_to_v2i(add_v2(scale_vector(v, sf), centre));
}

static inline void draw_map_line(v2 a, v2 b, f32 sf, u32 colour) {
  draw_line(transform_to_map(a, sf), transform_to_map(b, sf), colour);
}

static inline void draw_map_pixel(v2 a, f32 sf, u32 colour) {
  draw_pixel(transform_to_map(a, sf), colour);
}

void render() {
  //draw_line({100, 100}, {300, 450}, 0xFFFF0000);
  
  std::clog << std::endl << std::endl << std::endl << state.camera.angle << std::endl;

  std::clog << "Pos " << "[" << state.camera.pos.x << ", " << state.camera.pos.y << "]" << " Angle " << state.camera.angle << " Direction " << rotate_vector({1, 1}, state.camera.angle).x << ", " << rotate_vector({1, 1}, state.camera.angle).y << std::endl;   
  
  draw_map_pixel(world_to_camera(state.camera.pos), SCALE_FACTOR, 0xFF00FF00);
  draw_map_line(world_to_camera(state.camera.pos), world_to_camera(add_v2(state.camera.pos, rotate_vector({0, 1}, -state.camera.angle))), SCALE_FACTOR, 0xC000FF00);
 
  struct render_queue queue;
  memset(&queue, 0, sizeof(queue));
  
  queue.arr[0] = &state.sectors.arr[state.camera.sector];
  queue.size = 1;
  
  usize queue_idx = 0;
  
  while (queue_idx < queue.size) {
    std::clog << "Rendering Sector " << queue.arr[queue_idx]->id << " from queue (position: " << queue_idx << ")" << std::endl;
    const struct sector *sector = queue.arr[queue_idx]; 
    queue_idx++;

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
    
      draw_map_line(cam_a, cam_b, SCALE_FACTOR, get_test_colour(i));

      if (wall->viewportal && queue.size < 2) {
        std::clog << "Added Sector " << (wall->viewportal - 1) << " to queue (position: " << queue.size << ")" << std::endl;
        queue.arr[queue.size] = &state.sectors.arr[wall->viewportal - 1];
        queue.size++;
      }
    }
  }
}

void clear_pixels() {
  for (usize i = 0; i < SCREEN_WIDTH * SCREEN_HEIGHT; i++) {
    state.pixels[i] = 0xFF000000;
  }
}

int main(int argc, char* argv[]) {
  #ifndef _DEBUG 
    std::ofstream nullstream;
    std::clog.rdbuf(nullstream.rdbuf());
  #endif

  memset(&state, 0, sizeof(state));

  init_SDL();
  
  state.sectors.n = 2;
  //TEST level
  state.sectors.arr[0] = (struct sector) {state.walls.arr,     4, 1, 0.0f, 4.0f};
  state.sectors.arr[1] = (struct sector) {&state.walls.arr[4], 6, 2, 0.0f, 4.0f};

  state.walls.n = 9;

  state.walls.arr[0] = (struct wall) {{0,0 }, {5, 0}, 0};
  state.walls.arr[1] = (struct wall) {{5,0 }, {5, 7}, 0};
  state.walls.arr[2] = (struct wall) {{5,7 }, {0, 7}, 2};
  state.walls.arr[3] = (struct wall) {{0,7 }, {0, 0}, 0};
  
  state.walls.arr[4] = (struct wall) {{0,7 }, {5,7 }, 1};
  state.walls.arr[5] = (struct wall) {{5,7 }, {5,11}, 0};
  state.walls.arr[6] = (struct wall) {{5,11}, {3,13}, 0};
  state.walls.arr[7] = (struct wall) {{3,13}, {1,13}, 0};
  state.walls.arr[8] = (struct wall) {{1,13}, {0,11}, 0};
  state.walls.arr[9] = (struct wall) {{0,11}, {0, 7}, 0}; 

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
    
    const bool *keystate = SDL_GetKeyboardState(NULL);
    if (keystate[SDL_SCANCODE_LEFT]) 
      state.camera.angle -= 0.01;
    if (keystate[SDL_SCANCODE_RIGHT])
      state.camera.angle += 0.01;
    if (keystate[SDL_SCANCODE_UP])
      state.camera.pos = add_v2(state.camera.pos, rotate_vector({0, 0.01}, -state.camera.angle));
    if (keystate[SDL_SCANCODE_DOWN])
      state.camera.pos = add_v2(state.camera.pos, rotate_vector({0, -0.01}, -state.camera.angle));

    state.camera.angle = normalise_angle(state.camera.angle);
    if (!inside_sector(state.camera.pos, &state.sectors.arr[state.camera.sector])) {
        for (usize i = 0; i < state.sectors.n; i++) {
            if (inside_sector(state.camera.pos, &state.sectors.arr[i]))
                state.camera.sector = i;
      }
    }
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
