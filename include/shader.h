#pragma once

#include <include/pipeline.h>

#if 0
// shaders now in pipeline
{{{

struct ModelShader : public Shader<Vertex, Vertex, Fbuffer::Color> {
	PpmImg const *tex_img;

	void set_window(Window const &wnd) override
	{
		vp_tr.set_window(wnd);

		float fov = 3.1415 / 3;
		float far  = wnd.f;
		float near = wnd.n;

		float size = std::tan(fov / 2) * near;
		float ratio = float(wnd.w) / wnd.h;

		proj_mat = MakeMat4Projection(size * ratio,
			-size * ratio, size, -size, far, near);
	}

	void set_view(Mat4 const &view, float scale) override
	{
		modelview_mat = MakeMat4Scale(scale) * view;
		norm_mat = Transpose(Inverse(modelview_mat));

		light = Normalize(ReinterpVec3(norm_mat *
				(Vec4{1, 1, 1, 1})));
	}

	VsOut VShader(VsIn const &in) const override
	{
		VsOut out;
		Vec4 mv_pos = modelview_mat * ToVec4(in.pos);

		out.fs_vtx.pos 	= ToVec3(mv_pos);
		out.pos 	= ToVec4(vp_tr(ToVec3(proj_mat * mv_pos)));
		out.fs_vtx.norm = ToVec3(norm_mat * ToVec4(in.norm));
		out.fs_vtx.tex 	= in.tex;

		return out;
	}

	virtual FsOut FShader(FsIn const &in) const override = 0;

private:
	ViewportTransform vp_tr;
	Mat4 modelview_mat;
	Mat4 proj_mat;
	Mat4 norm_mat;

	Vec3 light;
};


struct TexShader final: public ModelShader {
public:
	FsOut FShader(FsIn const &in) const override
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
	FsOut FShader(FsIn const &in) const override
	{
		Vec3 light_dir = light;
		Vec3 norm = in.norm;
		Vec3 pos = Normalize(in.pos);

		float dot_d = DotProd(light_dir, norm);
		float dot_s = DotProd(light - 2 * dot_d * norm, pos);

		dot_d = std::max(0.0f, dot_d);
		dot_s = std::max(0.0f, dot_s);
		float intens = 0.4f +
			       0.25f * dot_d +
			       0.35f * std::pow(dot_s, 32);

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

#endif
