#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include "SDL.h"

#define map_width 10
#define map_height 10
#define screen_width 320
#define screen_height 320
#define cell_size 32

typedef uint8_t u8;
typedef uint32_t u32;

#define ASSERT(_e, ...) if (!(_e)) { fprintf(stderr, __VA_ARGS__); exit(1); }

typedef struct {
    double x;
    double y;
} vec2;

#define dot(v0, v1) \
    ({ const vec2 _v0 = (v0), _v1 = (v1); (_v0.x * _v1.x) + (_v0.y * _v1.y); })

#define length(v) ({ const vec2 _v = (v); sqrt(dot(_v, _v)); })
#define normalize(u) ({  \
    const vec2 _u = (u); \
    const float l = length(_u); \
    (vec2) {_u.x / l, _u.y / l};  \
})

#define vec_add(v0, v1)({ \
    const vec2 _v0 = (v0), _v1 = (v1); \
    (vec2) {_v0.x + _v1.x, _v0.y + _v1.y}; \
})
#define vec_sub(v0, v1)({ \
    const vec2 a = (v0), b = (v1); \
    (vec2) {a.x - b.x, a.y - b.y}; \
})

#define vec_scale(v0, scale)({  \
    (vec2){v0.x * scale, v0.y * scale}; \
})

#define min(a, b) ({ \
    __typeof__(a) _a = (a), _b = (b); _a < _b ? _a : _b; \
})

#define max(a, b)({ \
    __typeof__(a) _a = (a), _b = (b); _a > _b ? _a : _b; \
})

#define vec_copy(v)({ \
    const vec2 _v = (v); (vec2){_v.x, _v.y}; \
})

int worldMap[map_height][map_width]={
  {1,1,1,1,1,1,1,1,1,1,},
  {1,0,0,0,0,0,0,0,0,1,},
  {1,0,0,0,0,0,0,0,0,1,},
  {1,0,1,1,0,0,0,0,0,1,},
  {1,0,1,1,0,0,0,0,0,1,},
  {1,0,0,0,0,0,0,0,0,1,},
  {1,0,0,0,0,0,0,0,0,1,},
  {1,0,0,0,0,0,0,1,0,1,},
  {1,0,0,0,0,0,0,0,0,1,},
  {1,1,1,1,1,1,1,1,1,1,},
};

struct sdl_state {
    SDL_Window *window;
    SDL_Renderer *renderer;
    bool quit;
    vec2 pos, dir, plane;
};

typedef struct {
    vec2 pos;
    int h;
    int w;
    u32 color;
} box;

typedef struct {
    int h;
    int w;
    u32 color;
    vec2 pos, dir, plane;
    struct sdl_state *sdl;
} player;

void vec_print(const vec2 v){ printf("x: %f, y: %f \n", v.x, v.y); }
void print_float(const float f){ printf("FLOAT: %f \n", f); }
void print_cord(const float x, const float y){ printf("X: %f, Y: %f \n", x, y); }
void printf_float(const char *str, const float f){ printf("%s: %f \n", str, f); }

void render_box(SDL_Renderer *renderer, box *b) {
    SDL_Rect rect;
    rect.x = (int)b->pos.x;
    rect.y = (int)b->pos.y;
    rect.w = b->w;
    rect.h = b->h;
    SDL_SetRenderDrawColor(renderer, 
                           (b->color >> 24) & 0xFF, 
                           (b->color >> 16) & 0xFF, 
                           (b->color >> 8) & 0xFF, 
                           b->color & 0xFF);
    SDL_RenderFillRect(renderer, &rect);
}

void render_empty_box(SDL_Renderer *renderer, box *b) {
    SDL_Rect rect;
    rect.x = (int)b->pos.x;
    rect.y = (int)b->pos.y;
    rect.w = b->w;
    rect.h = b->h;
    SDL_SetRenderDrawColor(renderer, 
                           (b->color >> 24) & 0xFF, 
                           (b->color >> 16) & 0xFF, 
                           (b->color >> 8) & 0xFF, 
                           b->color & 0xFF);
    SDL_RenderDrawRect(renderer, &rect);
}

void draw_rect(SDL_Renderer *renderer, int x, int y, int w, int h, u32 color)
{
    SDL_Rect rect;
    rect.x = x;
    rect.y = y;
    rect.w = w;
    rect.h = h;
    SDL_SetRenderDrawColor(renderer,
                           (color >> 24) & 0xFF,
                           (color >> 16) & 0xFF,
                           (color >> 8) & 0xFF,
                           color & 0xFF);
    SDL_RenderFillRect(renderer, &rect);
}

// ------ PLAYER ----- // 

void player_render(player *p){
    draw_rect(p->sdl->renderer, p->pos.x * cell_size, p->pos.y * cell_size, p->w, p->h, p->color);
    /*
    vec2 dir = vec_scale(p->dir, 20);
    SDL_RenderDrawLine(
        p->sdl->renderer, (p->pos.x * cell_size) + 5,
                          (p->pos.y * cell_size) + 5,
                          ( (p->pos.x + dir.x) * cell_size) + 5,
                          ( (p->pos.y + dir.y) * cell_size) + 5 );
    */
}

void player_rot_dir(player *p, double angle_rad){
    vec2 v = {p->dir.x, p->dir.y};
    vec2 pv = {p->plane.x, p->plane.y};
    double cos_theta = cos(angle_rad);
    double sin_theta = sin(angle_rad);
    p->dir.x = v.x * cos_theta - v.y * sin_theta;
    p->dir.y = v.x * sin_theta + v.y * cos_theta;
    p->plane.x = pv.x * cos_theta - pv.y * sin_theta;
    p->plane.y = pv.x * sin_theta + pv.y * cos_theta;
}

