#pragma once

#include <include/tile.h>
#include <include/fbuffer.h>
#include <include/geom.h>
#include <include/sync_threadpool.h>

#include <iostream>
#include <vector>
#include <array>
#include <typeinfo>

#include <include/ppm.h>

template <typename _vs_in, typename _fs_in, typename _fs_out>
struct Shader {
	using VsIn  = _vs_in;
	using FsIn  = _fs_in;
	using FsOut = _fs_out;
	struct VsOut {
		Vec4 pos;
		FsIn fs_vtx;
	};
	virtual VsOut VShader(VsIn const &) const = 0;
	virtual FsOut FShader(FsIn const &) const = 0;
	virtual void set_view(Mat4 const &view, float scale) = 0;
	virtual void set_window(Window const &) = 0;
};


struct ModelShader : public Shader<Vertex, Vertex, Fbuffer::Color> {
	void set_window(Window const &wnd) override
	{
		vp_tr.set_window(wnd);

		float fov = 3.1415 / 3;
		float far  = wnd.f;
		float near = wnd.n;

		float size = std::tan(fov / 2) * near;
		float ratio = float(wnd.w) / wnd.h;

		proj_mat = MakeMat4Projection(size * ratio,
			-size * ratio, size, -size, far, near);
	}

	void set_view(Mat4 const &view, float scale) override
	{
		Mat4 scale_mat = MakeMat4Scale(Vec3{scale, scale, scale});
		modelview_mat = view * scale_mat;

		norm_mat = Transpose(Inverse(modelview_mat));

		light = Normalize(ReinterpVec3(norm_mat *
				(Vec4{1, 1, 1, 1})));
	}

	VsOut VShader(VsIn const &in) const override
	{
		VsOut out;
		Vec4 mv_pos = modelview_mat * ToVec4(in.pos);

		out.fs_vtx.pos 	= ToVec3(mv_pos);
		out.pos 	= ToVec4(vp_tr(ToVec3(proj_mat * mv_pos)));
		out.fs_vtx.norm = ToVec3(norm_mat * ToVec4(in.norm));
		out.fs_vtx.tex 	= in.tex;

		return out;
	}

	virtual FsOut FShader(FsIn const &in) const override = 0;

protected:
	ViewportTransform vp_tr;
	Mat4 modelview_mat;
	Mat4 proj_mat;
	Mat4 norm_mat;

	Vec3 light;
};

struct TexShader final: public ModelShader {
public:
	FsOut FShader(FsIn const &in) const override
	{
		int32_t w = tex_img->w;
		int32_t h = tex_img->h;

		int32_t x = (in.tex.x * w) + 0.5f;
		int32_t y = h - (in.tex.y * h) + 0.5f;

		if (x >= w)	x = w - 1;
		if (y >= h)	y = h - 1;
		if (x < 0)	x = 0;
		if (y < 0)	y = 0;

		PpmImg::Color c = tex_img->buf[x + w * y];
		//PpmImg::Color c { 200, 200, 200 };
		return Fbuffer::Color { c.b, c.g, c.r, 255 };
	}
	PpmImg const *tex_img;
};

struct TexHighlShader final: public ModelShader {
public:
	FsOut FShader(FsIn const &in) const override
	{
		Vec3 light_dir = light;
		Vec3 norm = in.norm;
		Vec3 pos = Normalize(in.pos);

		float dot_d = DotProd(light_dir, norm);
		float dot_s = DotProd(light - 2 * dot_d * norm, pos);

		dot_d = std::max(0.0f, dot_d);
		dot_s = std::max(0.0f, dot_s);
		float intens = 0.35f +
			       0.24f * dot_d +
			       0.40f * std::pow(dot_s, 32);

		int32_t w = tex_img->w;
		int32_t h = tex_img->h;

		int32_t x = (in.tex.x * w) + 0.5f;
		int32_t y = h - (in.tex.y * h) + 0.5f;

		if (x >= w)	x = w - 1;
		if (y >= h)	y = h - 1;
		if (x < 0)	x = 0;
		if (y < 0)	y = 0;

		PpmImg::Color c = tex_img->buf[x + w * y];
		return Fbuffer::Color { uint8_t(c.b * intens),
					uint8_t(c.g * intens),
					uint8_t(c.r * intens), 255 };
	}
	PpmImg const *tex_img;
};

