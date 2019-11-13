#include <include/fbuffer.h>
#include <include/mouse.h>
#include <include/pipeline.h>
#include <include/wfobj.h>
#include <iostream>
#include <chrono>
#include <utility>

#include <cstring>

extern "C"
{
#include <unistd.h>
}

int main(int argc, char *argv[])
{
	Fbuffer fb;
	//Mouse ms;
	if (fb.Init("/dev/fb0") < 0) {
		perror("fb0");
		return 1;
	}
/*
	if (ms.Init("/dev/input/mice") < 0) {
		perror("Mouse");
		return 1;
	}
*/

	std::vector<Wfobj> obj_buf;
	ImportWfobj("sky.obj", obj_buf);
	std::vector<std::array<Vertex, 3>> prim_buf;
	obj_buf[0].get_prim_buf(prim_buf);

	float pos = 20;
	Window wnd = { .x = 0, .y = 0, .w = fb.xres, .h = fb.yres,
	 	       .f = pos * 100, .n = pos / 10 };

	Mat4 view_mat = MakeMat4Rotate({0, 1, 0}, 3.1415 / 6);
	view_mat = MakeMat4Translate(Vec3{0, 0, 0.1}) * view_mat;

	Pipeline<TexShader, TrSetup, TrBinRast,
		TrCoarseRast, TrFineRast, TrInterp> pipe;
	pipe.shader.set_view(view_mat);
	pipe.shader.tex_img = &obj_buf[0].mtl.tex_img;
	pipe.set_window(wnd);
	pipe.add_concurrency(std::thread::hardware_concurrency());
	for (int i = 0; i < 100; ++i) {
		pipe.Render(prim_buf, &(fb.buf[0]));
		fb.Update();
	}

	return 0;
}
