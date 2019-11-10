#pragma once

#include <include/geom.h>
#include <include/fbuffer.h>
#include <include/tile.h>

template<typename _elem>
struct Zbuffer {
	using elem  = _elem;
	float static constexpr free_depth = std::numeric_limits<float>::max();
	std::vector<Tile<elem>> buf;

	void clear()
	{
		for (auto &tile : buf) {
			for (auto &e : tile)
				e.depth = free_depth;
		}
	}

	inline void add_elem(uint32_t tile, uint32_t offs, elem const &e)
	{
		if (buf[tile][offs].depth > e.depth)
			buf[tile][offs] = e;
	}
};

#include <include/tr_prim.h>

using TrZbuffer = Zbuffer<TrFragment>;
