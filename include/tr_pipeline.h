#pragma once

//#define HACK_TRINTERP_LINEAR

#include <include/pipeline.h>
#include <include/shaders.h>

using TrPrim = std::array<Vertex, 3>;
using TrData = std::array<ModelShader::VsOut, 3>;

struct TrFragm {
	union {
		struct {
			float bc[3];
			float depth;
		};
		Vec4 sse_data;
	};
};

struct TrOverlapInfo {
	uint32_t id;
	bool accepted;
};

enum class TrSetupCullingType {
	NO_CULLING,
	BACK,
	FRONT,
};

enum class TrFineRastZbufType {
	DISABLED,
	ACTIVE,
};

enum class TrInterpType {
	ALL,
	TEXTURE,
	POS,
};

float constexpr TrFreeDepth = std::numeric_limits<float>::min();

template <TrSetupCullingType _type, typename _shader>
struct TrSetup : public Setup<TrPrim, TrData, _shader> {
	using Base = Setup<TrPrim, TrData, _shader>;
	using In      = typename Base::In;
	using Data    = typename Base::Data;
	using _Shader = typename Base::_Shader;
	void Process(In const &in, std::vector<Data> &out) const override
	{
		Data data;
		for (int i = 0; i < in.size(); ++i) {
			data[i] = Base::shader.VShader(in[i]);
			if (data[i].pos.w >= 0)
				return;
		}
		Vec3 tr[3] = { ReinterpVec3(data[0].pos),
			       ReinterpVec3(data[1].pos),
			       ReinterpVec3(data[2].pos)};

		Vec3 d1_3 = tr[1] - tr[0];
		Vec3 d2_3 = tr[2] - tr[0];
		Vec2 d1 = { d1_3.x, d1_3.y };
		Vec2 d2 = { d2_3.x, d2_3.y };

		float det = d1.x * d2.y - d1.y * d2.x;
		if (_type == decltype(_type)::BACK) {
			if (det >= 0)
				return;
		} else if (_type == decltype(_type)::FRONT) {
			if (det <= 0)
				return;
			auto tmp = data[0];
			data[0] = data[2];
			data[2] = tmp;
		} else if (_type == decltype(_type)::NO_CULLING) {
			if (det >= 0) {
				auto tmp = data[0];
				data[0] = data[2];
				data[2] = tmp;
			}
		}
		out.push_back(data);
	}

	void set_window(Window const &wnd) override
	{
		/* Nothing */
	}
};

template <typename _shader>
struct TrSetupNoCulling final:
	public TrSetup<TrSetupCullingType::NO_CULLING, _shader> {
};

template <typename _shader>
struct TrSetupBackCulling final:
	public TrSetup<TrSetupCullingType::BACK, _shader> {
};

template <typename _shader>
struct TrSetupFrontCulling final:
	public TrSetup<TrSetupCullingType::FRONT, _shader> {
};

inline void GetTrBounds(TrData const &tr, Vec2i &min_r, Vec2i &max_r)
{
	min_r = {int32_t(std::min(tr[0].pos.x, std::min(tr[1].pos.x, tr[2].pos.x)) + 0.5f),
	         int32_t(std::min(tr[0].pos.y, std::min(tr[1].pos.y, tr[2].pos.y)) + 0.5f)};
	max_r = {int32_t(std::max(tr[0].pos.x, std::max(tr[1].pos.x, tr[2].pos.x)) + 0.5f),
	         int32_t(std::max(tr[0].pos.y, std::max(tr[1].pos.y, tr[2].pos.y)) + 0.5f)};
}

inline void ClipBounds(Vec2i &min_r, Vec2i &max_r,
	Vec2i const &wnd_min, Vec2i const &wnd_max)
{
	min_r.x = std::max(int32_t(min_r.x), int32_t(wnd_min.x));
	min_r.y = std::max(int32_t(min_r.y), int32_t(wnd_min.y));
	max_r.x = std::max(std:: min(int32_t(max_r.x), int32_t(wnd_max.x)), 0);
	max_r.y = std::max(std::min(int32_t(max_r.y), int32_t(wnd_max.y)), 0);
}

