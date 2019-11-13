#pragma once

#include <include/tile.h>
#include <include/fbuffer.h>
#include <include/geom.h>
#include <include/sync_threadpool.h>

#include <iostream>
#include <vector>
#include <array>
#include <typeinfo>

template <typename _vs_in, typename _fragm, typename _fs_out>
struct Shader {
	using VsIn  = _vs_in;
	using Fragm  = _fragm;
	using FsOut = _fs_out;
	struct VsOut {
		Vec4 pos;
		FsIn fs_vtx;
	};
	virtual VsOut VShader(VsIn const &) const = 0;
	virtual FsOut FShader(Fragm const &) const = 0;
	virtual void set_view(Mat4 &view, float scale) = 0;
	virtual void set_window(Window const &) = 0;
};

template <typename _in, typename _data>
struct Setup {
	using In   = _in;
	using Data = _out;
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
	using In   = _in;
	using Out  = _out;w
	virtual Out Process(Data const &, Fragm const &) const = 0;
};


template <typename _shader,      typename _setup,
	  typename _bin_rast,    typename _coarse_rast, typename _fine_rast,
	  typename _fragm_check, typename _interp>
struct Pipeline {
	using Input    = _Setup::In;
	using _Shader  = _shader;
	using InputBuf = std::vector<Input>;

	_Shader shader;

	void Render(InputBuf const &_inp_buf, Fbuffer::Color *cbuf);
	void set_concurrency(uint32_t n_threads);
	void set_window(Window const &wnd);
private:
	uint32_t w_bins;
	uint32_t w_tiles;
	uint32_t w_pix;

	using _Setup      = _setup;
	using _BinRast    = _bin_rast;
	using _CoarseRast = _coarse_rast;
	using _FineRast   = _rasterizer;
	using _FragmCheck = _fragm_check;
	using _Interp     = _interp;

	_Setup      setup;
	_BinRast    bin_rast;
	_CoarseRast coarse_rast;
	_FineRast   fine_rast;
	_FragmCheck fragm_check;
	_Interp     interp;

	using Data  = _Setup::Data;
	using Fragm = _FineRast::Out;

	using DataBuf   = std::vector<Data>;
	using BinBuf    = std::vector<std::vector<uint32_t>>;
	using CoarseBuf = Bin<std::vector<uint32_t>>;
	using FineBuf   = Tile<Fragm>;

	std::vector<DataBuf>   data_buffs;
	std::vector<BinBuf>    bin_buffs;
	std::vector<CoarseBuf> coarse_buffs;
	std::vector<FineBuf>   fine_buffs;

	/* Threading & routines */
	InputBuf const *cur_inp_buf;
	SyncThreadpool sync_tp;

	void Pipeline::SetupProcessRoutine(int thread_id, int task_id);
	void Pipeline::  SetupMergeRoutine(int thread_id, int task_id);
	void Pipeline::     BinRastRoutine(int thread_id, int task_id);
	void Pipeline::     DrawBinRoutine(int thread_id, int task_id);
};

template <typename _shader,      typename _setup,
	  typename _bin_rast,    typename _coarse_rast, typename _fine_rast,
	  typename _fragm_check, typename _interp>
void Pipeline::set_window(Window const &wnd)
{
	setup.set_window(wnd);
	bin_rast.set_window(wnd);
	coarse_rast.set_window(wnd);
	fine_rast.set_window(wnd);
	fragm_check.set_window(wnd);

	w_pix   = wnd.w;
	w_tiles = RoundUp(w_pix, TILE_SIZE);
	w_bins  = RoundUp(w_tiles, BIN_SIZE);
}

template <typename _shader,      typename _setup,
	  typename _bin_rast,    typename _coarse_rast, typename _fine_rast,
	  typename _fragm_check, typename _interp>
void Pipeline::set_concurrency(uint32_t n_threads)
{
	sync_tp.set_concurrency(n_threads);
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
void Pipeline::SetupProcessRoutine(int thread_id, int task_id)
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
void Pipeline::SetupMergeRoutine(int thread_id, int task_id)
{
	auto task = task_buf[task_id];

	for (uint32_t offs = 0; offs < data_buf.size(); ++offs)
		data_buffs[0][offs + task.end] = data_buffs[task.beg][offs];
}

template <typename _shader,      typename _setup,
	  typename _bin_rast,    typename _coarse_rast, typename _fine_rast,
	  typename _fragm_check, typename _interp>
void Pipeline::BinRastRoutine(int thread_id, int task_id)
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
void Pipeline::DrawBinRoutine(int thread_id, int task_id)
{
	auto task = task_buf[task_id];
	uint32_t bin_id = task.beg;

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

				auto const &crs_out = coarse_buf[fragm_id];
				auto const &fragm = crs_out.fragm;
				if (fragm_check.Check(fragm, r) == false)
					continue;

				auto const &data = data_buf[crs_out.data_id];
				auto inp_out = interp.Process(data, fragm);
				uint32_t cbuf_ind = r.x + r.y * wnd_w;
				cbuf[cbuf_ind] = shader.FSshader(inp_out);
			}
		}
	}
}

template <typename _shader,      typename _setup,
	  typename _bin_rast,    typename _coarse_rast, typename _fine_rast,
	  typename _fragm_check, typename _interp>
void Pipeline::Render(InputBuf const &inp_buf, Fbuffer::Color *cbuf)
{
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
