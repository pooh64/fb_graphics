#pragma once

#include <cstdint>
#include <cstdlib>

extern "C" {
#include <linux/fb.h>
#include <stdint.h>
}

struct fbuffer : public fb_var_screeninfo, public fb_fix_screeninfo {
	struct color {
		std::uint8_t b, g, r, a;
	};

	struct vector {
		std::uint32_t x, y;
	};

	color *buf;

	color *operator[](std::uint32_t);
	color const *operator[](std::uint32_t) const;

	int init(const char *path);
	int destroy();
	int update();

	void fill(color c);
	void clear();

    private:
	int fd;
};

inline fbuffer::color *fbuffer::operator[](std::uint32_t y)
{
	return buf + y * xres;
}

inline fbuffer::color const *fbuffer::operator[](std::uint32_t y) const
{
	return buf + y * xres;
}

inline void fbuffer::fill(fbuffer::color c)
{
	for (std::size_t i = 0; i < xres * yres; ++i)
		buf[i] = c;
}

inline void fbuffer::clear()
{
	color empty{ 0, 0, 0, 0 };
	for (std::size_t i = 0; i < xres * yres; ++i)
		buf[i] = empty;
}
