#pragma once
#include <include/geom.h>
#include <vector>
#include <cassert>

template<typename _rz_coord, typename _rz_baryc>
struct rasterizer {
protected:
	uint32_t wnd_min_x, wnd_max_x, wnd_min_y, wnd_max_y;
public:
	using rz_coord = _rz_coord;
	using rz_baryc = _rz_baryc;

	struct rz_out {
		uint32_t x;
		uint32_t pid;
		rz_baryc bc;
		float depth;
	};

	void set_window(window const &wnd)
	{
		wnd_min_x = wnd.x;
		wnd_min_y = wnd.y;
		wnd_max_x = wnd.x + wnd.w - 1;
		wnd_max_y = wnd.y + wnd.h - 1;
	}

	virtual void rasterize(rz_coord const &, uint32_t,
		       std::vector<std::vector<rz_out>> &) = 0;
};

struct tr_rasterizer final: public rasterizer<vec3[3], float[3]> {
public:
	void rasterize(rz_coord const &tr, uint32_t pid,
		       std::vector<std::vector<rz_out>> &buf) override
	{
		vec3 d1_3 = tr[1] - tr[0];
		vec3 d2_3 = tr[2] - tr[0];
		vec2 d1 = { d1_3.x, d1_3.y };
		vec2 d2 = { d2_3.x, d2_3.y };

		float det = d1.x * d2.y - d1.y * d2.x;
		if (det <= 0)
			return;

	 	vec2 min_r{std::min(tr[0].x, std::min(tr[1].x, tr[2].x)),
			   std::min(tr[0].y, std::min(tr[1].y, tr[2].y))};
		vec2 max_r{std::max(tr[0].x, std::max(tr[1].x, tr[2].x)),
			   std::max(tr[0].y, std::max(tr[1].y, tr[2].y))};
		uint32_t min_x = std::max(uint32_t(min_r.x), wnd_min_x);
		uint32_t min_y = std::max(uint32_t(min_r.y), wnd_min_y);
		uint32_t max_x = std::min(uint32_t(max_r.x), wnd_max_x);
		uint32_t max_y = std::min(uint32_t(max_r.y), wnd_max_y);

		vec2 r0 = vec2 { tr[0].x, tr[0].y };
		vec3 depth_vec = { tr[0].z, tr[1].z, tr[2].z };
		rz_out out;

define RAST_HACK
#ifdef RAST_HACK
//#define RAST_HACK_MORE
		vec2 rel_0 = vec2{float(min_x), float(min_y)} - r0;
		float c_1_x =  d2.y / det;
		float c_1_y = -d2.x / det;
		float c_2_x = -d1.y / det;
		float c_2_y =  d1.x / det;
		float c_0_x = -c_1_x - c_2_x;
		float c_0_y = -c_1_y - c_2_y;

		vec3 bc_dx = { c_0_x, c_1_x, c_2_x };
		vec3 bc_dy = { c_0_y, c_1_y, c_2_y };

		vec3 bc_0;
		bc_0[1] = c_1_x * rel_0.x + c_1_y * rel_0.y,
		bc_0[2] = c_2_x * rel_0.x + c_2_y * rel_0.y;
		bc_0[0] = 1 - bc_0[1] - bc_0[2];
		vec3 bc = bc_0;
#ifndef RAST_HACK_MORE
		for (uint32_t y = min_y; y <= max_y; ++y) {
			bc = bc_0 + (y - min_y) * bc_dy;
			for (uint32_t x = min_x; x <= max_x; ++x) {
				if (bc[0] >= 0 && bc[1] >= 0 && bc[2] >= 0) {
					out.depth = dot_prod(bc, depth_vec);
					out.x = x;
					out.pid = pid;
					out.bc[0] = bc[0];
					out.bc[1] = bc[1];
					out.bc[2] = bc[2];
					buf[y].push_back(out);
				}
				bc = bc + bc_dx;
			}
		}
#endif
#ifdef RAST_HACK_MORE
		for (uint32_t y = min_y; y <= max_y; /* nothing */) {
			for (uint32_t x = min_x; x <= max_x; ++x) {
				if (bc[0] >= 0 && bc[1] >= 0 && bc[2] >= 0) {
					out.depth = dot_prod(bc, depth_vec);
					out.x = x;
					out.pid = pid;
					out.bc[0] = bc[0];
					out.bc[1] = bc[1];
					out.bc[2] = bc[2];
					buf[y].push_back(out);
				}
				bc = bc + bc_dx;
			}
			bc = bc + bc_dy;
			++y;
			for (uint32_t x = max_x; x >= min_x; --x) {
				if (bc[0] >= 0 && bc[1] >= 0 && bc[2] >= 0) {
					out.depth = dot_prod(bc, depth_vec);
					out.x = x;
					out.pid = pid;
					out.bc[0] = bc[0];
					out.bc[1] = bc[1];
					out.bc[2] = bc[2];
					buf[y].push_back(out);
				}
				bc = bc - bc_dx;
			}
			bc = bc + bc_dy;
			++y;
		}

#endif
#endif
#ifndef RAST_HACK
		for (uint32_t y = min_y; y <= max_y; ++y) {
			for (uint32_t x = min_x; x <= max_x; ++x) {
				vec2 rel = vec2{float(x), float(y)} - r0;
				float det1 = rel.x * d2.y - rel.y * d2.x;
				float det2 = d1.x * rel.y - d1.y * rel.x;
				out.bc[0] = (det - det1 - det2) / det;
				out.bc[1] = det1 / det;
				out.bc[2] = det2 / det;
				if (out.bc[0] < 0 || out.bc[1] < 0 ||
				     out.bc[2] < 0)
					continue;
				out.depth = dot_prod((reinterpret_cast<vec3&>
						(out.bc)), depth_vec);
				out.x = x;
				out.pid = pid;
				buf[y].push_back(out);
			}
		}
#endif
	}
};
