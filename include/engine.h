#pragma once

#include <iostream>
#include <vector>
#include <array>
#include <typeinfo>

#include <include/tile.h>
#include <include/wfobj.h>
#include <include/geom.h>
#include <include/shader.h>
#include <include/rasterizer.h>
#include <include/zbuffer.h>
#include <include/interpolator.h>
#include <include/sync_threadpool.h>

struct TrModel {
public:
	std::vector<TrPrim> prim_buf;
	ModelShader *shader;

	void SetWfobj(Wfobj const &e)
	{
		for (std::size_t n = 0; n < e.mesh.inds.size(); n += 3) {
			Wfobj::Mesh const &m = e.mesh;
			TrPrim prim = {
				m.verts[m.inds[n]],
				m.verts[m.inds[n + 1]],
				m.verts[m.inds[n + 2]]
			};
			prim_buf.push_back(prim);
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
		float far  = wnd.f;
		float near = wnd.n;

		float size = std::tan(fov / 2) * near;

		float ratio = float (wnd.w) / wnd.h;
		shader->proj_mat = MakeMat4Projection(size * ratio,
			-size * ratio, size, -size, far, near);
	}
};

struct TrPipeline {
	TrRasterizer	rast;
	TrZbuffer	zbuf;
	TrInterpolator	interp;
	TileTransform	tl_tr;
	Window		wnd;
	SyncThreadpool	sync_tp;

	using VshaderBuf    = std::vector<std::array<ModelShader::vs_out, 3>>;
	using RasterizerBuf = std::vector<std::vector<decltype(rast)::rz_out>>;
	using PrimBuf       = std::vector<TrPrim>;

	struct ModelEntry {
		TrModel *ptr;
		ModelShader *shader;
		VshaderBuf vshader_buf;
	};
	std::vector<ModelEntry> model_buf; // maintain Rendering list

	TrPipeline(unsigned n_threads) :
		sync_tp(n_threads),
		rast_buffers(n_threads),
		vshader_buffers(n_threads)
	{

	}

	void SetWindow(Window const &_wnd)
	{
		wnd = _wnd;
		uint32_t w_tiles = ToTilesUp(wnd.w);
		uint32_t h_tiles = ToTilesUp(wnd.h);

		rast.set_Window(wnd);

		tl_tr.w_tiles = w_tiles;
		zbuf.buf.resize(w_tiles * h_tiles);

		for (auto &rast_buf : rast_buffers) {
			rast_buf.resize(w_tiles * h_tiles);
			for (auto &tile : rast_buf)
				tile.reserve(TILE_SIZE * TILE_SIZE *
						sizeof(tile[0]) * 8);
		}

		zbuf.clear();
	}

	void Render(Fbuffer::Color *cbuf)
	{
		for (auto &model : model_buf)
			model.vshader_buf.resize(model.ptr->prim_buf.size());

		for (uint32_t m_id = 0; m_id < model_buf.size(); ++m_id)
			RenderToZbuf(m_id);
		RenderToCbuf(cbuf);

		for (auto &model : model_buf)
			model.vshader_buf.clear();
	}

	struct Task {
		uint32_t beg;
		uint32_t end;
	};

private:
	std::vector<RasterizerBuf> rast_buffers;
	std::vector<VshaderBuf>    vshader_buffers;
	std::vector<Task>	   task_buf;

	ModelShader *cur_shader;
	VshaderBuf  *cur_vshader_buf;
	PrimBuf     *cur_prim_buf;

	void VshaderStage(uint32_t model_id);
	void VshaderRoutineProcess(int, int);
	void VshaderRoutineCollect(int, int);

	void RasterizerStage(uint32_t model_id);

	void RenderToZbuf(uint32_t model_id);
	void RenderToCbuf(Fbuffer::Color *cbuf);
};
