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


struct Model {
	std::vector<std::array<Vertex, 3>> prim_buf;
	PpmImg *tex;
	float scale;
};

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
	Model sky, a6m;

	std::vector<Wfobj> obj_buf;
	ImportWfobj("sky.obj", obj_buf);
	ImportWfobj("A6M.obj", obj_buf);

	obj_buf[0].get_prim_buf(sky.prim_buf);
	obj_buf[1].get_prim_buf(a6m.prim_buf);

	sky.tex = &obj_buf[0].mtl.tex_img;
	a6m.tex = &obj_buf[1].mtl.tex_img;

	sky.scale = 1000;
	a6m.scale = 0.05;

	float z_avg = 1;
	Window wnd = { .x = 0, .y = 0, .w = fb.xres, .h = fb.yres,
	 	       .f = z_avg * 100, .n = z_avg / 100 };

#if 1
	Vec3 eye {0, 0, 1};
	Vec3 at  {0, 0, 0};
	Vec3 up  {0, 1, 0};
	Mat4 view0 = MakeMat4LookAt(eye, at, up);
#else
	Mat4 view0 = MakeMat4Translate(Vec3{0,0,1});
#endif

//	Pipeline<TexShader, TrSetup, TrBinRast,
//		TrCoarseRast, TrFineRast, TrInterp> tex_pipe;
	Pipeline<TexShader, TrSetup, TrBinRast,
		TrCoarseRast, TrFineRast, TrInterp> hgl_pipe;
/*
	tex_pipe.shader.tex_img = sky.tex;
	tex_pipe.set_window(wnd);
	tex_pipe.add_concurrency(std::thread::hardware_concurrency());
*/
	hgl_pipe.shader.tex_img = a6m.tex; // sky
	hgl_pipe.set_window(wnd);
	hgl_pipe.add_concurrency(std::thread::hardware_concurrency());

	for (int i = 0; i < 1000; ++i) {
		float const rotspd = 3.1415 / 1000;
		Mat4 view = view0 * MakeMat4Rotate(Vec3{0,1,0}, i * rotspd);
/*
		tex_pipe.shader.set_view(view, sky.scale);
		tex_pipe.Accumulate(sky.prim_buf);
		tex_pipe.Render(&(fb.buf[0]));
*/
//		fb.Clear();
		hgl_pipe.shader.set_view(view, a6m.scale);
		hgl_pipe.Accumulate(a6m.prim_buf);
		hgl_pipe.Render(&(fb.buf[0]));
//		fb.Update();
	}

	return 0;
}
