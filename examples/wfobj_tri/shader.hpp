#pragma once

#include <include/geom.h>
#include <include/fbuffer.h>
#include <include/wfobj.h>

#include <iostream>

struct vertex {
	vec3 pos;
	vec2 tex;
	vec3 norm;
};

struct tr_vs_out {
	vertex view;
	vec3 screen_pos;
};

struct tr_shader {
	mat4 pos_transf;
	mat4 norm_transf;

	struct viewport_transform screen_transf;

	tr_vs_out vertex_shader(vertex const &in) const
	{
		struct tr_vs_out out;
		out.view.pos  = to_vec3(pos_transf  * to_vec4(in.pos));
		out.view.norm = to_vec3(norm_transf * to_vec4(in.norm));
		out.screen_pos = screen_transf(out.view.pos);
		out.view.tex = in.tex;
		return out;
	}

	fbuffer::color fragment_shader(vertex const &in) const
	{
		vec3 light { -1, 0, -4 };
		vec3 light_dir = normalize(light - in.pos);

		float dot = dot_prod(light_dir, normalize(in.norm));
		dot = std::max(dot, (typeof(dot))0);

		float col = 0.01 + 0.2 * dot + 0.65 * std::pow(dot, 30);

		return fbuffer::color { uint8_t(col * 255),
					uint8_t(col * 255),
					uint8_t(col * 255), 255 };
	}
};

struct tr_rasterizer {
	void rasterize(vec3 const (&tr)[3], std::vector<vec3> &out_barc,
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

		min_f = {std::max(min_f.x, scr_tr.min_scr.x),
			 std::max(min_f.y, scr_tr.min_scr.y)};

		max_f = {std::min(max_f.x, scr_tr.max_scr.x),
			 std::min(max_f.y, scr_tr.max_scr.y)};

		// add z check???

		vec2 r0 = vec2 { tr[0].x, tr[0].y };

		for (int32_t y = min_f.y; y <= max_f.y; ++y) {
			for (int32_t x = min_f.x; x <= max_f.x; ++x) {
				vec2 r_rel = vec2{float(x), float(y)} - r0;
				float det1 = r_rel.x * d2.y - r_rel.y * d2.x;
				float det2 = d1.x * r_rel.y - d1.y * r_rel.x;
				vec3 c = vec3 { (det - det1 - det2) / det,
						det1 / det, det2 / det };
				if (c[0] >= 0 && c[1] >= 0 && c[2] >= 0) {
					out_barc.push_back(c);
					//std::cout << "rasterize: " << r.x << " " << r.y << std::endl;
				}
			}
		}
		//std::cout<<"done---------------------------" << std::endl;
	}
};

struct tr_interpolator {
	void interpolate(tr_vs_out const (&tr)[3], std::vector<vec3> const &barc,
						   std::vector<tr_vs_out> &out)
	{
		for (auto const &c : barc) {
			vertex v;
			v.pos  = vec3 {0, 0, 0};
			v.tex  = vec2 {0, 0};
			v.norm = vec3 {0, 0, 0};
			vec3 scr_p = vec3 { 0, 0, 0 };
			for (int i = 0; i < 3; ++i) {
				v.pos  = v.pos  + c[i] * tr[i].view.pos;
				v.norm = v.norm + c[i] * tr[i].view.norm;
				v.tex  = v.tex  + c[i] * tr[i].view.tex;
				scr_p  = scr_p  + c[i] * tr[i].screen_pos;
			}
			v.norm = normalize(v.norm);
			out.push_back(tr_vs_out{.view = v, .screen_pos = scr_p});
		}
	}
};

struct tr_z_test {
	const float free_depth = std::numeric_limits<float>::max();

	struct zbuf_elem {
		vertex vert;
	};
	std::vector<zbuf_elem> zbuf;
	fbuffer fb;
	void test(std::vector<tr_vs_out> const &in)
	{
		for (auto &e : zbuf)
			e.vert.pos.z = free_depth;

		for (auto const &e : in) {
			uint32_t x = std::uint32_t(e.screen_pos.x - 0.5f);
			uint32_t y = std::uint32_t(e.screen_pos.y - 0.5f);
			uint32_t ind = x + fb.xres * y;
			//if (x >= fb.xres || y >= fb.yres)
			//	continue;
			if (zbuf[ind].vert.pos.z > e.view.pos.z)
				zbuf[ind].vert = e.view;
		}
	}
};

