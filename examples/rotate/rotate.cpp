#include <include/fbuffer.h>
#include <include/vector_space.h>
#include <include/mouse.h>

#include <cstdio>
#include <cstdlib>

extern "C" {
	#include <unistd.h>
}

struct test_cube3d {
	vector3d arr[8];
	vector3d angle = vector3d{0, 0, 0};
	double const camera_dist = 3;
	double const size = 0.6;

	test_cube3d()
	{
		arr[0] = vector3d{0, 0, 0};
		arr[1] = vector3d{1, 0, 0};
		arr[2] = vector3d{1, 1, 0};
		arr[3] = vector3d{0, 1, 0};
		arr[4] = vector3d{0, 0, 1};
		arr[5] = vector3d{1, 0, 1};
		arr[6] = vector3d{1, 1, 1};
		arr[7] = vector3d{0, 1, 1};

		for (int i = 0; i < 8; i++) {
			vector3d &vec = arr[i];
			vec = vector3d{vec.x - 0.5, vec.y - 0.5, vec.z - 0.5};
			vec = vec * size;
		}
	}

	void draw(fbuffer &fb, fbuffer::color c)
	{
		vector2d proj[8];
		matrix3d rot_mat = rotation_matrix3d(angle);

		for (int i = 0; i < 8; i++)
			proj[i] = simple_camera_transform(arr[i], camera_dist,
							  rot_mat);

		fb.draw_line_strip(proj, 4, c);
		fb.draw_line(proj[3], proj[0], c);
		fb.draw_line_strip(proj + 4, 4, c);
		fb.draw_line(proj[7], proj[4], c);
		for (int i = 0; i < 4; i++)
			fb.draw_line(proj[i], proj[i+4], c);
	}

	void rotate(vector3d d_angle)
		{ angle = angle + d_angle; }
};

int main()
{
	fbuffer fb;
	mouse ms;
	test_cube3d cube;

	if (fb.init("/dev/fb0") < 0) {
		std::perror("fb.init");
		goto handle_err_0;
	}

	if (ms.init("/dev/input/mice") < 0) {
		std::perror("ms.init");
		goto handle_err_1;
	}

	while (1) {
		const double rotate_scale = 0.01;
		mouse::event ev;
		if (ms.poll(ev)) {
			vector3d d_angle{rotate_scale * std::int8_t(ev.dy),
					 rotate_scale * std::int8_t(ev.dx), 0};
			cube.rotate(d_angle);
			fb.fill(fbuffer::color{0, 0, 0, });
			cube.draw(fb, fbuffer::color{0, 255, 0, 0});
			fb.update();
		}
		usleep(1000L * 1000L / 60);
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
