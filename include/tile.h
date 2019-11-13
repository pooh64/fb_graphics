#pragma once

#include <array>
#include <cstdint>

#define TILE_SIZE 8
#define BIN_SIZE 8

template <typename _elem>
using Tile = std::array<_elem, TILE_SIZE * TILE_SIZE>;

template <typename _elem>
using Bin = std::array<_elem, BIN_SIZE * BIN_SIZE>;

inline uint32_t RoundUp(uint32_t x, uint32_t base)
{
	uint32_t rem = x % base
	if (rem == 0)
		return x / base;
	return x / base + 1;
}

/*
struct TileTransform {
	uint32_t w_tiles;

	inline void ToTile(uint32_t x, uint32_t y,
			uint32_t &tile, uint32_t &offs) const
	{
		uint32_t delta_x = x % TILE_SIZE;
		uint32_t delta_y = y % TILE_SIZE;

		tile = ((x - delta_x) + w_tiles * (y - delta_y)) / TILE_SIZE;

		offs = delta_x + TILE_SIZE * delta_y;
	}

	inline void ToScr(uint32_t tile, uint32_t offs,
			uint32_t &x, uint32_t &y) const
	{
		uint32_t delta_x = offs % TILE_SIZE;
		uint32_t delta_y = (offs - delta_x) / TILE_SIZE;

		uint32_t t_x = tile % w_tiles;
		uint32_t t_y = (tile - t_x) / w_tiles;

		x = t_x * TILE_SIZE + delta_x;
		y = t_y * TILE_SIZE + delta_y;
	}
};
*/