struct tr_pipeline {
	struct tr_shader	shader;
	struct tr_rasterizer	rast;
	struct tr_z_test	z_test;
	struct tr_interpolator	interp;

	mesh data;
	std::vector<vertex>	vertex_buf;
	std::vector<tr_vs_out>	vshader_buf;
	std::vector<vec3>	barc_buf;
	std::vector<tr_vs_out>	interp_buf;

	void init()
	{
		z_test.fb.init("/dev/fb0");
		z_test.zbuf.resize((z_test.fb.xres * z_test.fb.yres));

		mat4 model = make_mat4_translate(vec3 {0, 0.2, 0});

		model = make_mat4_rotate(vec3 {0, 0, 1}, 3.1415) * model;
		model = make_mat4_rotate(vec3 {0, 1, 0}, 0.7) * model;
		model = make_mat4_rotate(vec3 {1, 0, 0}, 0.5) * model;
		model = make_mat4_translate(vec3 {0, 0, -6.0f}) * model; // no lookat

		//mat4 model = make_mat4_rotate(vec3 {0, 0, 1}, 3.1415);
		//model = make_mat4_rotate(vec3 {0, 1, 0}, 0.5) * model;
		//model = make_mat4_translate( vec3 {0.0, 0.8, -2.0f}) * model;
		//
		//mat4 view = make_mat4_view( vec3 {0, -0.5f, -1.0f}, vec3 {0, -0.5f, 0}, vec3 {0, 1.0f,  0});
		//mat4 model = make_mat4_identy();
		mat4 view = make_mat4_identy();

		float winsize = 1.0 / 2.5;
		mat4 proj = make_mat4_projection(winsize, -winsize, winsize, -winsize, 1, 3);
		shader.pos_transf = proj * (view * model);

		shader.norm_transf = transpose(inverse(shader.pos_transf));
		shader.screen_transf.set(0, 0, z_test.fb.xres - 1,
					 z_test.fb.xres - 1, -2000, 2000);
	}

	void load_data()
	{
		for (std::size_t n = 0; n < data.inds.size(); n += 3) {
			mesh::vertex vert[3] = {
				data.verts[data.inds[n]],
				data.verts[data.inds[n + 1]],
				data.verts[data.inds[n + 2]]
			};
			vertex geom_vert;
			for (int i = 0; i < 3; ++i) {
				geom_vert.pos  = vert[i].pos;
				geom_vert.norm = vert[i].norm;
				geom_vert.tex  = vert[i].tex;
				vertex_buf.push_back(geom_vert);
			}
		}
	}

	void process()
	{
		//int i = 0;
		for (auto const &v : vertex_buf) {
		//	if (i++ == 3)
		//		 break;
			vshader_buf.push_back(shader.vertex_shader(v));
		}

		for (std::size_t i = 0; i < vshader_buf.size(); i += 3) {
			vec3 tr_vec[3] = {    vshader_buf[i]    .screen_pos,
					      vshader_buf[i + 1].screen_pos,
					      vshader_buf[i + 2].screen_pos };
			tr_vs_out info[3] = { vshader_buf[i],
					      vshader_buf[i + 1],
					      vshader_buf[i + 2] };
			rast.rasterize(tr_vec, barc_buf, shader.screen_transf);
			interp.interpolate(info, barc_buf, interp_buf);
			barc_buf.clear();
		}
		vshader_buf.clear();

		z_test.test(interp_buf);
		interp_buf.clear();

		for (std::size_t i = 0; i < z_test.zbuf.size(); ++i) {
			vertex vert = z_test.zbuf[i].vert;
			if (vert.pos.z != z_test.free_depth) {
				z_test.fb.buf[i] = shader.fragment_shader(vert);
			}
		}
	}
};
