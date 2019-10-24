#include <include/fbuffer.h>
#include <include/mouse.h>
#include <include/engine.h>
#include <iostream>
#include <ctime>

extern "C"
{
#include <unistd.h>
};

void perf(tr_pipeline &pipeline, fbuffer &fb)
{
	const int cycles = 100;
	time_t start_time, end_time;

	start_time = clock();
	for (int i = 0; i < cycles; i++) {
		fb.clear();
		pipeline.render(fb.buf);
		fb.update();
	}
	end_time = clock();

	std::cout << double(cycles) / double(end_time - start_time)
		     * CLOCKS_PER_SEC << " fps\n";
}

int main(int argc, char *argv[])
{
	if (argc != 2 && argc != 3)
		return 1;

	mesh model = import_obj(argv[1]);
	fbuffer fb;
	mouse ms;

	if (fb.init("/dev/fb0") < 0) {
		perror("fb0");
		return 1;
	}
	if (ms.init("/dev/input/mice") < 0) {
		perror("mouse");
		return 1;
	}

	window wnd = { .x = 0, .y = 0, .w = fb.xres, .h = fb.yres,
		       .f = 1000, .n = 10 };
	tr_pipeline pipeline(wnd);
	pipeline.load_mesh(model);

	if (argc == 3) {
		pipeline.set_view(3.1415 * 1.45, 3.1415 * 0.30, atof(argv[2]));
		perf(pipeline, fb);
		return 0;
	}

	float pos, xang = 3.1415, yang = 0;
	std::cin >> pos;
	pipeline.set_view(xang, yang, pos);

	while (1) {
		mouse::event ev;
		const float rot_scale = 0.01;
		if (ms.poll(ev)) {
			yang -= int8_t(ev.dx) * rot_scale;
			xang += int8_t(ev.dy) * rot_scale;
			pipeline.set_view(xang, yang, pos);
			fb.clear();
			pipeline.render(fb.buf);
			fb.update();
		}
		// usleep(1024 * 1024 / 60);
	}

	return 0;
}
