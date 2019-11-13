#pragma once

#include <include/pipeline.h>

using TrPrim = std::array<Vertex, 3>;

struct TrFragm {
	union {
		struct {
			float bc[3];
			float depth;
		};
		Vec4 sse_data;
	};
	uint32_t data_id;
};


#if 0

///// Old code

struct TrInterpolator final: public Interpolator<Vertex[3], float[3], Vertex> {
	intr_out Interpolate(intr_prim const &tr, intr_in const &bc)
		const override
	{
		Vertex v;
		v.pos  = Vec3 {0, 0, 0};
		v.tex  = Vec2 {0, 0};
		v.norm = Vec3 {0, 0, 0};
		Vec3 bc_corr {bc[0], bc[1], bc[2]};

		for (int i = 0; i < 3; ++i)
			bc_corr[i] = bc_corr[i] / tr[i].pos.z;

		float mp = 0;
		for (int i = 0; i < 3; ++i)
			mp = mp + bc_corr[i];

		for (int i = 0; i < 3; ++i)
			bc_corr[i] = bc_corr[i] / mp;


		for (int i = 0; i < 3; ++i) {
			v.pos  = v.pos  + bc_corr[i] * tr[i].pos;
			v.norm = v.norm + bc_corr[i] * tr[i].norm;
			v.tex  = v.tex  + bc_corr[i] * tr[i].tex;
		}

		v.norm = Normalize(v.norm);
		return v;
	}
};

template<typename _rz_in, typename _fragm>
struct Rasterizer {
protected:
	uint32_t wnd_min_x, wnd_max_x, wnd_min_y, wnd_max_y;
	TileTransform tl_tr;
public:
	using rz_in  = _rz_in;
	using fragm  = _fragm;
	struct rz_out {
		fragm fg;
		uint32_t offs;
	};

	void set_Window(Window const &wnd)
	{
		tl_tr.w_tiles = ToTilesUp(wnd.w);
		wnd_min_x = wnd.x;
		wnd_min_y = wnd.y;
		wnd_max_x = wnd.x + wnd.w - 1;
		wnd_max_y = wnd.y + wnd.h - 1;
	}

	virtual void rasterize(rz_in const &, uint32_t, uint32_t,
			       std::vector<std::vector<rz_out>> &) const = 0;
};

#include <include/tr_prim.h>

struct TrRasterizer final: public Rasterizer<Vec3[3], TrFragment> {
public:

	void rasterize(rz_in const &tr, uint32_t prim_id, uint32_t model_id,
		       std::vector<std::vector<rz_out>> &buf) const override
	{
	 	Vec3 min_r{std::min(tr[0].x, std::min(tr[1].x, tr[2].x)),
			   std::min(tr[0].y, std::min(tr[1].y, tr[2].y)),
			   std::min(tr[0].z, std::min(tr[1].z, tr[2].z))};
		Vec3 max_r{std::max(tr[0].x, std::max(tr[1].x, tr[2].x)),
			   std::max(tr[0].y, std::max(tr[1].y, tr[2].y)),
			   std::max(tr[0].z, std::max(tr[1].z, tr[2].z))};

		uint32_t min_x = std::max(int32_t(min_r.x), int32_t(wnd_min_x));
		uint32_t min_y = std::max(int32_t(min_r.y), int32_t(wnd_min_y));
		uint32_t max_x = std::max(std::min(int32_t(max_r.x), int32_t(wnd_max_x)), 0);
		uint32_t max_y = std::max(std::min(int32_t(max_r.y), int32_t(wnd_max_y)), 0);

		Vec3 d1_3 = tr[1] - tr[0];
		Vec3 d2_3 = tr[2] - tr[0];
		Vec2 d1 = { d1_3.x, d1_3.y };
		Vec2 d2 = { d2_3.x, d2_3.y };

		float det = d1.x * d2.y - d1.y * d2.x;
		if (det == 0)
			return;

		Vec2 r0 = Vec2 { tr[0].x, tr[0].y };
		rz_out out;

#if 1
		Vec4 depth_vec = { tr[0].z, tr[1].z, tr[2].z, 0 };
		Vec2 rel_0 = Vec2{float(min_x), float(min_y)} - r0;
		float bc_d1_x =  d2.y / det;
		float bc_d1_y = -d2.x / det;
		float bc_d2_x = -d1.y / det;
		float bc_d2_y =  d1.x / det;
		float bc_d0_x = -bc_d1_x - bc_d2_x;
		float bc_d0_y = -bc_d1_y - bc_d2_y;

		Vec4 pack_dx = { bc_d0_x, bc_d1_x, bc_d2_x}; //0
		pack_dx[3] = DotProd3(pack_dx, depth_vec);
		Vec4 pack_dy = { bc_d0_y, bc_d1_y, bc_d2_y}; //0
		pack_dy[3] = DotProd3(pack_dy, depth_vec);

		Vec4 pack_0 { 1, 0, 0, tr[0].z };
		pack_0 = pack_0 + rel_0.x * pack_dx + rel_0.y * pack_dy;

		for (uint32_t y = min_y; y <= max_y; ++y) {
			Vec4 pack = pack_0;
			for (uint32_t x = min_x; x <= max_x; ++x) {
				if (pack[0] >= 0 && pack[1] >= 0 &&
				    pack[2] >= 0 && pack[3] >= 0) {
					out.fg.sse_data = pack;
					out.fg.prim_id = prim_id;
					out.fg.model_id = model_id;
					uint32_t tile;
					tl_tr.ToTile(x, y, tile, out.offs);
					buf[tile].push_back(out);
				}
				pack = pack + pack_dx;
			}
			pack_0 = pack_0 + pack_dy;
		}
#else
		assert(!"Implement tiles");
		Vec3 depth_vec = { tr[0].z, tr[1].z, tr[2].z };
		for (uint32_t y = min_y; y <= max_y; ++y) {
			for (uint32_t x = min_x; x <= max_x; ++x) {
				Vec2 rel = Vec2{float(x), float(y)} - r0;
				float det1 = rel.x * d2.y - rel.y * d2.x;
				float det2 = d1.x * rel.y - d1.y * rel.x;
				out.fg.bc[0] = (det - det1 - det2) / det;
				out.fg.bc[1] = det1 / det;
				out.fg.bc[2] = det2 / det;
				if (out.fg.bc[0] < 0 || out.fg.bc[1] < 0 ||
				     out.fg.bc[2] < 0)
					continue;
				out.fg.depth = DotProd((reinterpret_cast<Vec3&>
						(out.fg.bc)), depth_vec);
				out.x = x;
				out.fg.prim_id = prim_id;
				out.fg.model_id = model_id;
				buf[y].push_back(out);
			}
		}
#endif
	}
};

#endif
