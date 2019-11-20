#pragma once

#include <array>
#include <cstdint>

#define TILE_SIZE 16
#define BIN_SIZE 8

#define TILE_PIX TILE_SIZE
#define BIN_PIX  (BIN_SIZE * TILE_PIX)

template <typename _elem>
using Tile = std::array<_elem, TILE_SIZE * TILE_SIZE>;

template <typename _elem>
using Bin = std::array<_elem, BIN_SIZE * BIN_SIZE>;

inline uint32_t DivRoundUp(uint32_t x, uint32_t base)
{
	uint32_t rem = x % base;
	if (rem == 0)
		return x / base;
	return x / base + 1;
}
