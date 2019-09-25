#ifndef FB_INLINE_H
#define FB_INLINE_H

#include "include/fb_info.h"

static inline uint32_t fb_get_xres(fb_info_t *fb)
{
	return fb->vinfo.xres;
}

static inline uint32_t fb_get_yres(fb_info_t *fb)
{
	return fb->vinfo.yres;
}

static inline fb_pixel_t *fb_get_buf(fb_info_t *fb)
{
	return fb->buf;
}

static inline fb_pixel_t *fb_get_pixel(fb_info_t *fb, fb_vector_t vec)
{
	return fb->buf + vec.y * fb_get_xres(fb) + vec.x;
}

static inline void fb_vector_swap(fb_vector_t *v1, fb_vector_t *v2)
{
	fb_vector_t tmp = *v1;
	*v1 = *v2;
	*v2 = tmp;
}

#endif /* FB_INLINE_H */
