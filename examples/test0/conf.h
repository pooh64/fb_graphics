#pragma once

#define DEV_FB_PATH "/dev/fb0"

/* Linear textures, acceptable for highpoly models */
#define HACK_TRINTERP_LINEAR
/* Textures boundary check, protection for bad texture mappings */
/* Sky illustrates this using segfault */
//#define HACK_TRSHADER_NO_BOUNDS
/* Disable fb drawback */
//#define HACK_DRAWBIN_NO_DRAWBACK

#define DRAW_SKY
#define DRAW_A6M
//#define N_THREADS 4
#define N_THREADS (std::thread::hardware_concurrency())
#define DRAWBACK
#define N_FRAMES 500

#define TILE_SIZE 16
#define BIN_SIZE 4

#define EYE_POS {0, -0.18, 0.8}

#define SKY_SCALE 1000
#define A6M_SCALE 0.05

/* mouse */
//#define MOUSE_ROTATE
#define MOUSE_ROTSPD 0.1

#define SKY_OBJ_PATH "sky.obj"
#define A6M_OBJ_PATH "A6M.obj"
