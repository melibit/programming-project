#include <SDL3/SDL.h>

#include <cstdlib>
#include <cmath>
#include <ctime>

#include <iostream>
#include <fstream>

//#define _DEBUG

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

#define SCREEN_WIDTH 1920
#define SCREEN_HEIGHT 1080
#define WINDOW_SCALE 2

#define MAX_SECTORS 64
#define MAX_WALLS 512

#define HFOV (PI / 4.0f)
#define VFOV (0.5f)
#define CAMZ 0.5f

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
  f32 z;
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
  
  u32 frame;
} state;


static i32 load_level(char *path) {
  std::clog << "Loading Level: " << path << "\n";
  FILE *f = fopen(path, "r");
  if (!f)
    return -1;

  char line[512];

  fgets(line, sizeof(line), f); //level name (ignore for now)
    
  std::clog << "\tLevel Name: " << line << "\n";

  //Sector definitions
  fgets(line, sizeof(line), f); //number of sectors
  sscanf(line, "%u", &state.sectors.n);
  std::clog << "\tLoading " << state.sectors.n << " Sectors" << "\n";
  for (usize i = 0; i < state.sectors.n; i++) {
    fgets(line, sizeof(line), f);
      
    std::clog <<"\t\t Loading " << line << "\n";

    usize wall_start = 0;
    sscanf(line, "%u %u %u %f %f", &wall_start, &state.sectors.arr[i].nwalls, &state.sectors.arr[i].id, &state.sectors.arr[i].zfloor, &state.sectors.arr[i].zceil);
      
    std::clog << "\t\t\tWALL_START " << wall_start << ", NWALLS " << state.sectors.arr[i].nwalls << ", ID " << state.sectors.arr[i].id << "\n";

    state.sectors.arr[i].walls = &state.walls.arr[wall_start];
  }

  //Wall definitions
  fgets(line, sizeof(line), f);
  sscanf(line, "%u", &state.walls.n);
  for (usize i = 0; i < state.walls.n; i++) {
    fgets(line, sizeof(line), f);

    sscanf(line, "%f %f %f %f %u", &state.walls.arr[i].a.x, &state.walls.arr[i].a.y, &state.walls.arr[i].b.x, &state.walls.arr[i].b.y, &state.walls.arr[i].viewportal);
  }

  fgets(line, sizeof(line), f);
  sscanf(line, "%f %f %f", &state.camera.pos.x, &state.camera.pos.y, &state.camera.angle);

  fclose(f);
  return 1;
}

static inline f32 point_wall_side(v2 p, v2 a, v2 b) {
  return -(((p.x - a.x) * (b.y - a.y)) - ((p.y - a.y) * (b.x - a.x)));
} 

static inline bool inside_sector(v2 p, const struct sector *sector) {
  for (usize i = 0; i < sector->nwalls; i++) {
    const struct wall *wall = &sector->walls[i];
    if (point_wall_side(p, wall->a, wall->b) <= 0.0f)
      return false;
  }
  return true;
}

