#pragma once

#include <vector>
#include <include/geom.h>
#include <include/fbuffer.h>

template <typename _vs_in, typename _fs_in, typename _fs_out>
struct Shader {
	using fs_in  = _fs_in;
	using vs_in  = _vs_in;
	using fs_out = _fs_out;
	struct vs_out {
		Vec4 pos;
		fs_in fs_vtx;
	};
	virtual vs_out VShader(vs_in const &) const = 0;
	virtual fs_out FShader(fs_in const &) const = 0;
};

struct highl_Shader final: public Shader<Vertex, Vertex, Fbuffer::Color> {
private:
	ViewportTransform vp_tr;
public:
	Mat4 modelview_mat;
	Mat4 proj_mat;
	Mat4 norm_mat;

	void SetWindow(Window const &wnd)
	{
		vp_tr.SetWindow(wnd);
	}

	vs_out VShader(vs_in const &in) const override
	{
		vs_out out;
		Vec4 mv_pos = modelview_mat * ToVec4(in.pos);

		out.fs_vtx.pos 	= ToVec3(mv_pos);
		out.pos 	= ToVec4(vp_tr(ToVec3(proj_mat * mv_pos)));
		out.fs_vtx.norm = ToVec3(norm_mat * ToVec4(in.norm));
		out.fs_vtx.tex 	= in.tex;
		return out;
	}

	fs_out FShader(fs_in const &in) const override
	{
		Vec3 light { 0, 0, 100000 };

		Vec3 light_dir = Normalize(light - in.pos);
		float dot = DotProd(light_dir, in.norm);
		dot = std::max((typeof(dot)) 0, dot);
		float intens = 0.1 + 0.4f * dot + 0.4f * std::pow(dot, 32);

		return Fbuffer::Color { uint8_t(intens * 255),
					uint8_t(intens * 150),
					uint8_t(intens * 200), 255 };
	}
};

struct TexHighlShader final: public Shader<Vertex, Vertex, Fbuffer::Color> {
private:
	ViewportTransform vp_tr;
public:
	Mat4 modelview_mat;
	Mat4 proj_mat;
	Mat4 norm_mat;
	Ppm_img const *tex_img;

	void SetWindow(Window const &wnd)
	{
		vp_tr.SetWindow(wnd);
	}

	vs_out VShader(vs_in const &in) const override
	{
		vs_out out;
		Vec4 mv_pos = modelview_mat * ToVec4(in.pos);

		out.fs_vtx.pos 	= ToVec3(mv_pos);
		out.pos 	= ToVec4(vp_tr(ToVec3(proj_mat * mv_pos)));
		out.fs_vtx.norm = ToVec3(norm_mat * ToVec4(in.norm));
		out.fs_vtx.tex 	= in.tex;

		return out;
	}

	fs_out FShader(fs_in const &in) const override
	{
/*
		Vec3 light { 0, 0, 100000 };

		Vec3 light_dir = Normalize(light - in.pos);
		float dot = DotProd(light_dir, in.norm);
		dot = std::max((typeof(dot)) 0, dot);
		float intens = 0.2 + 0.5f * dot + 0.2f * std::pow(dot, 32);
*/
		float intens = 1;

		uint32_t w = tex_img->w;
		uint32_t h = tex_img->h;

		uint32_t x = (in.tex.x * w) + 0.5f;
		uint32_t y = h - (in.tex.y * h) + 0.5f;

		Ppm_img::Color c = {0, 0, 0};
		if (x <= w && y <= h)
			c = tex_img->buf[x + w * y];
		else
			c.r = 255;
		return Fbuffer::Color { uint8_t(c.b * intens),
					uint8_t(c.g * intens),
					uint8_t(c.r * intens), 255 };
	}
};
