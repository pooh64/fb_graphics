#include <include/fbuffer.h>
#include <cstdio>
#include <cstdlib>
#include <cerrno>

int main()
{
	int rc;
	fbuffer fb;

	if (fb.init("/dev/fb0") < 0) {
		std::perror("fb.init");
		return EXIT_FAILURE;
	}

	double a = 0.5;
	vector2d arr[5];

	arr[0] = vector2d{-a, -a};
	arr[1] = vector2d{ a, -a};
	arr[2] = vector2d{ a,  a};
	arr[3] = vector2d{-a,  a};
	arr[4] = arr[0];

	fb.fill(fbuffer::color{0, 0, 0, 0});
	fb.draw_line_strip(arr, 5, fbuffer::color{0, 255, 0, 0});
	fb.update();

	if (fb.destroy() < 0) {
		std::perror("fb.close");
		return EXIT_FAILURE;
	}

	return 0;
}
