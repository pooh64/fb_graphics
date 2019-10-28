#pragma once
#include <include/geom.h>
#include <vector>
#include <cassert>

template<typename _rz_in, typename _rz_out>
struct rasterizer {
protected:
	uint32_t wnd_min_x, wnd_max_x, wnd_min_y, wnd_max_y;
public:
	using rz_in  = _rz_in;
	using rz_out = _rz_out;

	void set_window(window const &wnd)
	{
		wnd_min_x = wnd.x;
		wnd_min_y = wnd.y;
		wnd_max_x = wnd.x + wnd.w - 1;
		wnd_max_y = wnd.y + wnd.h - 1;
	}

	virtual void rasterize(rz_in const &, uint32_t,
			       std::vector<std::vector<rz_out>> &) = 0;
};


struct tr_rz_out {
	union {
		struct {
			float bc[3];
			float depth;
		};
		vec4 sse_data;
	};
	uint32_t x;
	uint32_t pid;
};

struct tr_rasterizer final: public rasterizer<vec3[3], tr_rz_out> {
public:

	void rasterize(rz_in const &tr, uint32_t pid,
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
		rz_out out;

#define RAST_HACK
#ifdef RAST_HACK
		vec4 depth_vec = { tr[0].z, tr[1].z, tr[2].z, 0 };
		vec2 rel_0 = vec2{float(min_x), float(min_y)} - r0;
		float bc_d1_x =  d2.y / det;
		float bc_d1_y = -d2.x / det;
		float bc_d2_x = -d1.y / det;
		float bc_d2_y =  d1.x / det;
		float bc_d0_x = -bc_d1_x - bc_d2_x;
		float bc_d0_y = -bc_d1_y - bc_d2_y;

		vec4 pack_dx = { bc_d0_x, bc_d1_x, bc_d2_x, 0 };
		pack_dx[3] = dot_prod(pack_dx, depth_vec);
		vec4 pack_dy = { bc_d0_y, bc_d1_y, bc_d2_y, 0 };
		pack_dy[3] = dot_prod(pack_dy, depth_vec);

		vec4 pack_0 { 1, 0, 0, tr[0].z };
		pack_0 = pack_0 + rel_0.x * pack_dx + rel_0.y * pack_dy;

		for (uint32_t y = min_y; y <= max_y; ++y) {
			vec4 pack = pack_0;
			for (uint32_t x = min_x; x <= max_x; ++x) {
				if (pack[0] >= 0 && pack[1] >= 0 &&
				    pack[2] >= 0 && pack[3] >= -1.0f) {
					out.x = x;
					out.pid = pid;
					out.sse_data = pack;
					buf[y].push_back(out);
				}
				pack = pack + pack_dx;
			}
			pack_0 = pack_0 + pack_dy;
		}
#else
		vec3 depth_vec = { tr[0].z, tr[1].z, tr[2].z };
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
