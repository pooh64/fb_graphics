#pragma once

#include <include/fbuffer.h>
#include <vector>
#include <cmath>

struct line_rasterizer {
private:
	fbuffer::vector min_scr;
	fbuffer::vector max_scr;

	vector2d center_scr;
	vector2d size_scr;

	fbuffer::vector transform(vector2d &vec);

	bool check_bounds(fbuffer::vector &vec);

public:
	void set_from_fb(fbuffer &fb);
	void set_params(fbuffer::vector min, fbuffer::vector max);
	double get_ratio();

	void rasterize(vector2d &_beg, vector2d &_end,
		       std::vector<fbuffer::vector> &out);
};

inline void line_rasterizer::set_from_fb(fbuffer &fb)
{
	set_params(fbuffer::vector{0, 0},
		   fbuffer::vector{fb.xres - 1, fb.yres - 1});
}

inline void line_rasterizer::set_params(fbuffer::vector min,
					fbuffer::vector max)
{
	min_scr = min;
	max_scr = max;

	center_scr = vector2d{(double) (max.x + min.x) / 2,
			      (double) (max.y + min.y) / 2};

	size_scr   = vector2d{(double) (max.x - min.x) / 2,
			      (double) (max.y - min.y) / 2};
}

inline double line_rasterizer::get_ratio()
{
	return size_scr.x / size_scr.y;
}

inline fbuffer::vector line_rasterizer::transform(vector2d &vec)
{
	return fbuffer::vector
		{std::uint32_t(center_scr.x + vec.x * size_scr.x + 0.5f),
		 std::uint32_t(center_scr.y + vec.y * size_scr.y + 0.5f)};
}

inline bool line_rasterizer::check_bounds(fbuffer::vector &vec)
{
	if ((vec.x < min_scr.x) || (vec.x > max_scr.x) ||
	    (vec.y < min_scr.y) || (vec.y > max_scr.y))
		return false;
	return true;
}

inline void line_rasterizer::rasterize(vector2d &_beg, vector2d &_end,
				       std::vector<fbuffer::vector> &out)
{
	fbuffer::vector beg = transform(_beg);
	fbuffer::vector end = transform(_end);

	if (!check_bounds(beg) || !check_bounds(end))
		return;

	fbuffer::vector vec;
	double k;

	if (std::abs((double) end.y - beg.y) >
	    std::abs((double) end.x - beg.x)) {
		if (beg.y > end.y)
			std::swap(end, beg);

		k = ((double) end.x - beg.x) / ((double) end.y - beg.y);

		for (vec.y = beg.y; vec.y <= end.y; vec.y++) {
			vec.x = beg.x + k * ((double) vec.y - beg.y) + 0.5f;
			out.push_back(vec);
		}
		return;
	}

	if (beg.x > end.x)
		std::swap(end, beg);

	k = ((double) end.y - beg.y) / ((double) end.x - beg.x);

	for (vec.x = beg.x; vec.x <= end.x; vec.x++) {
		vec.y = beg.y + k * ((double) vec.x - beg.x) + 0.5f;
		out.push_back(vec);
	}
}
