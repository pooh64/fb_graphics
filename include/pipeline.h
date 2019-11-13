#pragma once

#include <include/tile.h>
#include <include/fbuffer.h>
#include <include/geom.h>
#include <include/sync_threadpool.h>

#include <iostream>
#include <vector>
#include <array>
#include <typeinfo>

//#include <include/shader.h>
#include <include/ppm.h>


void Vec3Dump(Vec3 const &v)
{
	std::cout << v.x << " " << v.y << " " << v.z << "\n";
}


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
	virtual void set_view(Mat4 const &view) = 0;
	virtual void set_window(Window const &) = 0;
};


struct ModelShader : public Shader<Vertex, Vertex, Fbuffer::Color> {
	PpmImg const *tex_img;

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

	void set_view(Mat4 const &view) override
	{
		modelview_mat = view;

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

private:
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
};

struct TexHighlShader final: public ModelShader {
public:
	Vec3 light;
	FsOut FShader(FsIn const &in) const override
	{
		Vec3 light_dir = light;
		Vec3 norm = in.norm;
		Vec3 pos = Normalize(in.pos);

		float dot_d = DotProd(light_dir, norm);
		float dot_s = DotProd(light - 2 * dot_d * norm, pos);

		dot_d = std::max(0.0f, dot_d);
		dot_s = std::max(0.0f, dot_s);
		float intens = 0.4f +
			       0.25f * dot_d +
			       0.35f * std::pow(dot_s, 32);

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
float constexpr TrFreeDepth = std::numeric_limits<float>::min();


template <typename _in, typename _data, typename _shader>
struct Setup {
	using In      = _in;
	using Data    = _data;
	using _Shader = _shader;

	_Shader shader; // set it manually

	virtual void Process(In const &, std::vector<Data> &) const = 0;
	virtual void set_window(Window const &) = 0;
};

//spec
template <typename _shader>
struct TrSetup final: public Setup<TrPrim, TrData, _shader> {
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
				break;
			if (i == in.size() - 1)
				out.push_back(data);
		}
	}

	void set_window(Window const &wnd) override
	{
		/* Nothing */
	}
};

template <typename _data>
struct BinRast {
	using Data = _data;
	virtual void Process(Data const &, uint32_t,
		std::vector<std::vector<uint32_t>> &) const = 0;
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
	max_r.x = std::max(std::min(int32_t(max_r.x), int32_t(wnd_max.x)), 0);
	max_r.y = std::max(std::min(int32_t(max_r.y), int32_t(wnd_max.y)), 0);
}

// spec
struct TrBinRast final : public BinRast<TrData> {
	int32_t w_bins, h_bins;
	void set_window(Window const &wnd) override
	{
		w_bins = DivRoundUp(wnd.w, BIN_PIX);
		h_bins = DivRoundUp(wnd.h, BIN_PIX);
	}

	void Process(Data const &data, uint32_t id,
		std::vector<std::vector<uint32_t>> &buf) const override
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
template <typename _data>
struct CoarseRast {
	using Data = _data;
	virtual void Process(Data const &, uint32_t,
			Bin<std::vector<uint32_t>> &, Vec2i const &) const = 0;
	virtual void set_window(Window const &) = 0;
};

// spec
struct TrCoarseRast final : public CoarseRast<TrData> {
	int32_t w_tiles, h_tiles;
	void set_window(Window const &wnd) override
	{
		w_tiles = DivRoundUp(wnd.w, TILE_SIZE);
		h_tiles = DivRoundUp(wnd.h, TILE_SIZE);
	}

	// bin -> crd
	void Process(Data const &data, uint32_t id,
		Bin<std::vector<uint32_t>> &buf, Vec2i const &bin) const override
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
		// Discard tiles
		ClipBounds(min_r, max_r,
				Vec2i{0, 0},
				Vec2i{w_tiles - 1, h_tiles - 1});

		min_r.x = min_r.x % BIN_SIZE;
		min_r.y = min_r.y % BIN_SIZE;

		max_r.x = max_r.x % BIN_SIZE;
		max_r.y = max_r.y % BIN_SIZE;

