#pragma once

#include <include/tile.h>
#include <include/fbuffer.h>
#include <include/geom.h>
#include <include/sync_threadpool.h>

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

template <typename _in, typename _data>
struct Setup {
	using In   = _in;
	using Data = _data;
	virtual Data Process(Data const &) const = 0;
	virtual void set_window(Window const &) = 0;
};

template <typename _data>
struct BinRast {
	using Data = _data;
	virtual void Process(Data const &, uint32_t,
		std::vector<std::vector<uint32_t>> &) const = 0;
	virtual void set_window(Window const &) = 0;
};

// bin_n_x, bin_n_y
template <typename _data>
struct CoarseRast {
	using Data = _data;
	virtual void Process(Data const &, uint32_t,
			Bin<std::vector<uint32_t>> &, Vec2i &) const = 0;
	virtual void set_window(Window const &) = 0;
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
			Tile<Out> &, Vec2i &) const = 0;
	virtual void set_window(Window const &) = 0;
};

template <typename _fragm>
struct FragmCheck {
	using Fragm = _fragm;
	virtual bool Check(Fragm const &, Vec2i const &) const = 0;
	virtual void set_window(Window const &wnd) = 0;
};

template <typename _data, typename _fragm, typename _out>
struct Interp {
	using Data = _data;
	using In   = _fragm;
	using Out  = _out;
	virtual Out Process(Data const &, In const &) const = 0;
};


template <typename _shader,      typename _setup,
	  typename _bin_rast,    typename _coarse_rast, typename _fine_rast,
	  typename _fragm_check, typename _interp>
struct Pipeline {
	using _Setup   = _setup;
	using _Shader  = _shader;
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
	using _FragmCheck = _fragm_check;
	using     _Interp =      _interp;

	_Setup            setup;
	_BinRast       bin_rast;
	_CoarseRast coarse_rast;
	_FineRast     fine_rast;
	_FragmCheck fragm_check;
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

	InputBuf       *cur_inp_buf;
	Fbuffer::Color *cur_cbuf;

	void SetupProcessRoutine(int thread_id, int task_id);
	void   SetupMergeRoutine(int thread_id, int task_id);
	void      BinRastRoutine(int thread_id, int task_id);
	void      DrawBinRoutine(int thread_id, int task_id);
};

template <typename _shader,      typename _setup,
	  typename _bin_rast,    typename _coarse_rast, typename _fine_rast,
	  typename _fragm_check, typename _interp>
void Pipeline<_shader, _setup, _bin_rast, _coarse_rast, _fine_rast,
     _fragm_check, _interp>::set_window(Window const &wnd)
{
	      setup.set_window(wnd);
	   bin_rast.set_window(wnd);
	coarse_rast.set_window(wnd);
	  fine_rast.set_window(wnd);
	fragm_check.set_window(wnd);

	w_pix  = wnd.w;
	w_bins = DivRoundUp(w_pix, BIN_SIZE * TILE_SIZE);
	h_bins = DivRoundUp(wnd.h, BIN_SIZE * TILE_SIZE);

	for (auto &buf : bin_buffs)
		buf.resize(w_bins * h_bins);
}

template <typename _shader,      typename _setup,
	  typename _bin_rast,    typename _coarse_rast, typename _fine_rast,
	  typename _fragm_check, typename _interp>
void Pipeline<_shader, _setup, _bin_rast, _coarse_rast, _fine_rast,
     _fragm_check, _interp>::add_concurrency(uint32_t n_threads)
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

template <typename _shader,      typename _setup,
	  typename _bin_rast,    typename _coarse_rast, typename _fine_rast,
	  typename _fragm_check, typename _interp>
void Pipeline<_shader, _setup, _bin_rast, _coarse_rast, _fine_rast,
     _fragm_check, _interp>::SetupProcessRoutine(int thread_id, int task_id)
{
	auto task = task_buf[task_id];
	auto &data_buf = data_buffs[thread_id];
	auto const &inp_buf = *cur_inp_buf;

	for (uint32_t i = task.beg; i < task.end; ++i)
		data_buf.push_back(setup.Process(inp_buf[i]));
}

template <typename _shader,      typename _setup,
	  typename _bin_rast,    typename _coarse_rast, typename _fine_rast,
	  typename _fragm_check, typename _interp>
