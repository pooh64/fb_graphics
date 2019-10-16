#include <include/fbuffer.h>
#include "shader.hpp"

int main()
{
	struct tr_pipeline pipeline;

	pipeline.data = import_obj("../../models/cat.obj");
	pipeline.init();

	pipeline.load_data();

	pipeline.z_test.fb.clear();
	pipeline.process();
	pipeline.z_test.fb.update();

	return 0;
}
