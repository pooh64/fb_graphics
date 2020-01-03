#define HACK_TRINTERP_LINEAR
#define HACK_TRSHADER_NO_BOUNDS
//#define HACK_DRAWBIN_NO_DRAWBACK

//#define N_THREADS 1
#define N_THREADS std::thread::hardware_concurrency()
#define DRAWBACK
#define N_FRAMES 300

#define TILE_SIZE 16
#define BIN_SIZE 8

#include <include/fbuffer.h>
#include <include/mouse.h>
#include <include/tr_pipeline.h>
#include <include/wfobj.h>
#include <iostream>
#include <chrono>
#include <utility>

#include "raycast.hpp"

Sphere const sph[] = {
	{ Vec3{ 0.f, 0.f, -1e4f }, 1e4f, Vec3{ 0.3f, 1.f, 0.3f } },
	{ Vec3{ 0.f, 0.f, 0.3f }, 0.5f, Vec3{ 1.f, 1.f, 1.f } }
};
int const sph_size = sizeof(sph) / sizeof(Sphere);

struct MyShader final : public Shader<Vec2, Vertex, Fbuffer::Color> {
	Camera cam;
	Vec3 light = Normalize(Vec3{ 1.f, 1.f, 2.f });

	float w, h;

	VsOut VShader(VsIn const &v) const override
	{
		VsOut out;
		out.pos = Vec4{ (v.x + 1) * w, (-v.y + 1) * h, h,
				1.f }; //negative z -> don't work, why?
		out.fs_vtx.pos = Vec3{ v.x, v.y, 1 };
		return out;
	}
	Fbuffer::Color FShader(FsIn const &vert) const override
	{
		Vec2 v{ vert.pos.x, vert.pos.y };

		Ray const r = CastFromCam(cam, v);
		Intersection const I = FindIntersection(r, sph, sph_size);
		if (I.idx < 0)
			return Fbuffer::Color{ 100, 25, 25, 255 };

		Vec3 color = sph[I.idx].color;
		Vec3 const pos = RayVec(r, I.t);
		Intersection const shadow =
			FindIntersection({ pos, light }, sph, sph_size);
		if (shadow.idx >= 0) {
			return Fbuffer::Color{ uint8_t(255 * color.x * 0.8f),
					       uint8_t(255 * color.y * 0.3f),
					       uint8_t(255 * color.z * 0.3f),
					       255 };
		}

		Vec3 const norm = Normalize(pos - sph[I.idx].pos);
		Vec3 const halfway = Normalize(light - r.dir);

		float const NH = std::max(0.f, DotProd(norm, halfway));
		float spec = NH;
		for (int i = 0; i < 4; i++)
			spec *= spec;
		float const diff = std::max(0.f, DotProd(norm, light));
		color = (0.3f + 0.4f * diff + 0.3f * spec) * color;
		return Fbuffer::Color{ uint8_t(color.x * 255),
				       uint8_t(color.y * 255),
				       uint8_t(color.z * 255), 255 };
	}
	void set_view(Mat4 const &view, float scale) override
	{
	}
	void set_window(Window const &wnd) override
	{
		w = wnd.w / 2;
		h = wnd.h / 2;
	}
};

enum class RaySetupCullingType {
	NO_CULLING,
	BACK,
	FRONT,
};

using RayPrim = std::array<Vec2, 3>;

template <RaySetupCullingType _type, typename _shader>
struct RaySetup : public Setup<RayPrim, TrData, _shader> {
	using Base = Setup<RayPrim, TrData, _shader>;
	using In = typename Base::In;
	using Data = typename Base::Data;
	using _Shader = typename Base::_Shader;
	void Process(In const &in, std::vector<Data> &out) const override
	{
		Data data;
		for (int i = 0; i < in.size(); ++i) {
			auto vs_out = Base::shader.VShader(in[i]);
			data[i].pos = vs_out.pos;
			data[i].fs_vtx = vs_out.fs_vtx;
		}

		Vec3 tr[3] = { ReinterpVec3(data[0].pos),
			       ReinterpVec3(data[1].pos),
			       ReinterpVec3(data[2].pos) };

		Vec3 d1_3 = tr[1] - tr[0];
		Vec3 d2_3 = tr[2] - tr[0];
		Vec2 d1 = { d1_3.x, d1_3.y };
		Vec2 d2 = { d2_3.x, d2_3.y };

		float det = d1.x * d2.y - d1.y * d2.x;
		if (_type == decltype(_type)::BACK) {
			if (det >= 0)
				return;
		} else if (_type == decltype(_type)::FRONT) {
			if (det <= 0)
				return;
			auto tmp = data[0];
			data[0] = data[2];
			data[2] = tmp;
		} else {
			if (det >= 0) {
				auto tmp = data[0];
				data[0] = data[2];
				data[2] = tmp;
			}
		}
		out.push_back(data);
	}