void Pipeline<_shader, _setup, _bin_rast, _coarse_rast, _fine_rast,
     _fragm_check, _interp>::SetupMergeRoutine(int thread_id, int task_id)
{
	auto task = task_buf[task_id];
	auto &my_buf = data_buffs[task.beg];
	auto &target_buf = data_buffs[0];

	for (uint32_t offs = 0; offs < my_buf.size(); ++offs)
		target_buf[offs + task.end] = my_buf[offs];
}

template <typename _shader,      typename _setup,
	  typename _bin_rast,    typename _coarse_rast, typename _fine_rast,
	  typename _fragm_check, typename _interp>
void Pipeline<_shader, _setup, _bin_rast, _coarse_rast, _fine_rast,
     _fragm_check, _interp>::BinRastRoutine(int thread_id, int task_id)
{
	auto task = task_buf[task_id];
	auto &data_buf = data_buffs[0];
	auto &bin_buf = bin_buffs[thread_id];

	for (uint32_t i = task.beg; i < task.end; ++i)
		bin_rast.Process(data_buf[i], i, bin_buf);
}

template <typename _shader,      typename _setup,
	  typename _bin_rast,    typename _coarse_rast, typename _fine_rast,
	  typename _fragm_check, typename _interp>
void Pipeline<_shader, _setup, _bin_rast, _coarse_rast, _fine_rast,
     _fragm_check, _interp>::DrawBinRoutine(int thread_id, int task_id)
{
	auto task = task_buf[task_id];
	uint32_t bin_id = task.beg;

	auto *cbuf = cur_cbuf;
	auto &data_buf = data_buffs[0];
	auto &coarse_buf = coarse_buffs[thread_id];
	auto   &fine_buf =   fine_buffs[thread_id];

	Vec2i bin_coord; // measured in bins
	bin_coord.x = bin_id % w_bins;
	bin_coord.y = (bin_id - bin_coord.x) / w_bins;

	for (auto const &bin_buf : bin_buffs) {
		for (auto const &data_id : bin_buf[bin_id]) {
			coarse_rast.Process(data_buf[data_id], data_id,
					coarse_buf, bin_coord);
		}
	}

	for (uint32_t tile_id = 0; tile_id < coarse_buf.size(); ++tile_id) {
		Vec2i tile_coord; // in tiles
		tile_coord.x = bin_coord.x * BIN_SIZE;
		tile_coord.y = bin_coord.x * BIN_SIZE;
		tile_coord.x += tile_id % BIN_SIZE;
		tile_coord.y += (tile_id - tile_id % BIN_SIZE) / BIN_SIZE;

		for (auto const &data_id : coarse_buf[tile_id]) {
			fine_rast.Process(data_buf[data_id], data_id,
					fine_buf, tile_coord);
		}

		Vec2i r0 = {.x = tile_coord.x * TILE_SIZE,
			    .y = tile_coord.y * TILE_SIZE };
		for (uint32_t x = 0; x < TILE_SIZE; ++x) {
			for (uint32_t y = 0; y < TILE_SIZE; ++y) {
				uint32_t fragm_ind = x + TILE_SIZE * y;
				Vec2i r = {.x = r0.x + x, .y = r0.y + y};

				auto const &fine_out = fine_buf[fragm_ind];
				auto const &fragm = fine_out.fragm;
				if (fragm_check.Check(fragm, r) == false)
					continue;

				auto const &data = data_buf[fine_out.data_id];
				auto inp_out = interp.Process(data, fragm);
				uint32_t cbuf_ind = r.x + r.y * w_pix;
				cbuf[cbuf_ind] = shader.FSshader(inp_out);
			}
		}
	}
}

template <typename _shader,      typename _setup,
	  typename _bin_rast,    typename _coarse_rast, typename _fine_rast,
	  typename _fragm_check, typename _interp>
void Pipeline<_shader, _setup, _bin_rast, _coarse_rast, _fine_rast,
     _fragm_check, _interp>::Render(InputBuf const &inp_buf, Fbuffer::Color *cbuf)
{
	cur_inp_buf = &inp_buf;
	cur_cbuf = cbuf;

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
