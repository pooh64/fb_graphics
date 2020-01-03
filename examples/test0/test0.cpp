#include "conf.h"

#include <include/fbuffer.h>
#include <include/mouse.h>
#include <include/tr_pipeline.h>
#include <include/wfobj.h>
#include <iostream>
#include <chrono>
#include <utility>

struct Model {
	std::vector<std::array<Vertex, 3>> prim_buf;
	PpmImg *tex;
	float scale;
};

int main(int argc, char *argv[])
{
	Fbuffer fb;
	if (fb.Init(DEV_FB_PATH) < 0) {
		perror(DEV_FB_PATH);
		return 1;
	}
	Model sky, a6m;

	std::vector<Wfobj> obj_buf;
	assert(!ImportWfobj(SKY_OBJ_PATH, obj_buf));
	assert(!ImportWfobj(A6M_OBJ_PATH, obj_buf));

	obj_buf[0].get_prim_buf(sky.prim_buf);
	obj_buf[1].get_prim_buf(a6m.prim_buf);

	sky.tex = &obj_buf[0].mtl.tex_img;
	a6m.tex = &obj_buf[1].mtl.tex_img;

	sky.scale = SKY_SCALE;
	a6m.scale = A6M_SCALE;

	float z_avg = 1;
	Window wnd = { .x = 0, .y = 0, .w = fb.xres, .h = fb.yres,
	 	       .f = z_avg * 100, .n = z_avg / 100 };

	Vec3 eye EYE_POS;
	Vec3 at  {0, 0, 0};
	Vec3 up  {0, 1, 0};
	Mat4 view0 = MakeMat4LookAt(eye, at, up);

	SyncThreadpool sync_tp;
	sync_tp.add_concurrency(N_THREADS);

#ifdef DRAW_SKY
	Pipeline<TexShader, TrSetupFrontCulling, TrBinRast,
		TrCoarseRast, TrFineRast<TrFineRastZbufType::DISABLED>,
		TrInterp<TrInterpType::TEXTURE>> tex_pipe;

	tex_pipe.shader.set_tex_img(sky.tex);
	tex_pipe.set_window(wnd);
	tex_pipe.set_sync_tp(&sync_tp);

#endif
#ifdef DRAW_A6M
	Pipeline<TexHighlShader, TrSetupBackCulling, TrBinRast,
		TrCoarseRast, TrFineRast<TrFineRastZbufType::ACTIVE>,
		TrInterp<TrInterpType::ALL>> hgl_pipe;

	hgl_pipe.shader.set_tex_img(a6m.tex);
	hgl_pipe.set_window(wnd);
	hgl_pipe.set_sync_tp(&sync_tp);
#endif
#ifdef MOUSE_ROTATE
	Mouse ms;
	Mouse::Event ms_event;
	if (ms.Init("/dev/input/mice") < 0) {
		perror("/dev/input/mice");
		return 1;
	}
	float xrot = 0, yrot = 0;
	while (1) {
		float const rotspd = MOUSE_ROTSPD;
		while (!ms.Poll(ms_event))
			/* repeat */;
		xrot += ms_event.dx * rotspd;
		yrot += ms_event.dy * rotspd;
		Mat4 view = MakeMat4Rotate(Vec3{0,1,0}, xrot * rotspd);
		view      = MakeMat4Rotate(Vec3{1,0,0}, yrot * rotspd) * view;
		view = view0 * view;
#else
	for (int i = 0; i < N_FRAMES; ++i) {
		float const rotspd = 2 * 3.141593 / N_FRAMES;
		Mat4 view = view0 * MakeMat4Rotate(Vec3{0,1,0}, (i+1) * rotspd);
		auto const t0 = std::chrono::system_clock::now();
#endif
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
#ifndef MOUSE_ROTATE
		auto const t1 = std::chrono::system_clock::now();
		std::chrono::duration<double, std::milli> const dt = t1 - t0;
		std::cout << dt.count() << std::endl;
#endif
#ifdef DRAWBACK
		fb.Update();
		//fb.Clear();
#endif
	}

	return 0;
}
