#include <include/fbuffer.h>
#include <include/vector_space.h>
#include <include/rasterize.h>

struct triangle_rasterizer
{
	vector2d center_scr;
	vector2d size_scr;

	struct pixel {
		fbuffer::vector vec;
		fbuffer::color col;
	};

	inline void set_from_fb(fbuffer &fb)
	{
		set_params(fbuffer::vector{ 0, 0 },
			   fbuffer::vector{ fb.xres - 1, fb.yres - 1 });
	}

	inline void set_params(fbuffer::vector min,
			       fbuffer::vector max)
	{
		center_scr = vector2d{ (double)(max.x + min.x) / 2,
				       (double)(max.y + min.y) / 2 };

		size_scr = vector2d{ (double)(max.x - min.x) / 2,
				     (double)(max.y - min.y) / 2 };
	}

	inline double get_ratio()
	{
		return size_scr.x / size_scr.y;
	}

	inline fbuffer::vector transform(vector2d &vec)
	{
		return fbuffer::vector{
			std::uint32_t(center_scr.x + vec.x * size_scr.x + 0.5f),
			std::uint32_t(center_scr.y + vec.y * size_scr.y + 0.5f)
		};
	}

	inline vector2d back_transform(fbuffer::vector &vec)
	{
		return vector2d{
			double((vec.x - center_scr.x - 0.5f) / size_scr.x),
			double((vec.y - center_scr.y - 0.5f) / size_scr.y)
		};
	}

	void rasterize(vector3d const (&vec)[3], fbuffer::color const (&col)[3], std::vector<pixel> &out)
	{
		vector2d d1 = direct_proj(vec[1] - vec[0]);
		vector2d d2 = direct_proj(vec[2] - vec[0]);

		double det = d1.x * d2.y - d1.y * d2.x;

		vector2d min_f{std::min(vec[0].x, std::min(vec[1].x, vec[2].x)), std::min(vec[0].y, std::min(vec[1].y, vec[2].y))};
		vector2d max_f{std::max(vec[0].x, std::max(vec[1].x, vec[2].x)), std::max(vec[0].y, std::max(vec[1].y, vec[2].y))};

		fbuffer::vector min = transform(min_f);
		fbuffer::vector max = transform(max_f);

		fbuffer::vector r = min;
		for (r.y = min.y; r.y <= max.y; r.y++) {
			for (r.x = min.x; r.x <= max.x; r.x++) {
				vector2d r_fl = back_transform(r) - direct_proj(vec[0]);
				double det1 = r_fl.x * d2.y - r_fl.y * d2.x;
				double det2 = d1.x * r_fl.y - d1.y * r_fl.x;
				double c[3] = {(det - det1 - det2) / det, det1 / det, det2 / det};
				if (c[0] < 0 || c[1] < 0 || c[2] < 0)
					continue;
				fbuffer::color pix_col{uint8_t(255 * c[0]), uint8_t(255 * c[1]), uint8_t(255 * c[2]), 0};
				out.push_back(pixel{r, pix_col});
			}
		}
	}
};

int main(int argc, char *argv[])
{
	fbuffer fb;
	fb.init("/dev/fb0");
	triangle_rasterizer rast;
	rast.set_from_fb(fb);

	vector3d vec[3];
	vec[0] = vector3d{-0.4, 0.4, 0};
	vec[1] = vector3d{0.6, -0.7, 0};
	vec[2] = vector3d{0.8, 0.9, 0};

	fbuffer::color col[3];

	std::vector<triangle_rasterizer::pixel> out;

	rast.rasterize(vec, col, out);
	fb.clear();
	for (auto const &p : out)
		fb[p.vec.y][p.vec.x] = p.col;
	fb.update();

	return 0;
}
