#include <include/fbuffer.h>
#include <include/vector_space.h>
#include <include/rasterize.h>
#include <include/mouse.h>

#include <cstdio>
#include <cstdlib>

extern "C" {
#include <unistd.h>
}

struct test_cube3d {
	vector3d arr[8];
	vector3d angle = vector3d{ 0, 0, 0 };
	double const camera_dist = 3;
	double const size = 1;

	test_cube3d()
	{
		arr[0] = vector3d{ 0, 0, 0 };
		arr[1] = vector3d{ 1, 0, 0 };
		arr[2] = vector3d{ 1, 1, 0 };
		arr[3] = vector3d{ 0, 1, 0 };
		arr[4] = vector3d{ 0, 0, 1 };
		arr[5] = vector3d{ 1, 0, 1 };
		arr[6] = vector3d{ 1, 1, 1 };
		arr[7] = vector3d{ 0, 1, 1 };

		for (int i = 0; i < 8; i++) {
			vector3d &vec = arr[i];
			vec = vector3d{ vec.x - 0.5, vec.y - 0.5, vec.z - 0.5 };
			vec = vec * size;
		}
	}

	void rasterize(line_rasterizer &rast, std::vector<fbuffer::vector> &out)
	{
		vector2d proj[8];
		matrix3d rot_mat = rotation_matrix3d_euler(angle);
		double ratio = rast.get_ratio();

		for (int i = 0; i < 8; i++) {
			proj[i] = simple_camera_transform(arr[i], camera_dist,
							  rot_mat);
			proj[i].y = proj[i].y * ratio;
		}

		rast.rasterize(proj[0], proj[1], out);
		rast.rasterize(proj[1], proj[2], out);
		rast.rasterize(proj[2], proj[3], out);
		rast.rasterize(proj[3], proj[0], out);

		rast.rasterize(proj[4], proj[5], out);
		rast.rasterize(proj[5], proj[6], out);
		rast.rasterize(proj[6], proj[7], out);
		rast.rasterize(proj[7], proj[4], out);

		rast.rasterize(proj[0], proj[4], out);
		rast.rasterize(proj[1], proj[5], out);
		rast.rasterize(proj[2], proj[6], out);
		rast.rasterize(proj[3], proj[7], out);
	}

	void rotate(vector3d d_angle)
	{
		angle = angle + d_angle;
	}
};

int main()
{
	fbuffer fb;
	line_rasterizer rast;
	std::vector<fbuffer::vector> rast_container;
	mouse ms;
	test_cube3d cube;

	if (fb.init("/dev/fb0") < 0) {
		std::perror("fb.init");
		goto handle_err_0;
	}
	rast.set_from_fb(fb);

	if (ms.init("/dev/input/mice") < 0) {
		std::perror("ms.init");
		goto handle_err_1;
	}

	while (1) {
		const double rotate_scale = 0.01;
		mouse::event ev;
		if (ms.poll(ev)) {
			vector3d d_angle{ rotate_scale * std::int8_t(ev.dy),
					  rotate_scale * std::int8_t(ev.dx),
					  0 };
			cube.rotate(d_angle);
			cube.rasterize(rast, rast_container);
			fb.clear();
			for (auto const &vec : rast_container)
				fb[vec.y][vec.x] =
					fbuffer::color{ 255, 255, 255, 0 };
			fb.update();
			rast_container.clear();
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
