#pragma once
#include <include/geom.h>
#include <vector>

template<typename _rz_coord, typename _rz_baryc>
struct rasterizer {
protected:
	vec3 min_scr_al;
	vec3 max_scr_al;
public:
	using rz_coord = _rz_coord;
	using rz_baryc = _rz_baryc;

	struct rz_out {
		float depth;
		uint32_t x;
		uint32_t pid;
		rz_baryc bc;
	};

	void set_window(window const &wnd)
	{
		viewport_transform vp_tr;
		vp_tr.set_window(wnd);
		min_scr_al = vp_tr.min_scr;
		max_scr_al = vp_tr.max_scr;
		for (int i = 0; i < 3; ++i) {
			min_scr_al[i] += 0.5f;
			max_scr_al[i] += 0.5f;
		}
	}

	virtual void rasterize(rz_coord const &, uint32_t,
		       std::vector<std::vector<rz_out>> &) const = 0;
};

struct tr_rasterizer final: public rasterizer<vec3[3], float[3]> {
	void rasterize(rz_coord const &tr, uint32_t pid,
		       std::vector<std::vector<rz_out>> &buf) const override
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

		min_r = {std::max(min_r.x, min_scr_al.x),
			 std::max(min_r.y, min_scr_al.y)};

		max_r = {std::min(max_r.x, max_scr_al.x),
			 std::min(max_r.y, max_scr_al.y)};

		vec2 r0 = vec2 { tr[0].x, tr[0].y };
		vec3 depth_vec = { tr[0].z, tr[1].z, tr[2].z };
		rz_out out;

		for (int32_t y = min_r.y; y <= max_r.y; ++y) {
			for (int32_t x = min_r.x; x <= max_r.x; ++x) {
				vec2 r_rel = vec2{float(x), float(y)} - r0;
				float det1 = r_rel.x * d2.y - r_rel.y * d2.x;
				float det2 = d1.x * r_rel.y - d1.y * r_rel.x;
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
	}
};