struct TrEdgeEqn {
	struct Edge {
		float cx, cy, c0;
		float rej_mult;
	} edge[3];

	/* Evaluate edge-func in left-bottom corner */

	void set(Vec3 const (&v)[3])
	{
		set_edge(edge[0], v[0], v[1]);
		set_edge(edge[1], v[1], v[2]);
		set_edge(edge[2], v[2], v[0]);
	}

  	void get_reject(float chunk_sz, float (&arr)[3]) const
	{
		for (int i = 0; i < 3; ++i) {
			Edge const &e = edge[i];
			float mult = (e.cx + e.cy + e.rej_mult);
			arr[i] = e.c0 + mult * (chunk_sz / 2);
		}
	}

	bool try_reject(Vec2i const &v, float const (&arr)[3]) const
	{
		for (int i = 0; i < 3; ++i) {
			Edge const &e = edge[i];
			float val = e.cx * float(v.x)
				+ e.cy * float(v.y) + arr[i]; // 0.5?

			if (val >= 0)
				return true;
		}
		return false;
	}

  	void get_accept(float chunk_sz, float (&arr)[3]) const
	{
		for (int i = 0; i < 3; ++i) {
			Edge const &e = edge[i];
			float mult = (e.cx + e.cy - e.rej_mult);
			arr[i] = e.c0 + mult * (chunk_sz / 2);
		}
	}

	bool try_accept(Vec2i const &v, float const (&arr)[3]) const
	{
		for (int i = 0; i < 3; ++i) {
			Edge const &e = edge[i];
			float val = e.cx * float(v.x)
				+ e.cy * float(v.y) + arr[i]; // 0.5?

			if (val >= 0)
				return false;
		}
		return true;
	}
private:
	void set_edge(Edge &e, Vec3 const &v0, Vec3 const &v1)
	{
		e.cx = v0.y - v1.y;
		e.cy = v1.x - v0.x;
		e.c0 = v0.x * v1.y - v0.y * v1.x;
		upd_rej_mult(e);
	}

	void upd_rej_mult(Edge &e) // from chunk center
	{
		if (e.cx < 0) // right
			e.rej_mult  =  e.cx;
		else
			e.rej_mult  = -e.cx;
		if (e.cy < 0) // up
			e.rej_mult +=  e.cy;
		else
			e.rej_mult += -e.cy;
	}
};

struct TrBinRast final : public BinRast<TrData, TrOverlapInfo> {
	int32_t w_bins, h_bins;
	void set_window(Window const &wnd) override
	{
		w_bins = DivRoundUp(wnd.w, BIN_PIX);
		h_bins = DivRoundUp(wnd.h, BIN_PIX);
	}

	void Process(std::vector<Data> const &data_buf, uint32_t id,
		std::vector<std::vector<Out>> &buf) const override
	{
		auto const &data = data_buf[id];
		Vec2i min_r, max_r;
		GetTrBounds(data, min_r, max_r); // move in data???

		min_r.x /= BIN_PIX;
		min_r.y /= BIN_PIX;
		max_r.x = DivRoundUp(max_r.x, BIN_PIX);
		max_r.y = DivRoundUp(max_r.y, BIN_PIX);

		ClipBounds(min_r, max_r,	// Screen culling
				Vec2i{0, 0},
				Vec2i{w_bins - 1, h_bins - 1});

		Vec3 tr_vec[3] = { ReinterpVec3(data[0].pos),
				   ReinterpVec3(data[1].pos),
				   ReinterpVec3(data[2].pos) };
		TrEdgeEqn eqn;
		float rej[3];
		float acc[3];

		eqn.set(tr_vec);
		eqn.get_reject(float(BIN_PIX), rej);
		eqn.get_accept(float(BIN_PIX), acc);

		Out out = {.id = id};
		for (uint32_t y = min_r.y; y <= max_r.y; ++y) {
			for (uint32_t x = min_r.x; x <= max_r.x; ++x) {
				Vec2i vec {int32_t(x * BIN_PIX),
					   int32_t(y * BIN_PIX)};
				if (eqn.try_reject(vec, rej))
					continue;
				out.accepted = eqn.try_accept(vec, acc);
				buf[x + y * w_bins].push_back(out);
			}
		}
	}
};

