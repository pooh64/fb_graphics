#ifndef FB_INFO_H
#define FB_INFO_H

#include <linux/fb.h>
#include <stdint.h>

typedef struct fb_vector {
	uint32_t x;
	uint32_t y;
} fb_vector_t;

typedef union fb_pixel {
	struct {
		uint8_t b : 8;
		uint8_t g : 8;
		uint8_t r : 8;
		uint8_t a : 8;
	};
	uint32_t val;
} fb_pixel_t;

typedef struct fb_info {
	char		path[128];
	int		fd;
	union fb_pixel *buf;
	size_t		buf_s;
	struct		fb_fix_screeninfo finfo;
	struct		fb_var_screeninfo vinfo;
} fb_info_t;

int fb_open(fb_info_t *fb, const char *path);
int fb_close(fb_info_t *fb);

void fb_clean(fb_info_t *fb);
void fb_capture(fb_info_t *fb, fb_pixel_t *buf);

void fb_set(fb_info_t *fb, fb_pixel_t *buf);


void fb_draw_rect(fb_info_t *fb, fb_vector_t v1, fb_vector_t sz_v,
		  fb_pixel_t pix);
void fb_draw_line(fb_info_t *fb, fb_vector_t beg, fb_vector_t end,
		  fb_pixel_t pix);

#include "include/fb_inline.h"

#endif /* FB_INFO_H */
