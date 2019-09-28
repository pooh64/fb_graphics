#pragma once

#include <cstdint>
#include <cstdio>
#include <algorithm>

#include <include/vector_space.h>

extern "C"
{
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

	int init(const char *path);
	int destroy();
	int update();

	color &operator()(std::uint32_t x, std::uint32_t y);

	void fill(color c);

	vector transform(vector2d vec);
	void draw_line(vector2d beg, vector2d end, color c);
	void draw_line_strip(vector2d arr[], std::size_t arr_s, color c);

private:
	int fd;
};

inline fbuffer::vector fbuffer::transform(vector2d vec)
{
	return vector{std::uint32_t((-vec.x * yres + (double) xres + 1) / 2),
		      std::uint32_t((-vec.y * yres + (double) yres + 1) / 2)};
}

inline void fbuffer::draw_line(vector2d _beg, vector2d _end, fbuffer::color c)
{
	fbuffer::vector beg = fbuffer::transform(_beg);
	fbuffer::vector end = fbuffer::transform(_end);

	fbuffer::vector vec;
	double k;

	if (std::abs((double) end.y - beg.y) >
	    std::abs((double) end.x - beg.x)) {
		if (beg.y > end.y)
			std::swap(end, beg);

		k = ((double) end.x - beg.x) / ((double) end.y - beg.y);

		for (vec.y = beg.y; vec.y <= end.y; vec.y++) {
			vec.x = beg.x + k * ((double) vec.y - beg.y);
			(*this)(vec.x, vec.y) = c;
		}
		return;
	}

	if (beg.x > end.x)
		std::swap(end, beg);

	k = ((double) end.y - beg.y) / ((double) end.x - beg.x);

	for (vec.x = beg.x; vec.x <= end.x; vec.x++) {
		vec.y = beg.y + k * ((double) vec.x - beg.x) + 1/2;
		(*this)(vec.x, vec.y) = c;
	}
}

inline void fbuffer::draw_line_strip(vector2d arr[], std::size_t arr_s,
				     fbuffer::color c)
{
	for (std::size_t i = 0; i < arr_s - 1; i++)
		draw_line(arr[i], arr[i + 1], c);
}

inline fbuffer::color &fbuffer::operator()(std::uint32_t x, std::uint32_t y)
{
	return buf[x + y * xres];
}

inline void fbuffer::fill(fbuffer::color c)
{
	for (std::size_t i = 0; i < xres * yres; i++)
		buf[i] = c;
}