struct TrCoarseRast final : public CoarseRast<TrData, TrOverlapInfo, TrOverlapInfo> {
	int32_t w_tiles, h_tiles;
	void set_window(Window const &wnd) override
	{
		w_tiles = DivRoundUp(wnd.w, TILE_SIZE);
		h_tiles = DivRoundUp(wnd.h, TILE_SIZE);
	}

	void Process(std::vector<Data> const &data_buf, In in,
		Bin<std::vector<Out>> &buf, Vec2i const &bin) const override
	{
		if (in.accepted)
			ProcessAccepted(in, buf, bin);
		else
			ProcessOverlapped(data_buf, in, buf, bin);
	}

private:
	void ProcessAccepted(In in, Bin<std::vector<Out>> &buf, Vec2i const &bin) const
	{
		Vec2i min_r {bin.x * BIN_SIZE,
			     bin.y * BIN_SIZE};
		Vec2i max_r {(bin.x + 1) * BIN_SIZE - 1,
			     (bin.y + 1) * BIN_SIZE - 1};

		if (min_r.x > (w_tiles - 1) || min_r.y > (h_tiles - 1))
			return; // discard

		max_r.x = std::min(max_r.x, w_tiles - 1);
		max_r.y = std::min(max_r.y, h_tiles - 1);

		max_r.x = max_r.x % BIN_SIZE;
		max_r.y = max_r.y % BIN_SIZE;

		for (uint32_t y = 0; y <= max_r.y; ++y) {
			for (uint32_t x = 0; x <= max_r.x; ++x)
				buf[x + y * BIN_SIZE].push_back(in);
		}
	}

	// bin -> crd
	void ProcessOverlapped(std::vector<Data> const &data_buf, In in,
		Bin<std::vector<Out>> &buf, Vec2i const &bin) const
	{
		auto const &data = data_buf[in.id];
		Vec2i min_r, max_r;
		GetTrBounds(data, min_r, max_r); // move in data???

		min_r.x /= TILE_SIZE;
		min_r.y /= TILE_SIZE;
		max_r.x = DivRoundUp(max_r.x, TILE_SIZE);
		max_r.y = DivRoundUp(max_r.y, TILE_SIZE);

		ClipBounds(min_r, max_r,
				Vec2i{bin.x * BIN_SIZE, bin.y * BIN_SIZE},
				Vec2i{(bin.x + 1) * BIN_SIZE - 1,
				      (bin.y + 1) * BIN_SIZE - 1});
		ClipBounds(min_r, max_r,	// Screen culling
				Vec2i{0, 0},
				Vec2i{w_tiles - 1, h_tiles - 1});

		min_r.x = min_r.x % BIN_SIZE;
		min_r.y = min_r.y % BIN_SIZE;
		max_r.x = max_r.x % BIN_SIZE;
		max_r.y = max_r.y % BIN_SIZE;

		Vec3 tr_vec[3] = { ReinterpVec3(data[0].pos),
				   ReinterpVec3(data[1].pos),
				   ReinterpVec3(data[2].pos) };
		TrEdgeEqn eqn;
		float rej[3];
		float acc[3];

		eqn.set(tr_vec);
		eqn.get_reject(float(TILE_SIZE), rej);
		eqn.get_accept(float(TILE_SIZE), acc);

		for (uint32_t y = min_r.y; y <= max_r.y; ++y) {
			for (uint32_t x = min_r.x; x <= max_r.x; ++x) {
				Vec2i vec {(bin.x * BIN_SIZE + int32_t(x))
					   * TILE_SIZE,
					   (bin.y * BIN_SIZE + int32_t(y))
				           * TILE_SIZE};
				if (eqn.try_reject(vec, rej))
					continue;
				in.accepted = eqn.try_accept(vec, acc);
				buf[x + y * BIN_SIZE].push_back(in);
			}
		}
	}
};

template <TrFineRastZbufType _type>
struct TrFineRast : public FineRast<TrData, TrOverlapInfo, TrFragm> {
	void set_window(Window const &wnd) override
	{

	}

