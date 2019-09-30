#include <include/fbuffer.h>
#include <include/vector_space.h>
#include <include/rasterize.h>
#include <include/mouse.h>
#include <include/wfobj.h>

#include <cstdio>
#include <cstdlib>

extern "C" {
	#include <unistd.h>
}

const fbuffer::color obj_color{255, 255, 0, 0};

struct wfobj {
	mesh data;

	vector3d angle = vector3d{0, 0, 0};
	double const camera_dist = 4;

	void import(char const * path)
	{
		data = import_obj(path);
	}

	void rasterize(line_rasterizer &rast, std::vector<fbuffer::vector> &out)
	{
		matrix3d rot_mat = rotation_matrix3d_euler(angle);
		double ratio = rast.get_ratio();

		for (std::size_t n = 0; n < data.inds.size(); n += 3) {
			mesh::vertex vert[3] = {
				data.verts[data.inds[n]],
				data.verts[data.inds[n + 1]],
				data.verts[data.inds[n + 2]]
			};

			vector3d vec[3];
			for (std::size_t i = 0; i < 3; i++)
				vec[i] = vert[i].pos;

			vector2d proj[3];
			for (std::size_t i = 0; i < 3; i++) {
				proj[i] = simple_camera_transform(vec[i],
						camera_dist, rot_mat);
				proj[i].y = proj[i].y * ratio;
			}

			rast.rasterize(proj[0], proj[1], out);
			rast.rasterize(proj[1], proj[2], out);
			rast.rasterize(proj[2], proj[0], out);
		}
	}

	void rotate(vector3d d_angle)
	{
		angle = angle + d_angle;
	}
};

int main(int argc, char *argv[])
{
	if (argc != 2) {
		std::fprintf(stderr, "Wrong args\n");
		return EXIT_FAILURE;
	}

	fbuffer fb;
	line_rasterizer rast;
	std::vector<fbuffer::vector> rast_container;
	mouse ms;
	wfobj obj;

	if (fb.init("/dev/fb0") < 0) {
		std::perror("fb.init");
		goto handle_err_0;
	}
	rast.set_from_fb(fb);

	if (ms.init("/dev/input/mice") < 0) {
		std::perror("ms.init");
		goto handle_err_1;
	}

	obj.import(argv[1]);

	while (1) {
		const double rotate_scale = 0.01;
		mouse::event ev;
		if (ms.poll(ev)) {
			vector3d d_angle{rotate_scale * std::int8_t(ev.dy),
					 rotate_scale * std::int8_t(ev.dx), 0};
			obj.rotate(d_angle);
			obj.rasterize(rast, rast_container);
			fb.clear();
			for (auto const &vec : rast_container) {
				fb[vec.y][vec.x] = obj_color;
			}
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
