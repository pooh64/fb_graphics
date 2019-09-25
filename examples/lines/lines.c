#include <stdlib.h>
#include <stdio.h>
#include "include/fb_info.h"

int main(int argc, char *argv[])
{
	fb_info_t fb;
	fb_pixel_t pix = {};
	uint32_t xres, yres;

	if (fb_open(&fb, "/dev/fb0") < 0) {
		perror("fb_open");
		return EXIT_FAILURE;
	}

	pix.g = 255;
	xres = fb_get_xres(&fb);
	yres = fb_get_yres(&fb);

	fb_draw_line(&fb,
		     (fb_vector_t) {.x = 0,
		     		    .y = 0},
		     (fb_vector_t) {.x = xres - 1,
		     		    .y = yres - 1},
				    pix);

	fb_draw_line(&fb,
		     (fb_vector_t) {.x = 0,
		     		    .y = yres - 1},
		     (fb_vector_t) {.x = xres - 1,
		     		    .y = 0},
				    pix);

	fb_draw_line(&fb,
		     (fb_vector_t) {.x = xres / 2,
		     		    .y = 0 },
		     (fb_vector_t) {.x = xres / 2,
		     		    .y = yres - 1},
				    pix);

	fb_draw_rect(&fb,
		     (fb_vector_t) {.x = xres / 4, yres / 4},
		     (fb_vector_t) {.x = xres / 2, yres / 2},
		     pix);

	if (fb_close(&fb)) {
		perror("fb_close");
		return EXIT_FAILURE;
	}

	return 0;
}
