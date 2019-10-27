#include <include/fbuffer.h>
#include <include/mouse.h>
#include <include/engine.h>
#include <iostream>
#include <ctime>

extern "C"
{
#include <unistd.h>
};

/*
int main(int argc, char *argv[])
{
	return 0;
}
*/

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

	tr_pipeline_obj obj;
	obj.load_mesh(model);
	window wnd = { .x = 0, .y = 0, .w = fb.xres, .h = fb.yres,
		       .f = 100, .n = 1 };
	obj.set_window(wnd);

	tr_pipeline pipeline;
	pipeline.set_window(wnd);
	pipeline.obj_buf.push_back(tr_pipeline::obj_entry {.ptr = &obj});

	if (argc == 3) {
		obj.set_view(3.1415 * 1.45, 3.1415 * 0.30, atof(argv[2]));
		perf(pipeline, fb);
		return 0;
	}

	float pos, xang = 3.1415, yang = 0;
	std::cin >> pos;
	obj.set_view(xang, yang, pos);

	while (1) {
		mouse::event ev;
		const float rot_scale = 0.01;
		if (ms.poll(ev)) {
			yang -= int8_t(ev.dx) * rot_scale;
			xang += int8_t(ev.dy) * rot_scale;
			obj.set_view(xang, yang, pos);
			fb.clear();
			pipeline.render(fb.buf);
			fb.update();
		}
		usleep(1024 * 1024 / 60);
	}

	return 0;
}
