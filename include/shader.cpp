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

	struct vec4 fragment_shader(vertex const &in) const
	{
		return vec4 { 0.9f, 0.9f, 0.9f, 1f };
	}
};


