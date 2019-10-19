#include <include/fbuffer.h>
#include <include/mouse.h>

extern "C"
{
#include <unistd.h>
};

#include "shader.hpp"

int main(int argc, char *argv[])
{
	if (argc != 2)
		return 1;

	struct tr_pipeline pipeline;

	pipeline.data = import_obj(argv[1]);
	pipeline.init();
	pipeline.set_rotate(0, 0);

	pipeline.load_data();
/*
	for (int i = 0; i < 100; ++i) {
		pipeline.z_test.fb.clear();
		pipeline.process();
		pipeline.z_test.fb.update();
	}
*/

	mouse ms;
	if (ms.init("/dev/input/mice") < 0) {
		perror("mouse:");
		return 1;
	}

	float xang = 0, yang = 0;
	while (1) {
		mouse::event ev;
		const float rot_scale = 0.01;
		if (ms.poll(ev)) {
			xang += int8_t(ev.dx) * rot_scale;
			yang += int8_t(ev.dy) * rot_scale;
			pipeline.set_rotate(yang, xang);
			pipeline.z_test.fb.clear();
			pipeline.process();
			pipeline.z_test.fb.update();
		}
		usleep(1024 * 1024 / 60);
	}

	return 0;
}
