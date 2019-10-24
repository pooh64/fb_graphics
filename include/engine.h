#pragma once

#include <include/geom.h>
#include <include/fbuffer.h>
#include <include/wfobj.h>

#include <vector>
#include <cassert>

struct window {
	uint32_t x, y, w, h, f, n;
};

struct vertex
{
	vec3 pos;
	vec2 tex;
	vec3 norm;
};

struct screen_vertex
{
	vertex view;
	vec3 scr_pos;
};

struct viewport_transform {
	vec3 scale, offs;
	vec3 min_scr;
	vec3 max_scr;

	inline void set_window(window const &wnd)
	{
		float x = wnd.x, y = wnd.y, w = wnd.w, h = wnd.h,
		      f = wnd.f, n = wnd.n;

		scale = vec3 { w/2,     h/2,     (f-n)/2 };
		offs  = vec3 { x + w/2, y + h/2, (f+n)/2 };

		min_scr = offs - scale;
		max_scr = offs + scale;
	}

	inline vec3 operator()(vec3 const &v) const
	{
		vec3 ret;
		for (int i = 0; i < 3; ++i)
			ret[i] = v[i] * scale[i] + offs[i];
		return ret;
	}
};

struct tr_zbuffer
{
	float const free_depth = std::numeric_limits<float>::max();
	struct elem {
	//	uint32_t x;
		vertex vtx;
		float depth;
	};

	window wnd;
	//std::vector<std::vector<elem>> buf;
	std::vector<elem> buf;

	void set_window(window const &_wnd)
	{
		wnd = _wnd;
		buf.resize(wnd.w * wnd.h);
	}

	void clear()
	{
		for (auto &e : buf) {
			e.vtx.tex = { 0, 0};
			e.vtx.norm = { 1, 0, 0 };
			e.vtx.pos = { 0, 0, 0 };
			e.depth = free_depth;
		}
	}

	void add(uint32_t x, uint32_t y, float depth, vertex const &vtx)
	{
		// check in rasterizer
		std::size_t ind = x + wnd.w * y;
		if (buf[ind].depth > depth) {
			buf[ind].depth = depth;
			buf[ind].vtx = vtx;
		}
	}
};


struct tr_shader
{
	mat4 modelview_mat;
	mat4 proj_mat;
	mat4 norm_mat;

	struct viewport_transform vp_tr;

	screen_vertex vertex_shader(vertex const &in)
	{
		struct screen_vertex out;
		vec4 view_pos = modelview_mat * to_vec4(in.pos);

		out.view.pos = to_vec3(view_pos);
		out.view.norm = to_vec3(norm_mat * to_vec4(in.norm));
		out.scr_pos = vp_tr(to_vec3(proj_mat * view_pos));
		out.view.tex = in.tex;
		return out;
	}

	fbuffer::color fragment_shader(vertex const &in)
	{
		vec3 light { 0, 0, 100000 };

		vec3 light_dir = normalize(light - in.pos);
		float dot = dot_prod(light_dir, in.norm);
		dot = std::max((typeof(dot)) 0, dot);

		float intens = 0.1 + 0.4f * dot + 0.45f * std::pow(dot, 32);

		return fbuffer::color { uint8_t(intens * 255),
					uint8_t(intens * 150),
					uint8_t(intens * 200), 255 };
	}
};

struct tr_rasterizer {
	void rasterize(vec3 const (&tr)[3], std::vector<vec3> &baryc,
		       viewport_transform const &scr_tr)
	{
		vec3 d1_3 = tr[1] - tr[0];
		vec3 d2_3 = tr[2] - tr[0];
		vec2 d1 = { d1_3.x, d1_3.y };
		vec2 d2 = { d2_3.x, d2_3.y };

		float det = d1.x * d2.y - d1.y * d2.x;
		if (det <= 0)
			return;

	 	vec2 min_f{std::min(tr[0].x, std::min(tr[1].x, tr[2].x)),
			   std::min(tr[0].y, std::min(tr[1].y, tr[2].y))};

		vec2 max_f{std::max(tr[0].x, std::max(tr[1].x, tr[2].x)),
			   std::max(tr[0].y, std::max(tr[1].y, tr[2].y))};

		min_f = {std::max(min_f.x, scr_tr.min_scr.x + 0.5f),
			 std::max(min_f.y, scr_tr.min_scr.y + 0.5f)};

		max_f = {std::min(max_f.x, scr_tr.max_scr.x + 0.5f),
			 std::min(max_f.y, scr_tr.max_scr.y + 0.5f)};

		vec2 r0 = vec2 { tr[0].x, tr[0].y };

		for (int32_t y = min_f.y; y <= max_f.y; ++y) {
			for (int32_t x = min_f.x; x <= max_f.x; ++x) {
				//assert((x >= 0) && (y >= 0));
				vec2 r_rel = vec2{float(x), float(y)} - r0;
				float det1 = r_rel.x * d2.y - r_rel.y * d2.x;
				float det2 = d1.x * r_rel.y - d1.y * r_rel.x;
				vec3 c = vec3 { (det - det1 - det2) / det,
						det1 / det, det2 / det };
				if (c[0] >= 0 && c[1] >= 0 && c[2] >= 0)
					baryc.push_back(c);
			}
		}
	}
};

