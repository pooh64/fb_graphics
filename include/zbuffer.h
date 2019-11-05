#pragma once

#include <include/geom.h>
#include <include/fbuffer.h>

template<typename _elem>
struct Zbuffer {
	using elem  = _elem;
	float static constexpr free_depth = std::numeric_limits<float>::max();
	Window wnd;
	std::vector<elem> buf;

	void set_Window(Window const &_wnd)
	{
		wnd = _wnd;
		buf.resize(wnd.w * wnd.h);
	}

	void clear()
	{
		for (auto &e : buf)
			e.depth = free_depth;
	}

	void add_elem(uint32_t x, uint32_t y, elem const &e)
	{
		std::size_t ind = x + wnd.w * y;
		if (buf[ind].depth > e.depth)
			buf[ind] = e;
	}
};

#include <include/tr_prim.h>

using TrZbuffer = Zbuffer<TrFragment>;