static void init_SDL() {
  SDL_Init(SDL_INIT_VIDEO);
      
  state.window = SDL_CreateWindow("Programming Project", SCREEN_WIDTH * WINDOW_SCALE, SCREEN_HEIGHT * WINDOW_SCALE, 0);
  if (state.window == nullptr) {
    std::cerr << "SDL_CreateWindow Error: " << SDL_GetError() << "\n";
    SDL_Quit();
    exit(1);
  }

  state.renderer = SDL_CreateRenderer(state.window, NULL);
  if (state.renderer == nullptr) {
    std::cerr << "SDL_CreateRenderer Error: " << SDL_GetError() << "\n";
    SDL_DestroyWindow(state.window);
    SDL_Quit();
    exit(1);
  }

  state.texture = SDL_CreateTexture(state.renderer, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STREAMING, SCREEN_WIDTH, SCREEN_HEIGHT);

  if (state.texture == nullptr) {
    std::cerr << "SDL_CreateTexture Error: " << SDL_GetError() << "\n";
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
    //std::clog << "Attepted to draw off-screen @ " << v.x << ", " << v.y << "\n";
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
}

static inline u32 angle_to_screen(f32 a) {
  return (u32) ((1-((a/HFOV) + 0.5f)) * (f32)SCREEN_WIDTH);
}

static inline f32 screen_to_angle(u32 x) {
  return (-(((f32)x / (f32)SCREEN_WIDTH) - 1.0f) - 0.5f) * HFOV;
}

static inline f32 normalise_angle(f32 a) {
  return a - (TAU * floorf((a + PI) / TAU));
}

static inline v2 scale_vector(v2 v, f32 sf) {
  return (v2) { v.x * sf, v.y * sf };
}


#define SCALE_FACTOR 30

// https://stackoverflow.com/questions/664014/what-integer-hash-function-are-good-that-accepts-an-integer-hash-key
u32 hash(u32 x) {
    x = ((x >> 16) ^ x) * 0x45D9F3B;
    x = ((x >> 16) ^ x) * 0x45D9F3B;
    x = (x >> 16) ^ x;
    return x;
}
static inline u32 get_map_colour(u32 w, u32 s) {
  u32 a = 0xFF;
  u32 x = 0xCE * hash(w+1 + hash(s));
  return a << 24 | x; 
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

// https://en.wikipedia.org/wiki/Line%E2%80%93line_intersection
/*
static inline v2 intersect_lines(v2 a0, v2 a1, v2 b0, v2 b1) {
  const f32 denom = (a0.x - a1.x) * (b0.y - b1.y) - (a0.y - a1.y) * (b0.x - b1.x);
  std::cout << "Intersecting: [" << a0.x << ", " << a0.y << "] -> [" << a1.x << ", " << a1.y << "] with {" << b0.x << ", " << b0.y << "} -> {" << b1.x << ", " << b1.y << "]" << "\n";
  if (fabsf(denom) < 1e-6)
    return (v2) {NAN, NAN}; // avoid division by 0; lines are parallel 

  v2 p;
  p.x = ((a0.x*a1.y - a0.y*a1.x) * (b0.x - b1.x) - (a0.y - a1.y) * (b0.x*b1.y - b0.y*b1.x))/denom;
  p.y = ((a0.x*a1.y - a0.y*a1.x) * (b0.y - b1.y) - (a0.y - a1.y) * (b0.x*b1.y - b0.y*b1.x))/denom;
  std::cout << "\t" << "[" << p.x << ", " << p.y << "]" << "\n";
  return p;
}*/ 
static inline v2 intersect_line_segments(v2 p1, v2 p2, v2 p3, v2 p4) {
  const f32 t = ( (p1.x - p3.x) * (p3.y - p4.y) - (p1.y - p3.y) * (p3.x - p4.x) ) / 
                ( (p1.x - p2.x) * (p3.y - p4.y) - (p1.y - p2.y) * (p3.x - p4.x) ); 
  const f32 u = ( (p1.x - p2.x) * (p1.y - p3.y) - (p1.y - p2.y) * (p1.x - p3.x) ) / 
                ( (p1.x - p2.x) * (p3.y - p4.y) - (p1.y - p2.y) * (p3.x - p4.x));
  if (t >= 0 && t <= 1)
    return {p1.x + t*(p2.x - p1.x), p1.y + t*(p2.y - p1.y)};
  if (u >= 0 && u <= 1)
    return {p3.x + u*(p4.x - p4.x), p3.y + u*(p4.y - p3.y)};
  return {NAN, NAN};
}

static void render_map() {
  struct render_queue queue; 
  memset(&queue, 0, sizeof(queue));
  
  draw_map_pixel(world_to_camera(state.camera.pos), SCALE_FACTOR, 0xFF00FF00); 
  
  for (usize sec = 0; sec < state.sectors.n; sec++) {
    const struct sector *sector = &state.sectors.arr[sec];
    for (usize i = 0; i < sector->nwalls; i++) {
      const struct wall *wall = &sector->walls[i];

      v2 cam_a = world_to_camera(wall->a),
         cam_b = world_to_camera(wall->b);
      if (!wall->viewportal)
        draw_map_line(cam_a, cam_b, SCALE_FACTOR, get_map_colour(i, sector->id));
    }
  }
}

static inline u32 darken_colour(u32 colour, f32 factor) {
  const u32 a =     ((colour & 0xFF000000) >> 24);
  const u32 b = min(((colour & 0x00FF0000) >> 16) / factor, (colour & 0x00FF0000) >> 16); 
  const u32 g = min(((colour & 0x0000FF00) >>  8) / factor, (colour & 0x0000FF00) >>  8);
  const u32 r = min(((colour & 0x000000FF)      ) / factor, (colour & 0x000000FF)      );
  return a << 24 | b << 16 | g << 8 | r;
}

static void render() {
  state.frame++;
  std::clog << "\033[H\033[J";
  std::clog << state.camera.angle << "\n";

  std::clog << "Pos " << "[" << state.camera.pos.x << ", " << state.camera.pos.y << "]" << " Angle " << state.camera.angle << " Direction " << rotate_vector({1, 1}, state.camera.angle).x << ", " << rotate_vector({1, 1}, state.camera.angle).y << "\n";   
    
  struct render_queue queue;
  memset(&queue, 0, sizeof(queue));
  
  queue.arr[0] = &state.sectors.arr[state.camera.sector];
  queue.size = 1;
    
  usize queue_idx = 0;
    
  bool rendered_sectors[MAX_SECTORS];
  memset(rendered_sectors, 0, sizeof(rendered_sectors));
  
  f32 rendered_verline_dists[SCREEN_WIDTH];
  i32 rendered_verline_floory[SCREEN_WIDTH];
  i32 rendered_verline_ceily[SCREEN_WIDTH];
  memset(rendered_verline_dists , 0, sizeof(rendered_verline_dists));
  memset(rendered_verline_floory, 0, sizeof(rendered_verline_dists));
  std::fill(rendered_verline_ceily, rendered_verline_ceily + SCREEN_WIDTH, SCREEN_HEIGHT - 1);

  while (queue_idx < queue.size) {
    std::clog << "Rendering Sector " << queue.arr[queue_idx]->id << " from queue (position: " << queue_idx << ")" << "\n";
    const struct sector *sector = queue.arr[queue_idx]; 
    queue_idx++;
      
    rendered_sectors[sector->id - 1] = true;

    for (usize i = 0; i < sector->nwalls; i++) {
      const struct wall *wall = &sector->walls[i];

      v2 cam_a = world_to_camera(wall->a),
         cam_b = world_to_camera(wall->b);
      
      if (cam_a.y < 0 && cam_b.y < 0) {
        continue;
      }

      f32 angle_a = normalise_angle(atan2f(cam_a.y, cam_a.x) - PI / 2.0f),
          angle_b = normalise_angle(atan2f(cam_b.y, cam_b.x) - PI / 2.0f);
        
      if ((angle_a > HFOV/2.0f && angle_b > HFOV/2.0f) || (angle_a < -HFOV/2.0f && angle_b < -HFOV/2.0f)) {
        continue;
      }
     
      std::clog << "\n" << "Wall " << i << " [" << wall->a.x << ", " << wall->a.y << " | "<< wall->b.x << ", " << wall->b.y << "]" << std::endl;
      std::clog << "\tCam " << " [" << cam_a.x << ", " << cam_a.y << " | "<< cam_b.x << ", " << cam_b.y << "]" << "\n";
      std::clog << "\t\t ANGLE: " << angle_a << " | " << angle_b << "\n";
      std::clog << "\t\t\t HFOV " << HFOV << " | " << HFOV/2.0f << std::endl;
      
      //CLIP
      v2 clip_a = cam_a,
         clip_b = cam_b;
      

      if (angle_a >  HFOV/2.0f) { 
        clip_a = intersect_line_segments(cam_a, cam_b, world_to_camera(state.camera.pos), rotate_vector({0, 1024}, -HFOV/2.0f));
        angle_a = normalise_angle(atan2f(clip_a.y, clip_a.x) - PI / 2.0f);
      }
      if (angle_a < -HFOV/2.0f) { 
        clip_a = intersect_line_segments(cam_a, cam_b, world_to_camera(state.camera.pos), rotate_vector({0, 1024},  HFOV/2.0f));
        angle_a = normalise_angle(atan2f(clip_a.y, clip_a.x) - PI / 2.0f);
      }
      if (angle_b >  HFOV/2.0f) {
        clip_b = intersect_line_segments(cam_a, cam_b, world_to_camera(state.camera.pos), rotate_vector({0, 1024}, -HFOV/2.0f));
        angle_b = normalise_angle(atan2f(clip_b.y, clip_b.x) - PI / 2.0f);
      }
      if (angle_b < -HFOV/2.0f) { 
        clip_b = intersect_line_segments(cam_a, cam_b, world_to_camera(state.camera.pos), rotate_vector({0, 1024},  HFOV/2.0f));
        angle_b = normalise_angle(atan2f(clip_b.y, clip_b.x) - PI / 2.0f);
      }

      if (clip_a.y < 0 && clip_b.y < 0) 
        continue;

      if (std::isnan(clip_a.x) || std::isnan(clip_b.x))
        continue;
      
      if (wall->viewportal) {
        if (queue.size < MAX_SECTORS && !(rendered_sectors[wall->viewportal - 1])) {
          std::clog << "Added Sector " << (wall->viewportal - 1) << " to queue (position: " << queue.size << ")" << "\n";
          queue.arr[queue.size] = &state.sectors.arr[wall->viewportal - 1];
          queue.size++;
        }
      }
      
      for (u32 x = angle_to_screen(max(angle_a, angle_b)) + 1; (x <= angle_to_screen(min(angle_a, angle_b))) && (x < SCREEN_WIDTH); x++) {
        f32 a = screen_to_angle(x);
        
        const v2 isect = intersect_line_segments(clip_a, clip_b, world_to_camera(state.camera.pos), rotate_vector({0, 1024}, -a));
        
        const f32 dist = isect.y; //sqrtf(isect.x * isect.x + isect.y + isect.y);
        
        if (rendered_verline_dists[x] && (dist - rendered_verline_dists[x]) >= 1e-3) 
          continue;
        //std::cout << isect.x << ", " << isect.y << " : "<< dist << "\n";
        const f32 h = SCREEN_HEIGHT / dist;
        const i32 y0 = max((SCREEN_HEIGHT / 2) + (SCREEN_HEIGHT / (dist / (sector->zfloor - state.camera.z))) - (h / 2), rendered_verline_floory[x]),
                  y1 = min((SCREEN_HEIGHT / 2) + (SCREEN_HEIGHT / (dist / (sector->zceil  - state.camera.z))) + (h / 2), rendered_verline_ceily[x]);
        
        rendered_verline_floory[x] = y0;
        rendered_verline_ceily[x] = y1;
        
        if (!wall->viewportal) {
          draw_line({(i32)x, y0}, {(i32)x, y1}, darken_colour(get_map_colour(i, sector->id), (dist*dist)/32));
          draw_line({(i32)x, 0 }, {(i32)x, y0}, 0xFF0F0F0F); 
          rendered_verline_dists[x] = dist;
        }
      }
    }
  }
}

static void clear_pixels() {
  for (usize i = 0; i < SCREEN_WIDTH * SCREEN_HEIGHT; i++) {
    state.pixels[i] = 0xFF000000;
  }
}

i32 main(int argc, char* argv[]) {
  
  #ifndef _DEBUG 
    std::ofstream nullstream;
    std::clog.rdbuf(nullstream.rdbuf());
  #endif
  
  memset(&state, 0, sizeof(state));
  
  if (argc > 1)
    load_level(argv[1]);
  else {
    std::cout << "Improper Usage\n\t" << argv[0] << " [Path/To/Level.dat]" << std::endl;
    return -1;
  }

  init_SDL();
    
  state.pixels = (u32 *) malloc(SCREEN_WIDTH * SCREEN_HEIGHT * sizeof(u32));
  
  state.camera.z = CAMZ;
  
  clock_t tstart, tend; 
  tstart = clock();

  while (!state.quit) {
    tstart = tend;
    tend = clock();
    f32 dt = (f32)(tend - tstart) / CLOCKS_PER_SEC;
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
      switch(e.type) {
        case SDL_EVENT_QUIT:
          state.quit = true;
          break;
      }
    }
    
    const bool *keystate = SDL_GetKeyboardState(NULL);
    
    std::clog << state.camera.sector << "\n";

    state.camera.angle = normalise_angle(state.camera.angle);
    if (!inside_sector(state.camera.pos, &state.sectors.arr[state.camera.sector])) {
      for (usize i = 0; i < state.sectors.n; i++) {
        std::clog << "\t" << i << "\n";
        if (inside_sector(state.camera.pos, &state.sectors.arr[i])) {
          state.camera.sector = i;
          state.camera.z = state.sectors.arr[i].zfloor + CAMZ;
        }
      }
    }
    //memset(state.pixels, 0xFF, SCREEN_WIDTH *SCREEN_HEIGHT * sizeof(u32));
    clear_pixels();

    if (keystate[SDL_SCANCODE_M]) 
        render_map();
    else {
      render();
    }
    
    if (state.frame < 8)
      continue;

    if (keystate[SDL_SCANCODE_LEFT]) 
      state.camera.angle += PI/3 * dt;
    if (keystate[SDL_SCANCODE_RIGHT])
      state.camera.angle -= PI/3 * dt;
    
    v2 newpos = state.camera.pos;
    
    if (keystate[SDL_SCANCODE_W])
      newpos = add_v2(newpos, rotate_vector({0,  3 * dt}, -state.camera.angle));
    if (keystate[SDL_SCANCODE_S])
      newpos = add_v2(newpos, rotate_vector({0, -3 * dt}, -state.camera.angle));
    if (keystate[SDL_SCANCODE_A])
      newpos = add_v2(newpos, rotate_vector({0, -3 * dt}, -state.camera.angle + (PI / 2.0f)));
    if (keystate[SDL_SCANCODE_D])
      newpos = add_v2(newpos, rotate_vector({0, -3 * dt}, -state.camera.angle - (PI / 2.0f)));
   
    state.camera.pos = newpos;

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