	void set_window(Window const &wnd) override
	{
		/* Nothing */
	}
};

template <typename _shader>
struct RaySetupNoCulling final
	: public RaySetup<RaySetupCullingType::NO_CULLING, _shader> {
};

template <typename _shader>
struct RaySetupBackCulling final
	: public RaySetup<RaySetupCullingType::BACK, _shader> {
};

template <typename _shader>
struct RaySetupFrontCulling final
	: public RaySetup<RaySetupCullingType::FRONT, _shader> {
};

Vec2 const verts[] = { { -1.f, -1.f }, { -1.f, 0.f }, { -1.f, 1.f },
		       { 0.f, -1.f },  { 0.f, 0.f },  { 0.f, 1.f },
		       { 1.f, -1.f },  { 1.f, 0.f },  { 1.f, 1.f } };
unsigned int const inds[] = { 0, 3, 1, 1, 3, 4, 3, 6, 4, 4, 6, 7,
			      1, 4, 2, 2, 4, 5, 4, 7, 5, 5, 7, 8 };

std::vector<RayPrim> MakeRayPrimBuf(Vec2 const *verts, unsigned const *inds,
				    size_t n_inds)
{
	std::vector<RayPrim> out;
	for (int i = 0; i < n_inds; i += 3) {
		out.push_back(RayPrim{ verts[inds[i]], verts[inds[i + 1]],
				       verts[inds[i + 2]] });
	}
	return out;
}

int main(int argc, char *argv[])
{
	Fbuffer fb;
	if (fb.Init("/dev/fb0") < 0) {
		perror("fb0");
		return 1;
	}

	float z_avg = 1;
	Window wnd = { .x = 0,
		       .y = 0,
		       .w = fb.xres,
		       .h = fb.yres,
		       .f = z_avg * 100,
		       .n = z_avg / 100 };

	SyncThreadpool sync_tp;
	sync_tp.add_concurrency(N_THREADS);
	Pipeline<MyShader, RaySetupBackCulling, TrBinRast, TrCoarseRast,
		 TrFineRast<TrFineRastZbufType::ACTIVE>,
		 TrInterp<TrInterpType::POS>> pipe;
	pipe.set_window(wnd);
	pipe.set_sync_tp(&sync_tp);

	auto prim_buf =
		MakeRayPrimBuf(verts, inds, sizeof(inds) / sizeof(*inds));

	pipe.shader.cam = { 1.f,
			    static_cast<float>(wnd.w) / wnd.h,
			    { 0.f, 0.f, 0.f },
			    { 0.f, 0.f, 0.f },
			    { 0.f, 0.f, 1.f } };
	pipe.shader.set_window(wnd);
	float theta = 0.4f;

	for (int i = 0; i < N_FRAMES; ++i) {
		float const phi = 2.f / N_FRAMES * 3.141593 * i;
		Vec3 const dir{ std::cos(phi) * std::cos(theta),
				std::sin(phi) * std::cos(theta),
				std::sin(theta) };
		pipe.shader.cam.pos = 1.5f * dir;

		auto const t0 = std::chrono::system_clock::now();

		pipe.Accumulate(prim_buf);
		pipe.Render(&(fb.buf[0]));

		auto const t1 = std::chrono::system_clock::now();
		std::chrono::duration<double, std::milli> const dt = t1 - t0;
		std::cout << dt.count() << std::endl;
#ifdef DRAWBACK
		fb.Update();
		fb.Clear();
#endif
	}
	return 0;
}
