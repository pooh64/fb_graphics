#include <include/fbuffer.h>
#include <include/mouse.h>
#include <include/engine.h>
#include <iostream>
#include <ctime>
#include <utility>

extern "C"
{
#include <unistd.h>
};

void perf(TrPipeline &pipeline, Fbuffer &fb, unsigned cycles)
{
	time_t start_time, end_time;

	start_time = clock();
	for (int i = 0; i < cycles; i++) {
		fb.Clear();
		pipeline.Render(fb.buf);
		fb.Update();
	}
	end_time = clock();

	std::cout << double(cycles) / double(end_time - start_time)
		     * CLOCKS_PER_SEC << " fps\n";
}


int main(int argc, char *argv[])
{
	if (argc != 3 && argc != 5)
		return 1;

	char const *obj_path = argv[1];
	char const *mtl_path = argv[2];
	bool perf_flag = false;
	float perf_pos = 0;
	unsigned perf_cycles = 0;
	if (argc == 5) {
		perf_flag = true;
		perf_pos = atof(argv[3]);
		perf_cycles = atoi(argv[4]);
	}

	Fbuffer fb;
	Mouse ms;
	if (fb.Init("/dev/fb0") < 0) {
		perror("fb0");
		return 1;
	}
	if (ms.Init("/dev/input/mice") < 0) {
		perror("Mouse");
		return 1;
	}
	Window wnd = { .x = 0, .y = 0, .w = fb.xres, .h = fb.yres,
	 	       .f = 1000, .n = 0 };
	std::vector<Wfobj> model_buf;
	std::vector<TrPipelineObj> trobj_buf;

	/* Import model */
	if (ImportWfobj(obj_path, mtl_path, model_buf) < 0) {
		std::cerr << "import_wrfobj failed\n";
		return 1;
	}

	/* Create buffer of maintainable tr objects */
	for (auto const &model : model_buf) {
#if 0
		std::cout << model.name << std::endl;
		int flag = 0;
		std::cin >> flag;
		if (!flag)
			continue;
#endif
		TrPipelineObj tmp;
		tmp.SetWfobj(model);
		tmp.SetWindow(wnd);
		if (perf_flag)
			tmp.SetView(3.1415 * 1.45, 3.1415 * 0.30, perf_pos);
		trobj_buf.push_back(std::move(tmp));
	}

	/* Create pipeline and pass tr objects */
	TrPipeline pipeline;
	pipeline.SetWindow(wnd);
	for (auto &obj : trobj_buf)
		pipeline.obj_buf.push_back(
				TrPipeline::TrObj {.ptr = &obj});

	if (perf_flag) {
		perf(pipeline, fb, perf_cycles);
		return 0;
	}

	float pos, xang = 3.1415, yang = 0;
	std::cin >> pos;

	while (1) {
		Mouse::Event ev;
		const float rot_scale = 0.01;
		if (ms.Poll(ev)) {
			yang -= int8_t(ev.dx) * rot_scale;
			xang += int8_t(ev.dy) * rot_scale;
			for (auto &obj : trobj_buf)
				obj.SetView(xang, yang, pos);
			fb.Clear();
			pipeline.Render(fb.buf);
			fb.Update();
		}
		//usleep(1024 * 1024 / 60);
	}

	return 0;
}
