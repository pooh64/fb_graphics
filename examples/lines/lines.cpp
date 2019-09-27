#include <include/fbuffer.h>
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

	double a = 0.5;

	vector2d v1(-a, -a);
	vector2d v2( a, -a);
	vector2d v3( a,  a);
	vector2d v4(-a,  a);

	fb.draw_line(v1, v2, col);
	fb.draw_line(v2, v3, col);
	fb.draw_line(v3, v4, col);
	fb.draw_line(v4, v1, col);

	if (fb.destroy() < 0) {
		std::perror("fb.close");
		return EXIT_FAILURE;
	}

	return 0;
}
