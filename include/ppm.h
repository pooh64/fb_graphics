#pragma once

#include <cstdint>

struct PpmImg {
	struct Color {
		uint8_t r, g, b;
	} *buf;
	uint32_t w, h;

	int Import(char const *path);
	void Destroy();
};
