#include <include/fbuffer.h>
#include <include/rasterize.h>
#include <cstdio>
#include <cstdlib>
#include <cerrno>

int main()
{
	int rc;
	fbuffer fb;
	line_rasterizer rast;

	if (fb.init("/dev/fb0") < 0) {
		std::perror("fb.init");
		return EXIT_FAILURE;
	}

	rast.set_from_fb(fb);

	double const a = 0.3;
	double ratio = rast.get_ratio();
	vector2d arr[5];

	arr[0] = vector2d{ -a, -a * ratio };
	arr[1] = vector2d{ a, -a * ratio };
	arr[2] = vector2d{ a, a * ratio };
	arr[3] = vector2d{ -a, a * ratio };
	arr[4] = arr[0];

	std::vector<fbuffer::vector> rast_container;
	for (int i = 0; i < 4; i++)
		rast.rasterize(arr[i], arr[i + 1], rast_container);

	fb.clear();
	for (auto const &vec : rast_container)
		fb[vec.y][vec.x] = fbuffer::color{ 255, 255, 0, 0 };
	fb.update();

	if (fb.destroy() < 0) {
		std::perror("fb.close");
		return EXIT_FAILURE;
	}

	return 0;
}
