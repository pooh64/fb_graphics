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

	void SetWindow(Window const &wnd)
	{
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
		std::vector<std::array<ModelShader::vs_out, 3>> vshader_buf;
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
			e.vshader_buf.clear();
	}

private:
	std::vector<std::vector<decltype(rast)::rz_out>> rast_buf;

	void RenderToZbuf(uint32_t model_id);
	void RenderToCbuf(Fbuffer::Color *cbuf);
};