	void ClearBuf(Tile<Out> &buf) const override
	{
		for (auto &out : buf)
			out.fragm.depth = TrFreeDepth;
	}

	bool Check(Fragm const &fragm) const override
	{
		return fragm.depth != TrFreeDepth;
	}

	bool Process(std::vector<TrData> const &data_buf, In in,
			Tile<Out> &buf, Vec2i const &crd) const override
	{
		auto const &data = data_buf[in.id];
		Vec3 tr[3] = { ReinterpVec3(data[0].pos),
			       ReinterpVec3(data[1].pos),
			       ReinterpVec3(data[2].pos)};

		Vec3 d1_3 = tr[1] - tr[0];
		Vec3 d2_3 = tr[2] - tr[0];
		Vec2 d1 = { d1_3.x, d1_3.y };
		Vec2 d2 = { d2_3.x, d2_3.y };

		Vec2 r0 = Vec2 { tr[0].x, tr[0].y };
		float det = d1.x * d2.y - d1.y * d2.x;

		float bc_d1_x =  d2.y / det;
		float bc_d1_y = -d2.x / det;
		float bc_d2_x = -d1.y / det;
		float bc_d2_y =  d1.x / det;
		float bc_d0_x = -bc_d1_x - bc_d2_x;
		float bc_d0_y = -bc_d1_y - bc_d2_y;

		Vec4 pack_dx = { bc_d0_x, bc_d1_x, bc_d2_x};
		Vec4 pack_dy = { bc_d0_y, bc_d1_y, bc_d2_y};
		if (_type == decltype(_type)::ACTIVE) {
			Vec4 depth_vec = { tr[0].z, tr[1].z, tr[2].z, 0 };
			pack_dx[3] = DotProd3(pack_dx, depth_vec);
			pack_dy[3] = DotProd3(pack_dy, depth_vec);
		}

		if (in.accepted == true) {
			Vec2 rel_0 = Vec2{float(crd.x * TILE_SIZE),
					  float(crd.y * TILE_SIZE)} - r0;
			Vec4 pack_0 { 1, 0, 0, tr[0].z };
			pack_0 = pack_0 + rel_0.x * pack_dx + rel_0.y * pack_dy;
			ProcessAccepted(pack_0, pack_dx, pack_dy, buf, in.id);
			return true;
		}

		Vec2i min_r, max_r;
		GetTrBounds(data, min_r, max_r);

		ClipBounds(min_r, max_r,
				Vec2i{crd.x * TILE_SIZE, crd.y * TILE_SIZE},
				Vec2i{(crd.x + 1) * TILE_SIZE - 1,
				      (crd.y + 1) * TILE_SIZE - 1});

		Vec2 rel_0 = Vec2{float(min_r.x), float(min_r.y)} - r0;
		Vec4 pack_0 { 1, 0, 0, tr[0].z };
		pack_0 = pack_0 + rel_0.x * pack_dx + rel_0.y * pack_dy;

		min_r.x = min_r.x % TILE_SIZE;
		min_r.y = min_r.y % TILE_SIZE;
		max_r.x = max_r.x % TILE_SIZE;
		max_r.y = max_r.y % TILE_SIZE;

		ProcessOverlapped(min_r, max_r, pack_0,
				pack_dx, pack_dy, buf, in.id);
		return false;
	}

private:
	inline void ProcessOverlapped(Vec2i const &min_r, Vec2i const &max_r,
		Vec4 pack_0, Vec4 const &pack_dx, Vec4 const &pack_dy,
		Tile<Out> &buf, uint32_t id) const
	{
#define _process_zbuf					\
do {							\
	float depth = (buf[pix_ind].fragm.depth);	\
	if (pack[0] >= 0 && pack[1] >= 0 &&		\
	    pack[2] >= 0 && pack[3] >= depth) {		\
		out.fragm.sse_data = pack;		\
		out.data_id = id;			\
		buf[pix_ind] = out;			\
	}						\
} while (0)

#define _process_no_zbuf				\
do {							\
	if (pack[0] >= 0 && pack[1] >= 0 &&		\
	    pack[2] >= 0 && pack[3] >=			\
	    std::numeric_limits<float>::min()) {	\
		out.fragm.sse_data = pack;		\
		out.data_id = id;			\
		buf[pix_ind] = out;			\
	}						\
} while (0)
		Out out;
		for (uint32_t y = min_r.y; y <= max_r.y; ++y) {
			Vec4 pack = pack_0;
			for (uint32_t x = min_r.x; x <= max_r.x; ++x) {
				uint32_t pix_ind = x + y * TILE_SIZE;
				if (_type == decltype(_type)::ACTIVE) {
					_process_zbuf;
				} else {
					_process_no_zbuf;
				}
				pack = pack + pack_dx;
			}
			pack_0 = pack_0 + pack_dy;
		}
#undef _process_zbuf
#undef _process_no_zbuf
	}

