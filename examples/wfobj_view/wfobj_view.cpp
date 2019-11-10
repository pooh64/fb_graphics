#include <include/fbuffer.h>
#include <include/mouse.h>
#include <include/engine.h>
#include <iostream>
#include <chrono>
#include <utility>

#include <cstring>

extern "C"
{
#include <unistd.h>
}

int perf(Fbuffer &fb, TrPipeline &pipeline,
		std::vector<TrModel> &model_buf,
		std::vector<float> &scale_buf, float pos, int n_frames)
{
	float xang = 3.1415, yang = 3.1415 / 3;

	auto beg_time = std::chrono::system_clock::now();

	for (int i = 0; i < n_frames; ++i) {
		const float rotspeed = 0.05;
		const float dx = 1 * rotspeed, dy = 0.07 * rotspeed;
		yang -= dx;
		xang += dy;
		for (int i = 0; i < model_buf.size(); ++i) {
			auto &obj = model_buf[i];
			float scale = scale_buf[i];
			obj.SetView(xang, yang, pos, scale);
		}
		fb.Clear();
		pipeline.Render(fb.buf);
		fb.Update();
	}

	auto end_time = std::chrono::system_clock::now();
	std::chrono::duration
		<double> perf_time = end_time - beg_time;

	std::cout << "fps: " <<
		double(n_frames) / perf_time.count() << std::endl;
	return 0;
}

int main(int argc, char *argv[])
{
	if (argc < 2)
		return 1;

	int obj_argc = 1;
	bool perf_flag = false;
	int perf_frames;

	if (!strcmp(argv[obj_argc], "-help")) {
		std::cout << "wfobj_view -help" << std::endl;
		std::cout << "wfobj_view pos 1.obj scale1 2.obj scale2 ..." <<
			std::endl;
		std::cout << "wfobj_view -perf nframes pos 1.obj scale1 ..." <<
			std::endl;
		return 0;
	}

	if (!strcmp(argv[obj_argc], "-perf")) {
		perf_flag = true;
		perf_frames = atoi(argv[2]);
		obj_argc += 2;
	}
	float pos = atof(argv[obj_argc++]);

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
	 	       .f = pos * 100, .n = pos / 10 };
	std::vector<Wfobj> obj_buf;
	std::vector<TrModel> model_buf;
	std::vector<float> scale_buf;

	/* Import models */
	for (int i = obj_argc; i < argc; i += 2) {
		if (ImportWfobj(argv[i], obj_buf) < 0) {
			std::cerr << "import_wfobj failed\n";
			return 1;
		}
		scale_buf.push_back(atof(argv[i + 1]));
	}

	/* Create buffer of maintainable tr objects */
	for (auto const &model : obj_buf) {
#if 0
		std::cout << model.name << std::endl;
		int flag = 0;
		std::cin >> flag;
		if (!flag)
			continue;
#endif
		TrModel tmp;
		tmp.SetWfobj(model);
		tmp.SetWindow(wnd);
		model_buf.push_back(std::move(tmp));
	}

	/* Create pipeline and pass tr objects */
	TrPipeline pipeline(std::thread::hardware_concurrency());
	pipeline.SetWindow(wnd);
	for (auto &model : model_buf)
		pipeline.model_buf.push_back(
				TrPipeline::ModelEntry {.ptr = &model,
				.shader = model.shader});

	float xang = 3.1415, yang = 0;

	if (perf_flag) {
		perf(fb, pipeline, model_buf, scale_buf, pos, perf_frames);
		return 0;
	}

	while (1) {
		Mouse::Event ev;
		const float rot_scale = 0.01;
		if (ms.Poll(ev)) {
			yang -= int8_t(ev.dx) * rot_scale;
			xang += int8_t(ev.dy) * rot_scale;
			for (int i = 0; i < model_buf.size(); ++i) {
				auto &obj = model_buf[i];
				float scale = scale_buf[i];
				obj.SetView(xang, yang, pos, scale);
			}
			fb.Clear();
			pipeline.Render(fb.buf);
			fb.Update();
		}
		//usleep(1024 * 1024 / 60);
	}

	return 0;
}
