#pragma once
#include <include/geom.h>
#include <include/fbuffer.h>

template<typename _zb_in, typename _elem>
struct zbuffer {
	using zb_in = _zb_in;
	using elem  = _elem;
	float static constexpr free_depth = std::numeric_limits<float>::max();
	window wnd;
	std::vector<elem> buf;

	void set_window(window const &_wnd)
	{
		wnd = _wnd;
		buf.resize(wnd.w * wnd.h);
	}

	void clear()
	{
		for (auto &e : buf)
			e.depth = free_depth;
	}

	virtual void add_elem(uint32_t y, elem const &e) = 0;
};

struct tr_zbuffer_elem {
	float    bc[3];
	float    depth;
	uint32_t x;
	uint32_t pid;
	uint32_t oid;
};

struct tr_zbuffer final: public zbuffer<float[3], tr_zbuffer_elem> {
	void add_elem(uint32_t y, elem const &e) override
	{
		std::size_t ind = e.x + wnd.w * y;
		if (buf[ind].depth > e.depth)
			buf[ind] = e;
	}
};
