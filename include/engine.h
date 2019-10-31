#pragma once

#include <iostream>
#include <vector>
#include <array>
#include <typeinfo>

#include <include/wfobj.h>
#include <include/geom.h>
#include <include/shader.h>
#include <include/rasterizer.h>
#include <include/zbuffer.h>
#include <include/interpolator.h>

struct TrPipelineObj {
	Mat4 modelview_mat;
	Mat4 proj_mat;
	Mat4 norm_mat;
	std::vector<std::array<Vertex, 3>> prim_buf;
	Window wnd;
	Ppm_img const *tex_img;

	void SetWfobj(Wfobj const &e)
	{
		for (std::size_t n = 0; n < e.mesh.inds.size(); n += 3) {
			Wfobj::Mesh const &m = e.mesh;
			std::array<Vertex, 3> vtx = {
				m.verts[m.inds[n]],
				m.verts[m.inds[n + 1]],
				m.verts[m.inds[n + 2]]
			};
			prim_buf.push_back(vtx);
		}
		tex_img = &e.mtl.tex_img;
	}

	void SetView(float xang, float yang, float pos)
	{
		modelview_mat = MakeMat4Translate(Vec3 {0, 0, 0});
		modelview_mat = MakeMat4Rotate(Vec3 {0, 1, 0}, yang)
			* modelview_mat;
		modelview_mat = MakeMat4Rotate(Vec3 {1, 0, 0}, xang)
			* modelview_mat;
		modelview_mat = MakeMat4Translate(Vec3 {0, 0, -pos})
			* modelview_mat;

		Mat4 view = MakeMat4Identy();

		modelview_mat = view * modelview_mat;
		norm_mat = Transpose(Inverse(modelview_mat));
	}

	void SetWindow(Window const &_wnd)
	{
		wnd = _wnd;
		float fov = 3.1415 / 3; // fov = 60deg
		float far  = 1;
		float near = 0.05;

		float size = std::tan(fov / 2) * near;

		float ratio = float (wnd.w) / wnd.h;
		proj_mat = MakeMat4Projection(size * ratio,
			-size * ratio, size, -size, far, near);
	}
};

struct TrPipeline {
	TexHighlShader	shader;
	TrRasterizer		rast;
	TrZbuffer		zbuf;
	TrInterpolator		interp;

	struct TrObj {
		TrPipelineObj *ptr;
		std::vector<std::array<decltype(shader)::vs_out, 3>> VShader_buf;
	};
	std::vector<TrObj> obj_buf; // maintain Rendering list

	void SetWindow(Window const &wnd)
	{
		rast.set_Window(wnd);
		zbuf.set_Window(wnd);
		rast_buf.resize(wnd.h);
		zbuf.clear();
	}

	void Render(Fbuffer::Color *cbuf)
	{
		for (uint32_t oid = 0; oid < obj_buf.size(); ++oid)
			RenderToZbuf(oid);
		RenderToCbuf(cbuf);
		for (auto &e : obj_buf)
			e.VShader_buf.clear();
	}

private:

	std::vector<std::vector<TrRasterizer::rz_out>> rast_buf;

	void RenderToZbuf(uint32_t oid)
	{
		TrObj &entry = obj_buf[oid];
		TrPipelineObj const &obj = *entry.ptr;

		shader.modelview_mat = obj.modelview_mat;
		shader.proj_mat = obj.proj_mat;
		shader.norm_mat = obj.norm_mat;
		shader.SetWindow(obj.wnd);

		// TEMPORARY!!!!!!!!!!!
		shader.tex_img = obj_buf[0].ptr->tex_img;

		// vshading
		for (auto const &pr : obj.prim_buf) {
			std::array<decltype(shader)::vs_out, 3> vs_pr;
			for (int i = 0; i < 3; ++i)
				vs_pr[i] = shader.VShader(pr[i]);
			entry.VShader_buf.push_back(vs_pr);
		}

		// rasterization
		for (int pid = 0; pid < entry.VShader_buf.size(); ++pid) {
			auto const &vs_pr = entry.VShader_buf[pid];
			Vec3 scr_pos[3];
			for (int i = 0; i < 3; ++i)
				scr_pos[i] = ReinterpVec3(vs_pr[i].pos);
			rast.rasterize(scr_pos, pid, rast_buf);
		}

		// Zbuffering
		for (uint32_t y = 0; y < rast_buf.size(); ++y) {
			for (auto const &re : rast_buf[y]) {
				auto ze = decltype(zbuf)::elem {
				        .bc    = {re.bc[0], re.bc[1], re.bc[2]},
					.depth = re.depth,
					.x     = re.x,
					.pid   = re.pid,
					.oid   = oid };
				zbuf.add_elem(y, ze);
			}
		}

		for (auto &line : rast_buf)
			line.clear();
	}

	Fbuffer::Color RenderFragment(decltype(zbuf)::elem const &e)
	{
		auto const &pr_vs = obj_buf[e.oid].VShader_buf[e.pid];
		Vertex pr_vtx[3] = { pr_vs[0].fs_vtx,
			pr_vs[1].fs_vtx, pr_vs[2].fs_vtx };
		Vertex vtx = interp.Interpolate(pr_vtx, e.bc);
		return shader.FShader(vtx);
	}

	void RenderToCbuf(Fbuffer::Color *cbuf)
	{
		uint32_t zbuf_w = zbuf.wnd.w;
		uint32_t zbuf_h = zbuf.wnd.h;
		for (uint32_t y = 0; y < zbuf_h; ++y) {
			uint32_t ind = y * zbuf_w;
			for (uint32_t x = 0; x < zbuf_w; ++x, ++ind) {
				decltype(zbuf)::elem e = zbuf.buf[ind];
				zbuf.buf[ind].depth = decltype(zbuf)::free_depth;
				if (e.depth != decltype(zbuf)::free_depth)
					cbuf[ind] = RenderFragment(e);
			}
		}
	}
};
