#pragma once

//#define HACK_DRAWBIN_NO_DRAWBACK

#include <include/tile.h>
#include <include/fbuffer.h>
#include <include/geom.h>
#include <include/sync_threadpool.h>
#include <include/ppm.h>

#include <iostream>
#include <vector>
#include <array>
#include <typeinfo>

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

template <typename _data, typename _out>
struct BinRast {
	using Data = _data;
	using Out = _out;
	virtual void Process(std::vector<Data> const &, uint32_t,
		std::vector<std::vector<Out>> &) const = 0;
	virtual void set_window(Window const &) = 0;
};

template <typename _data, typename _in, typename _out>
struct CoarseRast {
	using Data = _data;
	using In  = _in;
	using Out = _out;
	virtual void Process(std::vector<Data> const &, In,
			Bin<std::vector<Out>> &, Vec2i const &) const = 0;
	virtual void set_window(Window const &) = 0;
};

template <typename _data, typename _in, typename _fragm>
struct FineRast {
	using Data  = _data;
	using In    = _in;
	using Fragm = _fragm;
	struct Out {
		Fragm fragm;
		uint32_t data_id;
	};
	virtual bool Process(std::vector<Data> const &, In,
			Tile<Out> &, Vec2i const &) const = 0;
	virtual void set_window(Window const &) = 0;
	virtual void ClearBuf(Tile<Out> &) const = 0;
	virtual bool Check(Fragm const &fragm) const = 0;
};

template <typename _data, typename _fragm, typename _out>
struct Interp {
	using Data  = _data;
	using Fragm = _fragm;
	using Out   = _out;
	virtual Out Process(Data const &, Fragm const &) const = 0;
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
	void set_window(Window const &wnd);
	void set_sync_tp(SyncThreadpool *sync_tp_);
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
	SyncThreadpool *sync_tp;

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
      _interp>::set_sync_tp(SyncThreadpool *sync_tp_)
{
	sync_tp = sync_tp_;
	uint32_t n_threads = sync_tp->get_concurrency();

	  data_buffs.resize(n_threads);
	   bin_buffs.resize(n_threads);
	coarse_buffs.resize(n_threads);
	  fine_buffs.resize(n_threads);

	for (auto &buf : bin_buffs)
		buf.resize(w_bins * h_bins);
}

#define pipeline_execute_tasks(_routine)				\
do {									\
	sync_tp->set_tasks(std::bind(&Pipeline::_routine, this,		\
		std::placeholders::_1, std::placeholders::_2),		\
			task_buf.size());				\
	sync_tp->run();							\
	sync_tp->wait_completion();					\
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
		bin_rast.Process(data_buf, i, bin_buf);
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
		for (auto const &out : bin_buf[bin_id]) {
			coarse_rast.Process(data_buf, out,
					coarse_buf, bin_coord);
			++bin_count;
		}
	}
	if (bin_count == 0)
		return;

	_Shader loc_shader = shader;

	for (uint32_t tile_id = 0; tile_id < coarse_buf.size(); ++tile_id) {
		if (coarse_buf[tile_id].size() == 0)
			continue;
		fine_rast.ClearBuf(fine_buf);

		Vec2i tile_coord; // in tiles
		tile_coord.x = bin_coord.x * BIN_SIZE;
		tile_coord.y = bin_coord.y * BIN_SIZE;
		tile_coord.x += tile_id % BIN_SIZE;
		tile_coord.y += (tile_id - tile_id % BIN_SIZE) / BIN_SIZE;

		bool full = false;
		for (auto const &out : coarse_buf[tile_id]) {
			full |= fine_rast.Process(data_buf, out, fine_buf,
					tile_coord);
		}

		Vec2i r0 = {.x = tile_coord.x * TILE_SIZE,
			    .y = tile_coord.y * TILE_SIZE };

		if (full)
			goto shade_full;
		else
			goto shade_default;
shade_full:
		for (int32_t y = 0; y < TILE_SIZE; ++y) {
			for (int32_t x = 0; x < TILE_SIZE; ++x) {
				uint32_t fragm_ind = x + TILE_SIZE * y;
				auto const &fine_out = fine_buf[fragm_ind];
				auto const &fragm = fine_out.fragm;
				auto const &data = data_buf[fine_out.data_id];

				auto inp_out = interp.Process(data, fragm);
				Vec2i r = {.x = r0.x + x, .y = r0.y + y};
				uint32_t cbuf_ind = r.x + r.y * w_pix;
#ifndef HACK_DRAWBIN_NO_DRAWBACK
				cbuf[cbuf_ind] = loc_shader.FShader(inp_out);
#else
				cbuf[0] = loc_shader.FShader(inp_out);
#endif
			}
		}
		continue;

shade_default:
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
#ifndef HACK_DRAWBIN_NO_DRAWBACK
				cbuf[cbuf_ind] = loc_shader.FShader(inp_out);
#else
				cbuf[0] = loc_shader.FShader(inp_out);
#endif
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

	pipeline_split_tasks(inp_buf, 256); // big chunks for better coherency
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

	pipeline_split_tasks(bin_buffs[0], 1);
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