// spec
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

float constexpr TrFreeDepth = std::numeric_limits<float>::min();

enum class SetupCullingType {
	NO_CULLING,
	BACK,
	FRONT,
};

template <typename _in, typename _data, typename _shader>
struct Setup
{
	using In      = _in;
	using Data    = _data;
	using _Shader = _shader;

	_Shader shader; // set it manually

	virtual void Process(In const &, std::vector<Data> &) const = 0;
	virtual void set_window(Window const &) = 0;
};

//spec
template <SetupCullingType _cul, typename _shader>
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
			if (data[i].fs_vtx.pos.z <= 0)
				return;
		}

		if (_cul != decltype(_cul)::NO_CULLING) {
			Vec3 tr[3] = { ReinterpVec3(data[0].pos),
				       ReinterpVec3(data[1].pos),
				       ReinterpVec3(data[2].pos)};

			Vec3 d1_3 = tr[1] - tr[0];
			Vec3 d2_3 = tr[2] - tr[0];
			Vec2 d1 = { d1_3.x, d1_3.y };
			Vec2 d2 = { d2_3.x, d2_3.y };

			float det = d1.x * d2.y - d1.y * d2.x;
			if (_cul == decltype(_cul)::BACK) { // wtf
				if (det >= 0)
					return;
			} else if (_cul == decltype(_cul)::FRONT) {
				if (det <= 0)
					return;
				auto tmp = data[0];
				data[0] = data[2];
				data[2] = tmp;
			} else {
				assert("Not tested\n");
				if (det >= 0) {
					auto tmp = data[0];
					data[0] = data[2];
					data[2] = tmp;
				}
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
	public TrSetup<SetupCullingType::NO_CULLING, _shader> {
};

template <typename _shader>
struct TrSetupBackCulling final:
	public TrSetup<SetupCullingType::BACK, _shader> {
};

template <typename _shader>
struct TrSetupFrontCulling final:
	public TrSetup<SetupCullingType::FRONT, _shader> {
};

template <typename _data, typename _out>
struct BinRast {
	using Data = _data;
	using Out = _out;
	virtual void Process(Data const &, uint32_t,
		std::vector<std::vector<Out>> &) const = 0;
	virtual void set_window(Window const &) = 0;
};

//spec
inline void GetTrBounds(TrData const &tr, Vec2i &min_r, Vec2i &max_r)
{
	min_r = {int32_t(std::min(tr[0].pos.x, std::min(tr[1].pos.x, tr[2].pos.x)) + 0.5f),
	         int32_t(std::min(tr[0].pos.y, std::min(tr[1].pos.y, tr[2].pos.y)) + 0.5f)};
	max_r = {int32_t(std::max(tr[0].pos.x, std::max(tr[1].pos.x, tr[2].pos.x)) + 0.5f),
	         int32_t(std::max(tr[0].pos.y, std::max(tr[1].pos.y, tr[2].pos.y)) + 0.5f)};
}

// spec
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

		//printf("(%lg, %lg) (%lg, %lg)\n", v0.x, v0.y, v1.x, v1.y);
		//printf("(%lg, %lg), %lg\n", e.cx, e.cy, e.c0);
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


// spec
struct TrBinRast final : public BinRast<TrData, uint32_t> {
	int32_t w_bins, h_bins;
	void set_window(Window const &wnd) override
	{
		w_bins = DivRoundUp(wnd.w, BIN_PIX);
		h_bins = DivRoundUp(wnd.h, BIN_PIX);
	}

	void Process(Data const &data, uint32_t id,
		std::vector<std::vector<Out>> &buf) const override
	{
		Vec2i min_r, max_r;
		GetTrBounds(data, min_r, max_r); // move in data???

		min_r.x /= BIN_PIX;
		min_r.y /= BIN_PIX;
		max_r.x = DivRoundUp(max_r.x, BIN_PIX);
		max_r.y = DivRoundUp(max_r.y, BIN_PIX);

		ClipBounds(min_r, max_r,
				Vec2i{0, 0},
				Vec2i{w_bins - 1, h_bins - 1});

		for (uint32_t y = min_r.y; y <= max_r.y; ++y) {
			for (uint32_t x = min_r.x; x <= max_r.x; ++x)
				buf[x + y * w_bins].push_back(id);
		}
	}
};

// bin_n_x, bin_n_y
template <typename _data, typename _in, typename _out>
struct CoarseRast {
	using Data = _data;
	using In  = _in;
	using Out = _out;
	virtual void Process(Data const &, In,
			Bin<std::vector<Out>> &, Vec2i const &) const = 0;
	virtual void set_window(Window const &) = 0;
};

// spec
struct TrCoarseRast final : public CoarseRast<TrData, uint32_t, TrOverlapInfo> {
	int32_t w_tiles, h_tiles;
	void set_window(Window const &wnd) override
	{
		w_tiles = DivRoundUp(wnd.w, TILE_SIZE);
		h_tiles = DivRoundUp(wnd.h, TILE_SIZE);
	}

	// bin -> crd
	void Process(Data const &data, In in,
		Bin<std::vector<Out>> &buf, Vec2i const &bin) const override
	{
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
		ClipBounds(min_r, max_r,
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
				Out out = {.id = in, .accepted = false};
				if (eqn.try_reject(vec, rej))
					continue;
				if (eqn.try_accept(vec, acc))
					out.accepted = true;
				buf[x + y * BIN_SIZE].push_back(out);
			}
		}
	}
};

// tile_n_x, tile_n_y
template <typename _data, typename _in, typename _fragm>
struct FineRast {
	using Data  = _data;
	using In    = _in;
	using Fragm = _fragm;
	struct Out {
		Fragm fragm;
		uint32_t data_id;
	};
	virtual void Process(std::vector<Data> const &, In,
			Tile<Out> &, Vec2i const &) const = 0;
	virtual void set_window(Window const &) = 0;
	virtual void ClearBuf(Tile<Out> &) const = 0;
	virtual bool Check(Fragm const &fragm) const = 0;
};

enum class FineRastZbufType {
	DISABLED,
	ACTIVE,
};

// spec
template <FineRastZbufType _zbuf>
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

	void Process(std::vector<TrData> const &data_buf, In in,
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
		if (_zbuf == decltype(_zbuf)::ACTIVE) {
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
				if (_zbuf == decltype(_zbuf)::ACTIVE) {
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
				if (_zbuf == decltype(_zbuf)::ACTIVE) {
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


template <typename _data, typename _fragm, typename _out>
struct Interp {
	using Data  = _data;
	using Fragm = _fragm;
	using Out   = _out;
	virtual Out Process(Data const &, Fragm const &) const = 0;
};

//spec
struct TrInterp final: public Interp<TrData, TrFragm, Vertex> {
	Out Process(Data const &tr, Fragm const &fragm)	const override
	{
		Vertex v;
		v.pos  = Vec3 {0, 0, 0};
		v.tex  = Vec2 {0, 0};
		v.norm = Vec3 {0, 0, 0};
		Vec3 bc_corr {fragm.bc[0], fragm.bc[1], fragm.bc[2]};

		for (int i = 0; i < 3; ++i)
			bc_corr[i] = bc_corr[i] / tr[i].pos.z;

		float mp = 0;
		for (int i = 0; i < 3; ++i)
			mp = mp + bc_corr[i];

		for (int i = 0; i < 3; ++i)
			bc_corr[i] = bc_corr[i] / mp;

		for (int i = 0; i < 3; ++i) {
			v.pos  = v.pos  + bc_corr[i] * tr[i].fs_vtx.pos;
			v.norm = v.norm + bc_corr[i] * tr[i].fs_vtx.norm;
			v.tex  = v.tex  + bc_corr[i] * tr[i].fs_vtx.tex;
		}

		v.norm = Normalize(v.norm);
		return v;
	}
};


template <typename _shader,      template <typename> class _setup,
	  typename _bin_rast,    typename _coarse_rast, typename _fine_rast,
	  typename _interp>
struct Pipeline {
	using _Shader  = _shader;
	using _Setup   = _setup<_Shader>;
	using Input    = typename _Setup::In;
	using InputBuf = std::vector<Input>;

	_Shader shader;

	void Accumulate(InputBuf const &_inp_buf);
	void Render(Fbuffer::Color *cbuf);
	void add_concurrency(uint32_t n_threads);
	void set_window(Window const &wnd);
private:
	uint32_t w_bins = 0;
	uint32_t h_bins = 0;
	uint32_t w_pix  = 0;

	using    _BinRast =    _bin_rast;
	using _CoarseRast = _coarse_rast;
	using   _FineRast =   _fine_rast;
	using     _Interp =      _interp;

	_Setup            setup;
	_BinRast       bin_rast;
	_CoarseRast coarse_rast;
	_FineRast     fine_rast;
	_Interp          interp;

	using Data      = typename _Setup::Data;
	using BinOut    = typename _BinRast::Out;
	using CoarseOut = typename _CoarseRast::Out;
	using Fragm     = typename _FineRast::Out;

	using DataBuf   = std::vector<Data>;
	using BinBuf    = std::vector<std::vector<BinOut>>;
	using CoarseBuf = Bin<std::vector<CoarseOut>>;
	using FineBuf   = Tile<Fragm>;

	std::vector<DataBuf>     data_buffs;
	std::vector<BinBuf>       bin_buffs;
	std::vector<CoarseBuf> coarse_buffs;
	std::vector<FineBuf>     fine_buffs;

	/* Threading & routines */
	SyncThreadpool sync_tp;

	struct Task {
		uint32_t beg;
		uint32_t end;
	};
	std::vector<Task> task_buf;

	InputBuf const *cur_inp_buf;
	Fbuffer::Color *cur_cbuf;

	void SetupProcessRoutine(int thread_id, int task_id);
	void   SetupMergeRoutine(int thread_id, int task_id);
	void      BinRastRoutine(int thread_id, int task_id);
	void      DrawBinRoutine(int thread_id, int task_id);
};

// Internal
template <typename _shader,      template<typename> class _setup,
	  typename _bin_rast,    typename _coarse_rast, typename _fine_rast,
	   typename _interp>
void Pipeline<_shader, _setup, _bin_rast, _coarse_rast, _fine_rast,
      _interp>::set_window(Window const &wnd)
{
	     shader.set_window(wnd);
	      setup.set_window(wnd);
	   bin_rast.set_window(wnd);
	coarse_rast.set_window(wnd);
	  fine_rast.set_window(wnd);

	w_pix  = wnd.w;
	w_bins = DivRoundUp(w_pix, BIN_SIZE * TILE_SIZE);
	h_bins = DivRoundUp(wnd.h, BIN_SIZE * TILE_SIZE);

	for (auto &buf : bin_buffs)
		buf.resize(w_bins * h_bins);
}

template <typename _shader,      template<typename> class _setup,
	  typename _bin_rast,    typename _coarse_rast, typename _fine_rast,
	   typename _interp>
void Pipeline<_shader, _setup, _bin_rast, _coarse_rast, _fine_rast,
      _interp>::add_concurrency(uint32_t n_threads)
{
	sync_tp.add_concurrency(n_threads);
	n_threads = sync_tp.get_concurrency();

	  data_buffs.resize(n_threads);
	   bin_buffs.resize(n_threads);
	coarse_buffs.resize(n_threads);
	  fine_buffs.resize(n_threads);

	for (auto &buf : bin_buffs)
		buf.resize(w_bins * h_bins);
}

#define pipeline_execute_tasks(_routine)				\
do {									\
	sync_tp.set_tasks(std::bind(&Pipeline::_routine, this,		\
		std::placeholders::_1, std::placeholders::_2),		\
			task_buf.size());				\
	sync_tp.run();							\
	sync_tp.wait_completion();					\
	task_buf.clear();						\
} while (0)

#define pipeline_split_tasks(_buffer, _task_size)			\
do {									\
	uint32_t total_size = _buffer.size(), task_size = _task_size;	\
	for (uint32_t offs = 0; offs < total_size; offs += task_size) {	\
		Task task;						\
		task.beg = offs;					\
		if (offs + task_size > total_size)			\
			task.end = task.beg + total_size - offs;	\
		else							\
			task.end = task.beg + task_size;		\
		task_buf.push_back(task);				\
	}								\
} while (0)

/* Bad way to merge, unstable & n-1 threads usage */
#define pipeline_merge_tasks(_buffers)					\
do {									\
	uint32_t offs = 0;						\
	for (uint32_t i = 0; i < _buffers.size(); ++i) {		\
		Task task = {.beg = i, .end = offs };			\
		offs += _buffers[i].size();				\
		if (i != 0)						\
			task_buf.push_back(task);			\
	}								\
	_buffers[0].resize(offs);					\
} while (0)

template <typename _shader,      template<typename> class _setup,
	  typename _bin_rast,    typename _coarse_rast, typename _fine_rast,
	   typename _interp>
void Pipeline<_shader, _setup, _bin_rast, _coarse_rast, _fine_rast,
      _interp>::SetupProcessRoutine(int thread_id, int task_id)
{
	auto task = task_buf[task_id];
	auto &data_buf = data_buffs[thread_id];
	auto const &inp_buf = *cur_inp_buf;

	for (uint32_t i = task.beg; i < task.end; ++i)
		setup.Process(inp_buf[i], data_buf);
}

template <typename _shader,      template<typename> class _setup,
	  typename _bin_rast,    typename _coarse_rast, typename _fine_rast,
	   typename _interp>
void Pipeline<_shader, _setup, _bin_rast, _coarse_rast, _fine_rast,
      _interp>::SetupMergeRoutine(int thread_id, int task_id)
{
	auto task = task_buf[task_id];
	auto &my_buf = data_buffs[task.beg];
	auto &target_buf = data_buffs[0];

	for (uint32_t offs = 0; offs < my_buf.size(); ++offs)
		target_buf[offs + task.end] = my_buf[offs];
}

template <typename _shader,      template<typename> class _setup,
	  typename _bin_rast,    typename _coarse_rast, typename _fine_rast,
	   typename _interp>
void Pipeline<_shader, _setup, _bin_rast, _coarse_rast, _fine_rast,
      _interp>::BinRastRoutine(int thread_id, int task_id)
{
	auto task = task_buf[task_id];
	auto &data_buf = data_buffs[0];
	auto &bin_buf = bin_buffs[thread_id];

	for (uint32_t i = task.beg; i < task.end; ++i)
		bin_rast.Process(data_buf[i], i, bin_buf);
}

template <typename _shader,      template<typename> class _setup,
	  typename _bin_rast,    typename _coarse_rast, typename _fine_rast,
	   typename _interp>
void Pipeline<_shader, _setup, _bin_rast, _coarse_rast, _fine_rast,
      _interp>::DrawBinRoutine(int thread_id, int task_id)
{
	auto task = task_buf[task_id];
	uint32_t bin_id = task.beg;

	auto *cbuf = cur_cbuf;
	auto &data_buf = data_buffs[0];
	auto &coarse_buf = coarse_buffs[thread_id];
	auto   &fine_buf =   fine_buffs[thread_id];

	Vec2i bin_coord; // in bins
	bin_coord.x = bin_id % w_bins;
	bin_coord.y = (bin_id - bin_coord.x) / w_bins;

	uint32_t bin_count = 0;
	for (auto const &bin_buf : bin_buffs) {
		for (auto const &data_id : bin_buf[bin_id]) {
			coarse_rast.Process(data_buf[data_id], data_id,
					coarse_buf, bin_coord);
			++bin_count;
		}
	}
	if (bin_count == 0)
		return;

	for (uint32_t tile_id = 0; tile_id < coarse_buf.size(); ++tile_id) {
		if (coarse_buf[tile_id].size() == 0)
			continue;
		fine_rast.ClearBuf(fine_buf);

		Vec2i tile_coord; // in tiles
		tile_coord.x = bin_coord.x * BIN_SIZE;
		tile_coord.y = bin_coord.y * BIN_SIZE;
		tile_coord.x += tile_id % BIN_SIZE;
		tile_coord.y += (tile_id - tile_id % BIN_SIZE) / BIN_SIZE;

		for (auto const &out : coarse_buf[tile_id])
			fine_rast.Process(data_buf, out, fine_buf, tile_coord);

		Vec2i r0 = {.x = tile_coord.x * TILE_SIZE,
			    .y = tile_coord.y * TILE_SIZE };
		for (int32_t y = 0; y < TILE_SIZE; ++y) {
			for (int32_t x = 0; x < TILE_SIZE; ++x) {
				uint32_t fragm_ind = x + TILE_SIZE * y;

				auto const &fine_out = fine_buf[fragm_ind];
				auto const &fragm = fine_out.fragm;
				if (fine_rast.Check(fragm) == false)
					continue;

				auto const &data = data_buf[fine_out.data_id];
				auto inp_out = interp.Process(data, fragm);
				Vec2i r = {.x = r0.x + x, .y = r0.y + y};
				uint32_t cbuf_ind = r.x + r.y * w_pix;
				cbuf[cbuf_ind] = shader.FShader(inp_out);
			}
		}
	}
	for (auto &tile : coarse_buf)
		tile.clear();
}

template <typename _shader,      template<typename> class _setup,
	  typename _bin_rast,    typename _coarse_rast, typename _fine_rast,
	  typename _interp>
void Pipeline<_shader, _setup, _bin_rast, _coarse_rast, _fine_rast,
 _interp>::Accumulate(InputBuf const &inp_buf)
{
	cur_inp_buf = &inp_buf;
	setup.shader = shader;

	pipeline_split_tasks(inp_buf, 32); // bigger chunks for better coherency
	pipeline_execute_tasks(SetupProcessRoutine);

	pipeline_merge_tasks(data_buffs);
	pipeline_execute_tasks(SetupMergeRoutine);
}


template <typename _shader,      template<typename> class _setup,
	  typename _bin_rast,    typename _coarse_rast, typename _fine_rast,
	  typename _interp>
void Pipeline<_shader, _setup, _bin_rast, _coarse_rast, _fine_rast,
 _interp>::Render(Fbuffer::Color *cbuf)
{
	cur_cbuf = cbuf;

	pipeline_split_tasks(data_buffs[0], 32);
	pipeline_execute_tasks(BinRastRoutine);

	pipeline_split_tasks(bin_buffs[0], 1); // 2 == fun
	pipeline_execute_tasks(DrawBinRoutine);

	for (auto &buf : data_buffs)
		buf.clear();

	for (auto &buf : bin_buffs) {
		for (auto &bin : buf)
			bin.clear();
	}

	for (auto &bin : coarse_buffs) {
		for (auto &tile : bin)
			tile.clear();
	}
}
