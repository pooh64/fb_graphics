#pragma once

#include <cstdint>

struct Ppm_img {
	struct Color {
		uint8_t r, g, b;
	} *buf;
	uint32_t w, h;
};

int ImportPpm(char const *path, Ppm_img &img);
