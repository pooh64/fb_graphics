#pragma once

#include <cstdint>
#include <cstdio>

extern "C"
{
	#include <linux/fb.h>
	#include <stdint.h>
}

struct vector2d {
	double x, y;
	vector2d(double _x, double _y) : x(_x), y(_y) { }
	vector2d operator*(double m) const { return vector2d(x * m, y * m); }

	void print() { std::printf("(%lg, %lg)\n", x, y); }
};

struct vector3d {
	double x, y, z;
};

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

	color &operator()(std::uint32_t x, std::uint32_t y);

	void fill(color c);

	vector2d transform(vector2d vec);
	void draw_line(vector2d beg, vector2d end, color c);

private:
	int fd;
};

inline vector2d fbuffer::transform(vector2d vec)
{
	return vector2d((-1) * vec.x * xres + (double) xres / 2,
			(-1) * vec.y * xres + (double) yres / 2);
}

inline void fbuffer::draw_line(vector2d beg, vector2d end, color c)
{
	beg = transform(beg);
	end = transform(end);

	if (end.x < beg.x) {
		vector2d tmp = beg;
		beg = end;
		end = tmp;
	}

	double k = (end.y - beg.y) / (end.x - beg.x);

	for (std::uint32_t x = beg.x + 1/2; x < end.x + 1/2; x++)
		(*this)(x, beg.y + k * ((double) x - beg.x) + 1/2) = c;
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
