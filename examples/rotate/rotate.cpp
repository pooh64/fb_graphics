#include <include/fbuffer.h>
#include <include/vector_space.h>
#include <include/mouse.h>

#include <initializer_list>
#include <algorithm>

#include <cstdio>
#include <cstdlib>

extern "C" {
	#include <unistd.h>
}

struct test_cube3d {
	vector3d arr[8];
	double const size = 0.8;
	test_cube3d()
	{
		auto init_cube = std::initializer_list<vector3d>
		     ({ vector3d(0, 0, 0),
			vector3d(1, 0, 0),
			vector3d(1, 1, 0),
			vector3d(0, 1, 0),
			vector3d(0, 0, 1),
			vector3d(1, 0, 1),
			vector3d(1, 1, 1),
			vector3d(0, 1, 1) });
		std::copy(init_cube.begin(), init_cube.end(), arr);
		for (int i = 0; i < 8; i++) {
			vector3d &vec = arr[i];
			vec = vector3d(vec.x - 0.5, vec.y - 0.5, vec.z - 0.5);
			vec = vec * size;
		}
	}

	void draw(fbuffer &fb, fbuffer::color c)
	{
		vector2d proj[8];
		for (int i = 0; i < 8; i++)
			proj[i] = arr[i].direct_project2d();
		fb.draw_line_strip(proj,     4, c);
		fb.draw_line(proj[3], proj[0], c);
		fb.draw_line_strip(proj + 4, 4, c);
		fb.draw_line(proj[7], proj[4], c);
		for (int i = 0; i < 4; i++)
			fb.draw_line(proj[i], proj[i+4], c);
	}

	void rotate_x(double a)
	{
		for (int i = 0; i < 8; i++)
			arr[i].rotate_x(a);
	}

	void rotate_y(double a)
	{
		for (int i = 0; i < 8; i++)
			arr[i].rotate_y(a);
	}
};

int main()
{
	fbuffer fb;
	mouse ms;
	test_cube3d cube;
	fbuffer::color col, col_fill;
	col.g = 255;
	col_fill = {0};

	if (fb.init("/dev/fb0") < 0) {
		std::perror("fb.init");
		goto handle_err_0;
	}

	if (ms.init("/dev/input/mice") < 0) {
		std::perror("ms.init");
		goto handle_err_1;
	}


	std::int8_t prev_dx, prev_dy, delta_x, delta_y;
	prev_dx = prev_dy = delta_x = delta_y = 0;
	while (1) {
		const double rotate_scale = 0.01;
		mouse::event ev;
		usleep(1000L * 1000L / 60);
		if (ms.poll(ev)) {
			delta_x = (std::int8_t) ev.dx;
			delta_y = (std::int8_t) ev.dy;
			prev_dx = ev.dx;
			prev_dy = ev.dy;
			if (delta_y)
				cube.rotate_x(delta_y * rotate_scale);
			if (delta_x)
				cube.rotate_y(delta_x * rotate_scale);
			fb.fill(col_fill);
			cube.draw(fb, col);
			fb.update();
		}
	}


	if (ms.destroy() < 0) {
		std::perror("ms.destroy");
		goto handle_err_1;
	}

	if (fb.destroy() < 0) {
		std::perror("fb.destroy");
		goto handle_err_0;
	}

handle_err_1:
	fb.destroy();
handle_err_0:
	return EXIT_FAILURE;
}
