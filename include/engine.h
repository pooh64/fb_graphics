#pragma once

#include <iostream>
#include <vector>
#include <array>

#include <include/wfobj.h>
#include <include/geom.h>
#include <include/shader.h>
#include <include/rasterizer.h>
#include <include/zbuffer.h>
#include <include/interpolator.h>

struct tr_pipeline_obj {
	mat4 modelview_mat;
	mat4 proj_mat;
	mat4 norm_mat;
	std::vector<std::array<vertex, 3>> prim_buf;
	void *tex_img;
	window wnd;

	void load_mesh(mesh const &data)
	{
		for (std::size_t n = 0; n < data.inds.size(); n += 3) {
			std::array<vertex, 3> vtx = {
				data.verts[data.inds[n]],
				data.verts[data.inds[n + 1]],
				data.verts[data.inds[n + 2]]
			};
			prim_buf.push_back(vtx);
		}
	}

	void set_view(float xang, float yang, float pos)
	{
		modelview_mat = make_mat4_translate(vec3 {0, 0, 0});
		modelview_mat = make_mat4_rotate(vec3 {0, 1, 0}, yang)
			* modelview_mat;
		modelview_mat = make_mat4_rotate(vec3 {1, 0, 0}, xang)
			* modelview_mat;
		modelview_mat = make_mat4_translate(vec3 {0, 0, -pos})
			* modelview_mat;

		mat4 view = make_mat4_identy();

		modelview_mat = view * modelview_mat;
		norm_mat = transpose(inverse(modelview_mat));
	}

	void set_window(window const &_wnd)
	{
		wnd = _wnd;
		float winsize = 1.0;
		float ratio = float (wnd.w) / wnd.h;
		proj_mat = make_mat4_projection(winsize * ratio,
			-winsize * ratio, winsize, -winsize, wnd.n, wnd.f);
	}
};


struct tr_pipeline {
/*
	struct zb_out {
		uint32_t x;
		uint32_t pid;
		float[3] bc;
	};
*/

	struct obj_entry {
		tr_pipeline_obj *ptr;
		std::vector<std::array<def_shader::vs_out, 3>> vshader_buf;
		//std::vector<zb_out> zbuf_buf;
	};
	std::vector<obj_entry> obj_buf; // maintain rendering list

	void set_window(window const &wnd)
	{
		pl_rast.set_window(wnd);
		pl_zbuf.set_window(wnd);
		rast_buf.resize(wnd.h); // ?reserve
		pl_zbuf.clear();
	}

	void render(fbuffer::color *cbuf)
	{
		for (uint32_t oid = 0; oid < obj_buf.size(); ++oid)
			render_to_zbuf(oid);
		render_to_cbuf(cbuf);
		for (auto &e : obj_buf)
			e.vshader_buf.clear();
	}

private:
	std::vector<std::vector<tr_rasterizer::rz_out>> rast_buf;

	def_shader	pl_shader;
	tr_rasterizer	pl_rast;
	tr_zbuffer	pl_zbuf;
	tr_interpolator	pl_interp;

	void render_to_zbuf(uint32_t oid)
	{
		obj_entry &entry = obj_buf[oid];
		tr_pipeline_obj const &obj = *entry.ptr;

		pl_shader.modelview_mat = obj.modelview_mat;
		pl_shader.proj_mat = obj.proj_mat;
		pl_shader.norm_mat = obj.norm_mat;
		pl_shader.set_window(obj.wnd);

		// vshading
		for (auto const &pr : obj.prim_buf) {
			std::array<def_shader::vs_out, 3> vs_pr;
			for (int i = 0; i < 3; ++i)
				vs_pr[i] = pl_shader.vshader(pr[i]);
			entry.vshader_buf.push_back(vs_pr);
		}

		// rasterization
		for (int pid = 0; pid < entry.vshader_buf.size(); ++pid) {
			auto const &vs_pr = entry.vshader_buf[pid];
			vec3 scr_pos[3];
			for (int i = 0; i < 3; ++i)
				scr_pos[i] = vec3_reinterp(vs_pr[i].pos);
			pl_rast.rasterize(scr_pos, pid, rast_buf);
		}

		// zbuffering
		for (uint32_t y = 0; y < rast_buf.size(); ++y) {
			for (auto const &re : rast_buf[y]) {
				tr_zbuffer::elem ze = tr_zbuffer::elem {
					.depth = re.depth,
					.x     = re.x,
					.pid   = re.pid,
					.oid   = oid,
					.bc    {re.bc[0], re.bc[1], re.bc[2]}};
				pl_zbuf.add_elem(y, ze);
			}
		}

		for (auto &line : rast_buf)
			line.clear();
	}

	fbuffer::color render_fragment(tr_zbuffer::elem const &e)
	{
		auto const &pr_vs = obj_buf[e.oid].vshader_buf[e.pid];
		vertex pr_vtx[3] = { pr_vs[0].fs_vtx,
			pr_vs[1].fs_vtx, pr_vs[2].fs_vtx };
		vertex vtx = pl_interp.interpolate(pr_vtx, e.bc);
		return pl_shader.fshader(vtx);
	}

	void render_to_cbuf(fbuffer::color *cbuf)
	{
		uint32_t zbuf_w = pl_zbuf.wnd.w;
		uint32_t zbuf_h = pl_zbuf.wnd.h;
		for (uint32_t y = 0; y < zbuf_h; ++y) {
			for (uint32_t x = 0; x < zbuf_w; ++x) {
				uint32_t ind = x + zbuf_w * y;
				tr_zbuffer::elem fr = pl_zbuf.buf[ind];
				pl_zbuf.buf[ind].depth = tr_zbuffer::free_depth;
				if (fr.depth == tr_zbuffer::free_depth)
					continue;
				cbuf[ind] = render_fragment(fr);
			}
		}
	}
};
