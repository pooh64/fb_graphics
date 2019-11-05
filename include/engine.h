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


struct TrModel {
public:
	std::vector<std::array<Vertex, 3>> prim_buf;
	ModelShader *shader;

	Window wnd; // remove

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

		auto illum = e.mtl.illum;
		if      (illum == Wfobj::Mtl::IllumType::COLOR)
			shader = new TexShader;
		else if (illum == Wfobj::Mtl::IllumType::HIGHLIGHT)
			shader = new TexHighlShader;
		else
			assert(0);
		shader->tex_img = &e.mtl.tex_img;
	}

	void SetView(float xang, float yang, float pos, float scale)
	{
		shader->modelview_mat = MakeMat4Scale(
				Vec3 {scale, scale, scale});
		shader->modelview_mat = MakeMat4Rotate(Vec3 {0, 1, 0}, yang)
			* shader->modelview_mat;
		shader->modelview_mat = MakeMat4Rotate(Vec3 {1, 0, 0}, xang)
			* shader->modelview_mat;
		shader->modelview_mat = MakeMat4Translate(Vec3 {0, 0, -pos})
			* shader->modelview_mat;

		Mat4 view = MakeMat4Identy();

		shader->modelview_mat = view * shader->modelview_mat;
		shader->norm_mat = Transpose(Inverse(shader->modelview_mat));

		if (typeid(*shader) == typeid(TexHighlShader)) {
			auto sh = dynamic_cast<TexHighlShader*>(shader);
			sh->light = ReinterpVec3(shader->norm_mat *
					(Vec4 {1, 1, 1, 1}));
			sh->light = Normalize(sh->light);
		}
	}

	void SetWindow(Window const &_wnd)
	{
		wnd = _wnd;
		shader->SetWindow(wnd);

		float fov = 3.1415 / 3; // fov = 60deg
		float far  = 1;
		float near = 0.05;

		float size = std::tan(fov / 2) * near;

		float ratio = float (wnd.w) / wnd.h;
		shader->proj_mat = MakeMat4Projection(size * ratio,
			-size * ratio, size, -size, far, near);
	}
};

struct TrPipeline {
	ModelShader	*shader;
	TrRasterizer	rast;
	TrZbuffer	zbuf;
	TrInterpolator	interp;

	struct ModelEntry {
		TrModel *ptr;
		ModelShader *shader;
		std::vector<std::array<ModelShader::vs_out, 3>> VShader_buf;
	};
	std::vector<ModelEntry> model_buf; // maintain Rendering list

	void SetWindow(Window const &wnd)
	{
		rast.set_Window(wnd);
		zbuf.set_Window(wnd);
		rast_buf.resize(wnd.h);
		zbuf.clear();
	}

	void Render(Fbuffer::Color *cbuf)
	{
		for (uint32_t m_id = 0; m_id < model_buf.size(); ++m_id)
			RenderToZbuf(m_id);
		RenderToCbuf(cbuf);
		for (auto &e : model_buf)
			e.VShader_buf.clear();
	}

private:
	std::vector<std::vector<decltype(rast)::rz_out>> rast_buf;

	void RenderToZbuf(uint32_t model_id)
	{
		ModelEntry &entry = model_buf[model_id];
		shader = entry.shader;
		TrModel const &obj = *entry.ptr;

		// vshading
		for (auto const &pr : obj.prim_buf) {
			std::array<ModelShader::vs_out, 3> vs_pr;
			for (int i = 0; i < 3; ++i)
				vs_pr[i] = shader->VShader(pr[i]);
			entry.VShader_buf.push_back(vs_pr);
		}

		// rasterization
		for (int pid = 0; pid < entry.VShader_buf.size(); ++pid) {
			auto const &vs_pr = entry.VShader_buf[pid];
			Vec3 scr_pos[3];
			for (int i = 0; i < 3; ++i)
				scr_pos[i] = ReinterpVec3(vs_pr[i].pos);
			rast.rasterize(scr_pos, pid, model_id, rast_buf);
		}

		// Zbuffering
		for (uint32_t y = 0; y < rast_buf.size(); ++y) {
			for (auto const &rast_el : rast_buf[y])
				zbuf.add_elem(rast_el.x, y, rast_el.fg);
		}

		for (auto &line : rast_buf)
			line.clear();
	}

	Fbuffer::Color RenderFragment(decltype(zbuf)::elem const &e)
	{
		shader = model_buf[e.model_id].shader;
		auto const &pr_vs = model_buf[e.model_id].VShader_buf[e.prim_id];
		Vertex pr_vtx[3] = { pr_vs[0].fs_vtx,
			pr_vs[1].fs_vtx, pr_vs[2].fs_vtx };
		Vertex vtx = interp.Interpolate(pr_vtx, e.bc);
		return shader->FShader(vtx);
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
