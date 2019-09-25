#define _GNU_SOURCE

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>

#include <string.h>

#include "include/fb_info.h"

int fb_open(struct fb_info *fb, const char *path)
{
	size_t path_len = strlen(path);
	if (path_len + 1 > sizeof(fb->path))
		goto handle_err_0;
	memcpy(fb->path, path, path_len + 1);

	fb->fd = open(fb->path, O_RDWR);
	if (fb->fd == -1)
		goto handle_err_0;

	if (ioctl(fb->fd, FBIOGET_FSCREENINFO, &fb->finfo) < 0)
		goto handle_err_1;

	if (ioctl(fb->fd, FBIOGET_VSCREENINFO, &fb->vinfo) < 0)
		goto handle_err_1;

	fb->buf_s = fb->vinfo.xres * fb->vinfo.yres;

	fb->buf = mmap(NULL, fb->buf_s * sizeof(*fb->buf),
		       PROT_READ | PROT_WRITE, MAP_SHARED,
		       fb->fd, 0);
	if (fb->buf == MAP_FAILED)
		goto handle_err_1;

	return 0;

handle_err_1:
	close(fb->fd);
handle_err_0:
	return -1;
}

int fb_close(struct fb_info *fb)
{
	if (munmap(fb->buf, fb->buf_s * sizeof(*fb->buf)) < 0)
		return -1;
	if (close(fb->fd) < 0)
		return -1;
	return 0;
}

void fb_clean(struct fb_info *fb)
{
	memset(fb->buf, 0, fb->buf_s * sizeof(*fb->buf));
}

void fb_capture(struct fb_info *fb, union fb_pixel *buf)
{
	memcpy(buf, fb->buf, fb->buf_s * sizeof(*fb->buf));
}

void fb_set(struct fb_info *fb, union fb_pixel *buf)
{
	memcpy(fb->buf, buf, fb->buf_s * sizeof(*fb->buf));
}

void fb_draw_line(fb_info_t *fb,
		  fb_vector_t beg, fb_vector_t end, fb_pixel_t pix)
{
	fb_vector_t vec;

	if (beg.x == end.x) {
		if (beg.y > end.y)
			fb_vector_swap(&beg, &end);

		vec = (fb_vector_t) {.x = beg.x, .y = beg.y};

		while (vec.y <= end.y) {
			*fb_get_pixel(fb, vec) = pix;
			vec.y++;
		}
		return;
	}

	if (beg.x > end.x)
		fb_vector_swap(&beg, &end);

	double k = ((double) end.y - (double) beg.y) /
		   ((double) end.x - (double) beg.x);

	for (vec.x = beg.x; vec.x <= end.x; vec.x++) {
		vec.y = k * vec.x + (double) beg.y + (double) 1 / 2;
		*fb_get_pixel(fb, vec) = pix;
	}
}

void fb_draw_rect(fb_info_t *fb, fb_vector_t v1, fb_vector_t sz_v,
		  fb_pixel_t pix)
{
	fb_vector_t v2, v3, v4;

	v2 = (fb_vector_t) {.x = v1.x,		.y = v1.y + sz_v.y};
	v3 = (fb_vector_t) {.x = v1.x + sz_v.x,	.y = v1.y + sz_v.y};
	v4 = (fb_vector_t) {.x = v1.x + sz_v.x,	.y = v1.y};

	fb_draw_line(fb, v1, v2, pix);
	fb_draw_line(fb, v2, v3, pix);
	fb_draw_line(fb, v3, v4, pix);
	fb_draw_line(fb, v4, v1, pix);
}
