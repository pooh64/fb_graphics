#pragma once

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
		out.tex = in.tex;
		return out;
	}

	fbuffer::color fragment_shader(vertex const &in) const
	{
		return fbuffer::color { 200, 200, 200, 255 };
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

	 	vec2 min_f{std::min(tr[0].x, std::min(tr[1].x, tr[2].x)),
			   std::min(tr[0].y, std::min(tr[1].y, tr[2].y))};

		vec2 max_f{std::max(tr[0].x, std::max(tr[1].x, tr[2].x)),
			   std::max(tr[0].y, std::max(tr[1].y, tr[2].y))};

		min_f = {std::min(min_f.x, scr_tr.min_scr.x),
			 std::min(min_f.y, scr_tr.min_scr.y)};

		max_f = {std::max(max_f.x, scr_tr.max_scr.x)
			 std::max(max_f.y, scr_tr.max_scr.y)};

		vec2 r;
		vec2 r0 = vec2 { tr[0].x, tr[0].y };
		float det = d1.x * d2.y - d1.y * d2.x;
		if (det == 0)
			return;

		for (r.y = min_f.y; r.y <= max_f.y; r.y++) {
			for (r.x = min_f.x; r.x <= max_f.x; r.x++) {
				vector2d r_rel = r - r0;
				float det1 = r_rel.x * d2.y - r_rel.y * d2.x;
				float det2 = d1.x * r_rel.y - d1.y * r_rel.x;
				vec3 c = vec3 { (det - det1 - det2) / det,
						det1 / det, det2 / det };
				if (c[0] < 0 || c[1] < 0 || c[2] < 0)
					continue;
				out_barc.push_back(c);
			}
		}
	}
};

struct tr_interpolator {
	void interpolate(vertex const (&tr)[3], std::vector<vec3> const &barc,
						std::vector<vertex> &out)
	{
		for (auto const &c : barc) {
			vertex v = { .pos = 0, .tex = 0, .norm = 0 };
			for (int i = 0; i < 3; ++i) {
				v.pos  += c[i] * tr[i].pos;
				v.norm += c[i] * tr[i].norm;
				v.tex  += c[i] * tr[i].tex;
			}
			n.norm = normalize(n.norm);
		}
	}
};

struct tr_z_test {
	struct zbuf_elem {
		bool free;
		vertex vert;
	};
	std::vector<zbuf_elem> zbuf; // init
	void test(std::vector<vertex> const &in)
	{
		for (auto &e : zbuf)
			e.free = true;

		for (auto const &v : in) {
			std::size_t j = std::size_t(v.pos.x + 0.5f) *
					std::size_t(v.pos.y + 0.5f);
			if (j > zbuf.size)
				continue;
			if (zbuf[j].free == true) {
				zbuf[j].free = false;
				zbuf[j].vert = v;
			} else {
				if (zbuf[j].vertex.pos.z > v.pos.z) ////// ????????????????????????????????
					zbuf[j].vert = v;
			}
		}
	}
};

struct tr_pipeline {
	struct tr_shader	shader;
	struct tr_rasterizer	rast;
	struct tr_z_test	z_test;
	struct tr_interpolator	interp;
	struct fbuffer		fb;

	mesh data;
	std::vector<vertex>	vertex_buf;
	std::vector<tr_vs_out>	vshader_buf;
	std::vector<vec3>	barc_buf;
	std::vector<vertex>	interp_buf;

	void init()
	{
		fb.init("/dev/fb0");

		mat4 view = make_mat4_view( vec3 {0, 0, -2},
					    vec3 {0, 0,  0},
					    vec3 {0, 1,  0});
		mat4 proj = make_mat4_projection(1, -1, 1, -1, 1, 3);
		shader.pos_transf = proj * view;
		shader.norm_transf = transpose(inverse(shader.pos_transf));
		shader.screen_transf.set(0, 0, fb.xres - 1, fb.xres - 1);

		z_test.zbuf.resize((fb.xres * fb.yres));
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
		for (auto const &v : vertex_buf)
			vshader_buf.push_back(shader.vertex_shader(v));

		for (std::size_t i = 0; i < vshader_buf.size(); i += 3) {
			vertex tr[3] = { vshader_buf[i], vshader_buf[i + 1],
					 vshader_buf[i + 2] };
			rast.rasterize(tr, barc_buf, shader.screen_transf);
			interp.interpolate(tr, barc_buf, interp_buf);
			barc_buf.clear();
		}
		vshader_buf.clear();

		z_test.test(interp_buf);
		interp_buf.clear();

		for (std::size_t i = 0; i < z_test.zbuf.size(); ++i) {
			bool free   = z_test.zbuf[i].free;
			vertex vert = z_test.zbuf[i].vert;
			if (free == false) {
				fb[vert.pos.y + 0.5f][vert.pos.x + 0.5f] =
					shader.fragment_shader(vert);
			}
		}
	}
};




















