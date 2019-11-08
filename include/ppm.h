#pragma once

#include <cstdint>
#include <vector>

struct PpmImg {
	struct Color {
		uint8_t r, g, b;
	};

	std::vector<Color> buf;

	uint32_t w, h;

	int Import(char const *path);
};