	inline void ProcessAccepted(Vec4 pack_0, Vec4 const &pack_dx,
		Vec4 const &pack_dy, Tile<Out> &buf, uint32_t id) const
	{
#define _process_zbuf					\
do {							\
	float depth = (buf[pix_ind].fragm.depth);	\
	if (pack[3] >= depth) {				\
		out.fragm.sse_data = pack;		\
		out.data_id = id;			\
		buf[pix_ind] = out;			\
	}						\
} while (0)

#define _process_no_zbuf		\
do {					\
	out.fragm.sse_data = pack;	\
	out.data_id = id;		\
	buf[pix_ind] = out;		\
} while (0)
		Out out;
		for (uint32_t y = 0; y < TILE_SIZE; ++y) {
			Vec4 pack = pack_0;
			for (uint32_t x = 0; x < TILE_SIZE; ++x) {
				uint32_t pix_ind = x + y * TILE_SIZE; // just inc
				if (_type == decltype(_type)::ACTIVE) {
					_process_zbuf;
				} else {
					_process_no_zbuf;
				}
				pack = pack + pack_dx;
			}
			pack_0 = pack_0 + pack_dy;
		}
#undef _process_zbuf
#undef _process_no_zbuf
	}
};

template<TrInterpType _type>
struct TrInterp: public Interp<TrData, TrFragm, Vertex> {
	Out Process(Data const &tr, Fragm const &fragm)	const override
	{
		Vertex v;
		if (_type == decltype(_type)::ALL) {
			v.pos  = Vec3 {0, 0, 0};
			v.norm = Vec3 {0, 0, 0};
			v.tex  = Vec2 {0, 0};
		}
		if (_type == decltype(_type)::TEXTURE) {
			v.tex  = Vec2 {0, 0};
		}
		if (_type == decltype(_type)::POS) {
			v.pos  = Vec3 {0, 0, 0};
		}
		Vec3 bc_corr {fragm.bc[0], fragm.bc[1], fragm.bc[2]};

#ifndef HACK_TRINTERP_LINEAR
		for (int i = 0; i < 3; ++i)
			bc_corr[i] = bc_corr[i] / tr[i].pos.z;

		float mp = 0;
		for (int i = 0; i < 3; ++i)
			mp = mp + bc_corr[i];

		for (int i = 0; i < 3; ++i)
			bc_corr[i] = bc_corr[i] / mp;
#endif

		for (int i = 0; i < 3; ++i) {
			if (_type == decltype(_type)::ALL) {
				v.pos  = v.pos  + bc_corr[i] * tr[i].fs_vtx.pos;
				v.norm = v.norm + bc_corr[i] * tr[i].fs_vtx.norm;
				v.tex  = v.tex  + bc_corr[i] * tr[i].fs_vtx.tex;
			}
			if (_type == decltype(_type)::POS) {
				v.pos  = v.pos  + bc_corr[i] * tr[i].fs_vtx.pos;
			}
			if (_type == decltype(_type)::TEXTURE) {
				v.tex  = v.tex  + bc_corr[i] * tr[i].fs_vtx.tex;
			}
		}

		if (_type == decltype(_type)::ALL)
			v.norm = Normalize(v.norm);
		return v;
	}
};
