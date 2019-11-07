#pragma once

#include <vector>
#include <include/geom.h>
#include <include/fbuffer.h>

#include <iostream>

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


struct ModelShader : public Shader<Vertex, Vertex, Fbuffer::Color> {
	ViewportTransform vp_tr;

	Mat4 modelview_mat;
	Mat4 proj_mat;
	Mat4 norm_mat;
	PpmImg const *tex_img;

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

		std::cout << "mv.z:" << mv_pos.z << " scr.z: " << out.pos.z << std::endl;

		return out;
	}

	virtual fs_out FShader(fs_in const &in) const override = 0;
};


struct TexShader final: public ModelShader {
public:
	fs_out FShader(fs_in const &in) const override
	{
		int32_t w = tex_img->w;
		int32_t h = tex_img->h;

		int32_t x = (in.tex.x * w) + 0.5f;
		int32_t y = h - (in.tex.y * h) + 0.5f;

		if (x >= w)	x = w - 1;
		if (y >= h)	y = h - 1;
		if (x < 0)	x = 0;
		if (y < 0)	y = 0;

		PpmImg::Color c = tex_img->buf[x + w * y];
		return Fbuffer::Color { c.b, c.g, c.r, 255 };
	}
};

struct TexHighlShader final: public ModelShader {
public:
	Vec3 light;
	fs_out FShader(fs_in const &in) const override
	{
		Vec3 light_dir = light;
		Vec3 norm = in.norm;
		Vec3 pos = Normalize(in.pos);

		float dot_d = DotProd(light_dir, norm);
		float dot_s = DotProd(light - 2 * dot_d * norm, pos);

		dot_d = std::max(0.0f, dot_d);
		dot_s = std::max(0.0f, dot_s);
		float intens = 0.2f + 0.3f * dot_d + 0.45f * std::pow(dot_s, 32);

		int32_t w = tex_img->w;
		int32_t h = tex_img->h;

		int32_t x = (in.tex.x * w) + 0.5f;
		int32_t y = h - (in.tex.y * h) + 0.5f;

		if (x >= w)	x = w - 1;
		if (y >= h)	y = h - 1;
		if (x < 0)	x = 0;
		if (y < 0)	y = 0;

		PpmImg::Color c = tex_img->buf[x + w * y];
		return Fbuffer::Color { uint8_t(c.b * intens),
					uint8_t(c.g * intens),
					uint8_t(c.r * intens), 255 };
	}
};