struct tr_interpolator {
	void interpolate(screen_vertex const (&tr)[3], std::vector<vec3> const &baryc,
						   std::vector<screen_vertex> &out)
	{
		for (auto const &c : baryc) {
			vertex v;
			v.pos  = vec3 {0, 0, 0};
			v.tex  = vec2 {0, 0};
			v.norm = vec3 {0, 0, 0};
			vec3 scr_p = vec3 { 0, 0, 0 };
			for (int i = 0; i < 3; ++i) {
				v.pos  = v.pos  + c[i] * tr[i].view.pos;
				v.norm = v.norm + c[i] * tr[i].view.norm;
				v.tex  = v.tex  + c[i] * tr[i].view.tex;
				scr_p  = scr_p  + c[i] * tr[i].scr_pos;
			}
			v.norm = normalize(v.norm);
			out.push_back(screen_vertex{.view = v, .scr_pos = scr_p});
		}
	}
};

struct tr_pipeline {
	struct tr_shader	shader;
	struct tr_rasterizer	rast;
	struct tr_zbuffer	zbuffer;
	struct tr_interpolator	interp;

	std::vector<vertex>		vertex_buf;
	std::vector<screen_vertex>	vshader_buf;
	std::vector<vec3>		baryc_buf;
	std::vector<screen_vertex>	interp_buf;

	void set_view(float xang, float yang, float pos)
	{
		mat4 model = make_mat4_translate(vec3 {0, 0, 0});
		model = make_mat4_rotate(vec3 {0, 1, 0}, yang) * model;
		model = make_mat4_rotate(vec3 {1, 0, 0}, xang) * model;
		model = make_mat4_translate(vec3 {0, 0, -pos}) * model;

		mat4 view = make_mat4_identy();

		shader.modelview_mat = view * model;
		shader.norm_mat = transpose(inverse(shader.modelview_mat));

		float winsize = 1.0;
		float ratio = float (zbuffer.wnd.w) / zbuffer.wnd.h;
		shader.proj_mat = make_mat4_projection(winsize * ratio,
				-winsize * ratio, winsize, -winsize, 1, 10);
	}

	tr_pipeline(window const &wnd)
	{
		zbuffer.set_window(wnd);
		zbuffer.clear();

		shader.vp_tr.set_window(wnd);
	}

	void load_mesh(mesh const &data)
	{
		for (std::size_t n = 0; n < data.inds.size(); n += 3) {
			mesh::vertex vtx[3] = {
				data.verts[data.inds[n]],
				data.verts[data.inds[n + 1]],
				data.verts[data.inds[n + 2]]
			};
			vertex geom_vtx;
			for (int i = 0; i < 3; ++i) {
				geom_vtx.pos  = vtx[i].pos;
				geom_vtx.norm = vtx[i].norm;
				geom_vtx.tex  = vtx[i].tex;
				vertex_buf.push_back(geom_vtx);
			}
		}
	}

	void render(fbuffer::color *color_buf)
	{
		for (auto const &v : vertex_buf)
			vshader_buf.push_back(shader.vertex_shader(v));

		for (std::size_t i = 0; i < vshader_buf.size(); i += 3) {
			vec3 tr_vec[3] = {    vshader_buf[i]    .scr_pos,
					      vshader_buf[i + 1].scr_pos,
					      vshader_buf[i + 2].scr_pos };
			screen_vertex info[3] = { vshader_buf[i],
						  vshader_buf[i + 1],
						  vshader_buf[i + 2] };
			rast.rasterize(tr_vec, baryc_buf, shader.vp_tr);
			interp.interpolate(info, baryc_buf, interp_buf);
			baryc_buf.clear();
		}
		vshader_buf.clear();

		zbuffer.clear();
		for (auto const &e : interp_buf) {
			//assert((e.scr_pos.x >= 0) && (e.scr_pos.y >= 0));
			uint32_t x = e.scr_pos.x + 0.5f;
			uint32_t y = e.scr_pos.y + 0.5f;
			float depth = e.scr_pos.z;
			zbuffer.add(x, y, depth, e.view);
		}
		interp_buf.clear();

		for (std::size_t i = 0; i < zbuffer.buf.size(); ++i) {
			tr_zbuffer::elem e = zbuffer.buf[i];
			if (e.depth != zbuffer.free_depth) {
				color_buf[i] = shader.fragment_shader(e.vtx);
			}
		}
	}
};
