#pragma once
#include <include/geom.h>
#include <include/fbuffer.h>
#include <vector>

template <typename _vs_in, typename _fs_in, typename _fs_out>
struct shader {
	using fs_in  = _fs_in;
	using vs_in  = _vs_in;
	using fs_out = _fs_out;
	struct vs_out {
		vec4 pos;
		fs_in fs_vtx;
	};
	virtual vs_out vshader(vs_in const &) const = 0;
	virtual fs_out fshader(fs_in const &) const = 0;
};

struct def_shader final: public shader<vertex, vertex, fbuffer::color> {
private:
	viewport_transform vp_tr;
public:
	mat4 modelview_mat;
	mat4 proj_mat;
	mat4 norm_mat;

	void set_window(window const &wnd)
	{
		vp_tr.set_window(wnd);
	}

	vs_out vshader(vs_in const &in) const override
	{
		vs_out out;
		vec4 mv_pos = modelview_mat * to_vec4(in.pos);

		out.fs_vtx.pos 	= to_vec3(mv_pos);
		out.pos 	= to_vec4(vp_tr(to_vec3(proj_mat * mv_pos)));
		out.fs_vtx.norm = to_vec3(norm_mat * to_vec4(in.norm));
		out.fs_vtx.tex 	= in.tex;
		return out;
	}

	fs_out fshader(fs_in const &in) const override
	{
		vec3 light { 0, 0, 100000 };

		vec3 light_dir = normalize(light - in.pos);
		float dot = dot_prod(light_dir, in.norm);
		dot = std::max((typeof(dot)) 0, dot);

		float intens = 0.1 + 0.4f * dot + 0.4f * std::pow(dot, 32);

		return fbuffer::color { uint8_t(intens * 255),
					uint8_t(intens * 150),
					uint8_t(intens * 200), 255 };
	}
};