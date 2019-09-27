#include "include/fbuffer.h"
#include <cstdio>
#include <cstdlib>
#include <cerrno>

int main()
{
	int rc;
	fbuffer fb;
	fbuffer::color col = {0};
	col.g = 255;

	if (fb.init("/dev/fb0") < 0) {
		std::perror("fb.init");
		return EXIT_FAILURE;
	}

	fb.draw_line(vector2d(-0.2, -0.2), vector2d(0.2, 0.2), col);

	if (fb.destroy() < 0) {
		std::perror("fb.close");
		return EXIT_FAILURE;
	}

	return 0;
}