		for (uint32_t y = min_r.y; y <= max_r.y; ++y) {
			for (uint32_t x = min_r.x; x <= max_r.x; ++x)
				buf[x + y * BIN_SIZE].push_back(id);
		}
	}
};

// tile_n_x, tile_n_y
template <typename _data, typename _fragm>
struct FineRast {
	using Data  = _data;
	using Fragm = _fragm;
	struct Out {
		Fragm fragm;
		uint32_t data_id;
	};
	virtual void Process(Data const &, uint32_t,
			Tile<Out> &, Vec2i const &) const = 0;
	virtual void set_window(Window const &) = 0;
	virtual void ClearBuf(Tile<Out> &) const = 0;
	virtual bool Check(Fragm const &fragm) const = 0;
};

// spec
struct TrFineRast final : public FineRast<TrData, TrFragm> {
	void set_window(Window const &wnd) override
	{

	}
	void Process(Data const &data, uint32_t id,
			Tile<Out> &buf, Vec2i const &crd) const override
	{
		Vec2i min_r, max_r;
		GetTrBounds(data, min_r, max_r);

		ClipBounds(min_r, max_r,
				Vec2i{crd.x * TILE_SIZE, crd.y * TILE_SIZE},
				Vec2i{(crd.x + 1) * TILE_SIZE - 1,
				      (crd.y + 1) * TILE_SIZE - 1});

		Vec3 tr[3] = { ReinterpVec3(data[0].pos),
			       ReinterpVec3(data[1].pos),
			       ReinterpVec3(data[2].pos)};

		Vec3 d1_3 = tr[1] - tr[0];
		Vec3 d2_3 = tr[2] - tr[0];
		Vec2 d1 = { d1_3.x, d1_3.y };
		Vec2 d2 = { d2_3.x, d2_3.y };

		float det = d1.x * d2.y - d1.y * d2.x;
		if (det == 0)
			return;

		Vec2 r0 = Vec2 { tr[0].x, tr[0].y };
		Out out;

		Vec4 depth_vec = { tr[0].z, tr[1].z, tr[2].z, 0 };
		Vec2 rel_0 = Vec2{float(min_r.x), float(min_r.y)} - r0;
		float bc_d1_x =  d2.y / det;
		float bc_d1_y = -d2.x / det;
		float bc_d2_x = -d1.y / det;
		float bc_d2_y =  d1.x / det;
		float bc_d0_x = -bc_d1_x - bc_d2_x;
		float bc_d0_y = -bc_d1_y - bc_d2_y;

		Vec4 pack_dx = { bc_d0_x, bc_d1_x, bc_d2_x};
		pack_dx[3] = -DotProd3(pack_dx, depth_vec);  // tricky
		Vec4 pack_dy = { bc_d0_y, bc_d1_y, bc_d2_y};
		pack_dy[3] = -DotProd3(pack_dy, depth_vec);  // tricky

		Vec4 pack_0 { 1, 0, 0, tr[0].z };
		pack_0 = pack_0 + rel_0.x * pack_dx + rel_0.y * pack_dy;

		min_r.x = min_r.x % TILE_SIZE;
		min_r.y = min_r.y % TILE_SIZE;
		max_r.x = max_r.x % TILE_SIZE;
		max_r.y = max_r.y % TILE_SIZE;
		for (uint32_t y = min_r.y; y <= max_r.y; ++y) {
			Vec4 pack = pack_0;
			for (uint32_t x = min_r.x; x <= max_r.x; ++x) {
				uint32_t pix_ind = x + y * TILE_SIZE;
				float depth = (buf[pix_ind].fragm.depth); // tricky
				if (pack[0] >= 0 && pack[1] >= 0 &&
				    pack[2] >= 0 && pack[3] >= depth) {
					out.fragm.sse_data = pack;
					out.data_id = id;
					buf[x + y * TILE_SIZE] = out;
				}
				pack = pack + pack_dx;
			}
			pack_0 = pack_0 + pack_dy;
		}
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

	void Render(InputBuf const &_inp_buf, Fbuffer::Color *cbuf);
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

	using Data  = typename _Setup::Data;
	using Fragm = typename _FineRast::Out;

	using DataBuf   = std::vector<Data>;
	using BinBuf    = std::vector<std::vector<uint32_t>>;
	using CoarseBuf = Bin<std::vector<uint32_t>>;
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

		for (auto const &data_id : coarse_buf[tile_id]) {
			fine_rast.Process(data_buf[data_id], data_id,
					fine_buf, tile_coord);
		}

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
 _interp>::Render(InputBuf const &inp_buf, Fbuffer::Color *cbuf)
{
	cur_inp_buf = &inp_buf;
	cur_cbuf = cbuf;
	setup.shader = shader;

	pipeline_split_tasks(inp_buf, 32); // bigger chunks for better coherency
	pipeline_execute_tasks(SetupProcessRoutine);

	pipeline_merge_tasks(data_buffs);
	pipeline_execute_tasks(SetupMergeRoutine);

	// accumulated model

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
