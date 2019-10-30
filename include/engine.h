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
	window wnd;
	ppm_img const *tex_img;

	void set_wfobj_entry(wfobj_entry const &e)
	{
		for (std::size_t n = 0; n < e.mesh.inds.size(); n += 3) {
			wfobj_mesh const &m = e.mesh;
			std::array<vertex, 3> vtx = {
				m.verts[m.inds[n]],
				m.verts[m.inds[n + 1]],
				m.verts[m.inds[n + 2]]
			};
			prim_buf.push_back(vtx);
		}
		tex_img = &e.mtl.tex_img;
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
		float fov = 3.1415 / 2; // fov = 90deg
		float far  = 1;
		float near = 0.05;

		float size = std::tan(fov / 2) * near;

		float ratio = float (wnd.w) / wnd.h;
		proj_mat = make_mat4_projection(size * ratio,
			-size * ratio, size, -size, far, near);
	}
};

struct tr_pipeline {
	tex_shader	pl_shader;
	tr_rasterizer	pl_rast;
	tr_zbuffer	pl_zbuf;
	tr_interpolator	pl_interp;

	struct obj_entry {
		tr_pipeline_obj *ptr;
		std::vector<std::array<tex_shader::vs_out, 3>> vshader_buf;
	};
	std::vector<obj_entry> obj_buf; // maintain rendering list

	void set_window(window const &wnd)
	{
		pl_rast.set_window(wnd);
		pl_zbuf.set_window(wnd);
		rast_buf.resize(wnd.h);
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

	void render_to_zbuf(uint32_t oid)
	{
		obj_entry &entry = obj_buf[oid];
		tr_pipeline_obj const &obj = *entry.ptr;

		pl_shader.modelview_mat = obj.modelview_mat;
		pl_shader.proj_mat = obj.proj_mat;
		pl_shader.norm_mat = obj.norm_mat;
		pl_shader.set_window(obj.wnd);

		// TEMPORARY!!!!!!!!!!!
		pl_shader.tex_img = obj_buf[0].ptr->tex_img;

		// vshading
		for (auto const &pr : obj.prim_buf) {
			std::array<tex_shader::vs_out, 3> vs_pr;
			for (int i = 0; i < 3; ++i)
				vs_pr[i] = pl_shader.vshader(pr[i]);
			entry.vshader_buf.push_back(vs_pr);
		}

		// rasterization
		for (int pid = 0; pid < entry.vshader_buf.size(); ++pid) {
			auto const &vs_pr = entry.vshader_buf[pid];
			vec3 scr_pos[3];
			for (int i = 0; i < 3; ++i)
				scr_pos[i] = reinterp_vec3(vs_pr[i].pos);
			pl_rast.rasterize(scr_pos, pid, rast_buf);
		}

		// zbuffering
		for (uint32_t y = 0; y < rast_buf.size(); ++y) {
			for (auto const &re : rast_buf[y]) {
				tr_zbuffer::elem ze = tr_zbuffer::elem {
				        .bc    = {re.bc[0], re.bc[1], re.bc[2]},
					.depth = re.depth,
					.x     = re.x,
					.pid   = re.pid,
					.oid   = oid };
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
			uint32_t ind = y * zbuf_w;
			for (uint32_t x = 0; x < zbuf_w; ++x, ++ind) {
				tr_zbuffer::elem e = pl_zbuf.buf[ind];
				pl_zbuf.buf[ind].depth = tr_zbuffer::free_depth;
				if (e.depth != tr_zbuffer::free_depth)
					cbuf[ind] = render_fragment(e);
			}
		}
	}
};
