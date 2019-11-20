#pragma once

#include <include/pipeline.h>

//#define HACK_TRSHADER_NO_BOUNDS

struct ModelShader : public Shader<Vertex, Vertex, Fbuffer::Color> {
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
		Mat4 scale_mat = MakeMat4Scale(Vec3{scale, scale, scale});
		modelview_mat = view * scale_mat;

		norm_mat = Transpose(Inverse(modelview_mat));

		light = Normalize(ReinterpVec3(norm_mat *
				(Vec4{1, 1, 1, 1})));
	}

	void set_tex_img(PpmImg const *tex_img_)
	{
		tex_img = tex_img_;
		tex_buf = &tex_img->buf[0];
		tex_w = tex_img->w;
		tex_h = tex_img->h;
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

	PpmImg::Color FShaderGetColor(Vec2 const &tex) const
	{
		int32_t w = tex_w;
		int32_t h = tex_h;

		int32_t x = (tex.x * w) + 0.5f;
		int32_t y = h - (tex.y * h) + 0.5f;

#ifndef HACK_TRSHADER_NO_BOUNDS
		if (x >= w) x = w - 1;
		if (y >= h) y = h - 1;
		if (x < 0)  x = 0;
		if (y < 0)  y = 0;
#endif
		return tex_img->buf[x + w * y];
	}

	virtual FsOut FShader(FsIn const &in) const override = 0;

protected:
	ViewportTransform vp_tr;
	Mat4 modelview_mat;
	Mat4 proj_mat;
	Mat4 norm_mat;

	Vec3 light;
	PpmImg const *tex_img;
	PpmImg::Color const *tex_buf;
	int32_t tex_w, tex_h;
};

struct TexShader final: public ModelShader {
public:
	FsOut FShader(FsIn const &in) const override
	{
		auto c = FShaderGetColor(in.tex);
		return Fbuffer::Color { c.b, c.g, c.r, 255 };
	}
};

struct TexHighlShader final: public ModelShader {
public:
	FsOut FShader(FsIn const &in) const override
	{
		Vec3 light_dir = light;
		Vec3 norm = in.norm;
		Vec3 pos = Normalize(in.pos);

		float dot_d = DotProd(light_dir, norm);
		float dot_s = DotProd(light - 2 * dot_d * norm, pos);

		dot_d = std::max(0.0f, dot_d);
		dot_s = std::max(0.0f, dot_s);
		float intens = 0.35f +
			       0.24f * dot_d +
			       0.40f * std::pow(dot_s, 32);

		auto c = FShaderGetColor(in.tex);
		return Fbuffer::Color { uint8_t(c.b * intens),
					uint8_t(c.g * intens),
					uint8_t(c.r * intens), 255 };
	}
};