void raycast(player *p) {

    for(int i = 0; i < screen_width; i++){

        vec2 ray_start __attribute__((unused)) = vec_copy(p->pos);
        vec2 pos = vec_copy(p->pos);

        double cam_x = (2 * (i / (double)screen_width)) - 1; // |-1     0      1|
        vec2 ray_dir = {p->dir.x + p->plane.x * cam_x, p->dir.y + p->plane.y * cam_x};

        if(i == 0){
            SDL_RenderDrawLine(
                p->sdl->renderer,
                (p->pos.x * cell_size) + 5,
                (p->pos.y * cell_size) + 5,
                ((p->pos.x + p->plane.x) * cell_size) + 5,
                ((p->pos.y + p->plane.y) * cell_size) + 5);
        }

        int map_x = (int)pos.x; // current square on tile map
        int map_y = (int)pos.y; // ^
        double side_dist_x;      // dist ray has to travel from start to first x & y
        double side_dist_y;

        double delta_dist_x = (ray_dir.x == 0) ? 1e30 : fabs(1 / ray_dir.x);
        double delta_dist_y = (ray_dir.y == 0) ? 1e30 : fabs(1 / ray_dir.y);
        double perp_wall_dist;

        int step_x;
        int step_y;

        int hit = 0; // was there a wall hit?
        int side;    // was a NS or a EW wall hit? = 0; // was there a wall hit?

        if (ray_dir.x < 0) {
            step_x = -1;
            side_dist_x = (pos.x - map_x) * delta_dist_x;
        } else {
            step_x = 1;
            side_dist_x = (map_x + 1.0 - pos.x) * delta_dist_x;
        }
        if (ray_dir.y < 0) {
            step_y = -1;
            side_dist_y = (pos.y - map_y) * delta_dist_y;
        } else{
            step_y = 1;
            side_dist_y = (map_y + 1.0 - pos.y) * delta_dist_y;
        }

        //perform DDA
        while (hit == 0){
                //jump to next map square, either in x-direction, or in y-direction
                if (side_dist_x < side_dist_y) {
                side_dist_x += delta_dist_x;
                map_x += step_x;
                side = 0;
            } else {
                side_dist_y += delta_dist_y;
                map_y += step_y;
                side = 1;
            }
            //Check if ray has hit a wall
            if (worldMap[map_y][map_x] > 0) hit = 1;
        }

        if (side == 0){
            perp_wall_dist = (side_dist_x - delta_dist_x);
        } else {
            perp_wall_dist = (side_dist_y - delta_dist_y);
        }


        SDL_RenderDrawLine(
            p->sdl->renderer,
            (p->pos.x * cell_size) + 5,
            (p->pos.y * cell_size) + 5,
            ((p->pos.x + perp_wall_dist * ray_dir.x) * cell_size) ,
            ((p->pos.y + perp_wall_dist * ray_dir.y) * cell_size) );
        

    } // end for 

    
}



int main(void){

    struct sdl_state state;

    state.quit = false;

    ASSERT(
        !SDL_Init(SDL_INIT_VIDEO),
        "SDL failed to initialize: %s\n",
        SDL_GetError());

    state.window =
        SDL_CreateWindow(
            "DEMO",
            SDL_WINDOWPOS_CENTERED,
            SDL_WINDOWPOS_CENTERED,
            screen_width,
            screen_height,
            SDL_WINDOW_SHOWN);
    ASSERT(
        state.window,
        "failed to create SDL window: %s\n", SDL_GetError());

    state.renderer =
        SDL_CreateRenderer(state.window, -1, SDL_RENDERER_PRESENTVSYNC);
    ASSERT(
        state.renderer,
        "failed to create SDL renderer: %s\n", SDL_GetError());

    
    player player_1 = {
        10, 10, 
        0xFFFF0000,
        {5, 5}, 
        normalize(((vec2){-1.0f, 0.1f})),
        {0.0f, 0.66f}, 
        &state
    };

    //box b1 = {{300, 300}, 10, 10, 0xFFFF0000};

    SDL_SetRenderDrawColor(state.renderer, 255, 0, 0, 255); // Set draw color to red

    while(!state.quit){

        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            switch (ev.type) {
                case SDL_QUIT:
                    state.quit = true;
                    break;
            }
        }

        const u8 *keystate = SDL_GetKeyboardState(NULL);
        if (keystate[SDL_SCANCODE_LEFT]) {
            player_rot_dir(&player_1, -.12);
        }

        if (keystate[SDL_SCANCODE_RIGHT]) {
            player_rot_dir(&player_1, .12);
        }

        if (keystate[SDL_SCANCODE_UP]) {
            player_1.pos.x += player_1.dir.x * .1;
            player_1.pos.y += player_1.dir.y * .1;
        }

        if (keystate[SDL_SCANCODE_DOWN]) {
            player_1.pos.x -= player_1.dir.x * .1;
            player_1.pos.y -= player_1.dir.y * .1;
        }
        

        // Clear the screen
        SDL_SetRenderDrawColor(state.renderer, 0, 0, 0, 255); // Black
        SDL_RenderClear(state.renderer);

        // ------- RENDER ------- //

        for(int i = 0; i < map_height; i++){
            for(int j = 0; j < map_width; j++){
                if(worldMap[i][j] == 1){
                    box bound = {{j * cell_size, i * cell_size}, cell_size, cell_size, 0xFF00FF00};
                    render_empty_box(state.renderer, &bound);
                }
            }
        }
        
        player_render(&player_1);
        raycast(&player_1);


        // -------------------- //



        // Present the rendered frame
        SDL_RenderPresent(state.renderer);

    }

    SDL_DestroyRenderer(state.renderer);
    SDL_DestroyWindow(state.window);

    SDL_Quit();

    return 0;
}
