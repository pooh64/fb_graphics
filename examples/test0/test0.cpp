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
	if (fb.Init("/dev/fb0") < 0) {
		perror("fb0");
		return 1;
	}
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

#define DRAW_SKY
#define DRAW_A6M
//#define N_THREADS 4
#define N_THREADS std::thread::hardware_concurrency()
#define DRAWBACK
#define N_FRAMES 200

#ifdef DRAW_SKY
	Pipeline<TexShader, TrSetupFrontCulling, TrBinRast,
		TrCoarseRast, TrFineRast<FineRastZbufType::DISABLED>,
		TrInterp> tex_pipe;

	tex_pipe.shader.tex_img = sky.tex;
	tex_pipe.set_window(wnd);
	tex_pipe.add_concurrency(N_THREADS);

#endif
#ifdef DRAW_A6M
	Pipeline<TexHighlShader, TrSetupBackCulling, TrBinRast,
		TrCoarseRast, TrFineRast<FineRastZbufType::ACTIVE>,
		TrInterp> hgl_pipe;

	hgl_pipe.shader.tex_img = a6m.tex;
	hgl_pipe.set_window(wnd);
	hgl_pipe.add_concurrency(N_THREADS);
#endif

	for (int i = 0; i < N_FRAMES; ++i) {
		float const rotspd = 2 * 3.141593 / N_FRAMES;
		Mat4 view = view0 * MakeMat4Rotate(Vec3{0,1,0}, (i+1) * rotspd);
#ifdef DRAW_SKY
		tex_pipe.shader.set_view(view, sky.scale);
		tex_pipe.Accumulate(sky.prim_buf);
		tex_pipe.Render(&(fb.buf[0]));
#endif
#ifdef DRAW_A6M
		hgl_pipe.shader.set_view(view, a6m.scale);
		hgl_pipe.Accumulate(a6m.prim_buf);
		hgl_pipe.Render(&(fb.buf[0]));
#endif
#ifdef DRAWBACK
		fb.Update();
		fb.Clear();
#endif
	}

	return 0;
}
