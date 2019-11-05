#include <include/fbuffer.h>
#include <include/mouse.h>
#include <include/engine.h>
#include <iostream>
#include <ctime>
#include <utility>

extern "C"
{
#include <unistd.h>
}

int main(int argc, char *argv[])
{
	if (argc < 2)
		return 1;

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
	std::vector<Wfobj> obj_buf;
	std::vector<TrModel> model_buf;
	std::vector<float> scale_buf;

	/* Import models */
	for (int i = 1; i < argc; i += 2) {
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
	TrPipeline pipeline;
	pipeline.SetWindow(wnd);
	for (auto &model : model_buf)
		pipeline.model_buf.push_back(
				TrPipeline::ModelEntry {.ptr = &model,
				.shader = model.shader});

	float pos, xang = 3.1415, yang = 0;
	std::cin >> pos;

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
