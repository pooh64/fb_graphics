#pragma once

#include <cstdint>
#include <cmath>
#include <xmmintrin.h>
#include <smmintrin.h>

#define GEOM_XMM

struct Vec2 {
	union {
		struct {
			float x, y;
		};
		float data[2];
	};

	float &operator[](int i) noexcept
	{
		return data[i];
	}

	float const &operator[](int i) const noexcept
	{
		return data[i];
	}
};

inline Vec2 operator+(Vec2 const &v1, Vec2 const &v2)
{
	Vec2 res;
	for (int i = 0; i < 2; ++i)
		res[i] = v1[i] + v2[i];
	return res;
}

inline Vec2 operator-(Vec2 const &v1, Vec2 const &v2)
{
	Vec2 res;
	for (int i = 0; i < 2; ++i)
		res[i] = v1[i] - v2[i];
	return res;
}

inline float DotProd(Vec2 const &v1, Vec2 const &v2)
{
	float accum = 0;
	for (int i = 0; i < 2; ++i)
		accum += v1[i] * v2[i];
	return accum;
}

inline float Length(Vec2 const &v)
{
	return std::sqrt(DotProd(v, v));
}

inline Vec2 operator*(float const a, Vec2 const &v)
{
	Vec2 ret;
	for (int i = 0; i < 2; ++i)
		ret[i] = a * v[i];
	return ret;
}

/* ************************************************************************* */

struct Vec3 {
	union {
		struct {
			float x, y, z;
		};
		float data[3];
	};

	float &operator[](int i) noexcept
	{
		return data[i];
	}

	float const &operator[](int i) const noexcept
	{
		return data[i];
	}
};

inline Vec3 operator+(Vec3 const &v1, Vec3 const &v2)
{
	Vec3 res;
	for (int i = 0; i < 3; ++i)
		res[i] = v1[i] + v2[i];
	return res;
}

inline Vec3 operator-(Vec3 const &v1, Vec3 const &v2)
{
	Vec3 res;
	for (int i = 0; i < 3; ++i)
		res[i] = v1[i] - v2[i];
	return res;
}

inline float DotProd(Vec3 const &v1, Vec3 const &v2)
{
	float accum = 0;
	for (int i = 0; i < 3; ++i)
		accum += v1[i] * v2[i];
	return accum;
}

inline Vec3 CrossProd(Vec3 const &v1, Vec3 const &v2)
{
	return Vec3{ v1.y * v2.z - v1.z * v2.y,
		     v1.z * v2.x - v1.x * v2.z,
		     v1.x * v2.y - v1.y * v2.x };
}

inline float Length(Vec3 const &v)
{
	return std::sqrt(DotProd(v, v));
}

inline Vec3 Normalize(Vec3 const &v)
{
	float len = Length(v);
	Vec3 res;
	for (int i = 0; i < 3; ++i)
		res[i] = v[i] / len;
	return res;
}

/* ************************************************************************** */

struct Mat3 {
	float data[3][3];
	float *operator[](int i) noexcept
	{
		return data[i];
	}
	float const *operator[](int i) const noexcept
	{
		return data[i];
	}
};

inline Vec3 operator*(Mat3 const &m, Vec3 const &v)
{
	Vec3 ret = { 0, 0, 0 };
	for (int i = 0; i < 3; ++i)
		for (int j = 0; j < 3; ++j)
			ret[i] += m[i][j] * v[j];
	return ret;
}

inline Vec3 operator*(float const a, Vec3 const &v)
{
	Vec3 ret;
	for (int i = 0; i < 3; ++i)
		ret[i] = a * v[i];
	return ret;
}

inline bool operator>(Vec3 const &v1, Vec3 const &v2)
{
	for (int i = 0; i < 3; ++i) {
		if (v1[i] <= v2[i])
			return false;
	}
	return true;
}

/* ************************************************************************** */

struct alignas(16) Vec4 {
	union {
		struct {
			float x, y, z, w;
		};
		float data[4];
		__m128 ymm;
	};

	float &operator[](int i) noexcept
	{
		return data[i];
	}

	float const &operator[](int i) const noexcept
	{
		return data[i];
	}
};

inline Vec4 operator+(Vec4 const &v1, Vec4 const &v2)
{
	Vec4 res;
#ifdef GEOM_XMM
	res.ymm = _mm_add_ps(v1.ymm, v2.ymm);
#else
	for (int i = 0; i < 4; ++i)
		res[i] = v1[i] + v2[i];
#endif
	return res;
}

inline Vec4 operator*(float const a, Vec4 const &v)
{
	Vec4 ret;
#ifdef GEOM_XMM
	ret.ymm = _mm_mul_ps(v.ymm, _mm_set1_ps(a));
#else
	for (int i = 0; i < 4; ++i)
		ret[i] = a * v[i];
#endif
	return ret;
}

inline float DotProd(Vec4 const &v1, Vec4 const &v2)
{
#ifdef GEOM_XMM
	return _mm_cvtss_f32(_mm_dp_ps(v1.ymm, v2.ymm, 0b11111111));
#else
	float accum = 0;
	for (int i = 0; i < 4; ++i)
		accum += v1[i] * v2[i];
	return accum;
#endif
}

inline float DotProd3(Vec4 const &v1, Vec4 const &v2)
{
#ifdef GEOM_XMM
	return _mm_cvtss_f32(_mm_dp_ps(v1.ymm, v2.ymm, 0b01110111));
#else
	float accum = 0;
	for (int i = 0; i < 3; ++i)
		accum += v1[i] * v2[i];
	return accum;
#endif
}

/* ************************************************************************** */

struct Mat4 {
	float data[4][4];
	float *operator[](int i) noexcept
	{
		return data[i];
	}
	float const *operator[](int i) const noexcept
	{
		return data[i];
	}
};

inline Mat4 MakeMat4Identy()
{
	Mat4 ret;
	for (int i = 0; i < 4; ++i) {
		for (int j = 0; j < 4; ++j) {
			if (i == j)
				ret[i][j] = 1.0f;
			else
				ret[i][j] = 0;
		}
	}
	return ret;
}

inline Mat4 MakeMat4LookAt(Vec3 const &eye, Vec3 const &at, Vec3 const &up)
{
	Vec3 zaxis = Normalize(eye - at);
	Vec3 xaxis = Normalize(CrossProd(up, zaxis));
	Vec3 yaxis = CrossProd(zaxis, xaxis);

	return Mat4 { xaxis[0], xaxis[1], xaxis[2], DotProd(xaxis, eye),
		      yaxis[0], yaxis[1], yaxis[2], DotProd(yaxis, eye),
		      zaxis[0], zaxis[1], zaxis[2], DotProd(zaxis, eye),
		      0,        0,        0,        1                   };
}

inline Mat4 MakeMat4Translate(Vec3 const &v)
{
	Mat4 ret = { 1.0f, 0,    0,    v[0],
		     0,    1.0f, 0,    v[1],
		     0,    0,    1.0f, v[2],
		     0,    0,    0,    1.0f };
	return ret;
}

inline Mat4 MakeMat4Scale(Vec3 const &v)
{
	Mat4 ret = { v[0], 0,    0,    0,
		     0,    v[1], 0,    0,
		     0,    0,    v[2], 0,
		     0,    0,    0,    1.0f };
	return ret;
}

inline Mat4 MakeMat4Rotate(Vec3 const &v, float a)
{
	float c = std::cos(a);
	float s = std::sin(a);
	float t = 1 - c;

	Vec3 n = Normalize(v);
	float x = n.x;
	float y = n.y;
	float z = n.z;

	Mat4 ret = { c+x*x*t,   y*x*t+z*s, z*x*t-y*s, 0,
		     x*y*t-z*s, c+y*y*t,   z*y*t+x*s, 0,
		     x*z*t+y*s, y*z*t-x*s, z*z*t+c,   0,
		     0,         0,         0,         1 };

	return ret;
}

inline Mat4 MakeMat4Projection(float r, float l,
			       float t, float b,
			       float f, float n)
{
	Mat4 ret = { 2*n/(r-l), 0,         (r+l)/(r-l),  0,
		     0,         2*n/(t-b), (t+b)/(t-b),  0,
		     0,         0,        -(f+n)/(f-n), -2*f*n/(f-n),
		     0,         0,        -1,            0           };
	return ret;
}

inline Mat4 Inverse(Mat4 const &mat)
{
	Mat4 ret;
	float *inv = &ret.data[0][0];
	float const *m = &mat.data[0][0];
	float det;

	inv[0] = m[5] * m[10] * m[15] - m[5] * m[11] * m[14] -
		 m[9] * m[6] * m[15] + m[9] * m[7] * m[14] +
		 m[13] * m[6] * m[11] - m[13] * m[7] * m[10];

	inv[4] = -m[4] * m[10] * m[15] + m[4] * m[11] * m[14] +
		 m[8] * m[6] * m[15] - m[8] * m[7] * m[14] -
		 m[12] * m[6] * m[11] + m[12] * m[7] * m[10];

	inv[8] = m[4] * m[9] * m[15] - m[4] * m[11] * m[13] -
		 m[8] * m[5] * m[15] + m[8] * m[7] * m[13] +
		 m[12] * m[5] * m[11] - m[12] * m[7] * m[9];

	inv[12] = -m[4] * m[9] * m[14] + m[4] * m[10] * m[13] +
		  m[8] * m[5] * m[14] - m[8] * m[6] * m[13] -
		  m[12] * m[5] * m[10] + m[12] * m[6] * m[9];

	inv[1] = -m[1] * m[10] * m[15] + m[1] * m[11] * m[14] +
		 m[9] * m[2] * m[15] - m[9] * m[3] * m[14] -
		 m[13] * m[2] * m[11] + m[13] * m[3] * m[10];

	inv[5] = m[0] * m[10] * m[15] - m[0] * m[11] * m[14] -
		 m[8] * m[2] * m[15] + m[8] * m[3] * m[14] +
		 m[12] * m[2] * m[11] - m[12] * m[3] * m[10];

	inv[9] = -m[0] * m[9] * m[15] + m[0] * m[11] * m[13] +
		 m[8] * m[1] * m[15] - m[8] * m[3] * m[13] -
		 m[12] * m[1] * m[11] + m[12] * m[3] * m[9];

	inv[13] = m[0] * m[9] * m[14] - m[0] * m[10] * m[13] -
		  m[8] * m[1] * m[14] + m[8] * m[2] * m[13] +
		  m[12] * m[1] * m[10] - m[12] * m[2] * m[9];

	inv[2] = m[1] * m[6] * m[15] - m[1] * m[7] * m[14] -
		 m[5] * m[2] * m[15] + m[5] * m[3] * m[14] +
		 m[13] * m[2] * m[7] - m[13] * m[3] * m[6];

	inv[6] = -m[0] * m[6] * m[15] + m[0] * m[7] * m[14] +
		 m[4] * m[2] * m[15] - m[4] * m[3] * m[14] -
		 m[12] * m[2] * m[7] + m[12] * m[3] * m[6];

	inv[10] = m[0] * m[5] * m[15] - m[0] * m[7] * m[13] -
		  m[4] * m[1] * m[15] + m[4] * m[3] * m[13] +
		  m[12] * m[1] * m[7] - m[12] * m[3] * m[5];

	inv[14] = -m[0] * m[5] * m[14] + m[0] * m[6] * m[13] +
		  m[4] * m[1] * m[14] - m[4] * m[2] * m[13] -
		  m[12] * m[1] * m[6] + m[12] * m[2] * m[5];

	inv[3] = -m[1] * m[6] * m[11] + m[1] * m[7] * m[10] +
		 m[5] * m[2] * m[11] - m[5] * m[3] * m[10] -
		 m[9] * m[2] * m[7] + m[9] * m[3] * m[6];

	inv[7] = m[0] * m[6] * m[11] - m[0] * m[7] * m[10] -
		 m[4] * m[2] * m[11] + m[4] * m[3] * m[10] +
		 m[8] * m[2] * m[7] - m[8] * m[3] * m[6];

	inv[11] = -m[0] * m[5] * m[11] + m[0] * m[7] * m[9] +
		  m[4] * m[1] * m[11] - m[4] * m[3] * m[9] -
		  m[8] * m[1] * m[7] + m[8] * m[3] * m[5];

	inv[15] = m[0] * m[5] * m[10] - m[0] * m[6] * m[9] -
		  m[4] * m[1] * m[10] + m[4] * m[2] * m[9] +
		  m[8] * m[1] * m[6] - m[8] * m[2] * m[5];

	det = m[0] * inv[0] + m[1] * inv[4] + m[2] * inv[8] + m[3] * inv[12];

	det = 1.0 / det;

	for (int i = 0; i < 16; i++)
		inv[i] = inv[i] * det;

	return ret;
}

inline Mat4 Transpose(Mat4 const &m)
{
	Mat4 ret;
	for (int i = 0; i < 4; ++i)
		for (int j = 0; j < 4; ++j)
			ret[i][j] = m[j][i];
	return ret;
}

inline Vec3 ToVec3(Vec4 const &v)
{
	return Vec3 { v.x/v.w, v.y/v.w, v.z/v.w };
}

inline Vec4 ToVec4(Vec3 const &v)
{
	return Vec4 { v.x, v.y, v.z, 1.0f };
}

inline Vec4 operator*(Mat4 const &m, Vec4 const &v)
{
	Vec4 ret = { 0, 0, 0, 0 };
	for (int i = 0; i < 4; ++i)
		for (int j = 0; j < 4; ++j)
			ret[i] += m[i][j] * v[j];
	return ret;
}

inline Mat4 operator*(Mat4 const &m1, Mat4 const &m2)
{
	Mat4 ret;
	for (int i = 0; i < 4; ++i) {
		for (int j = 0; j < 4; ++j) {
			float accum = 0;
			for (int k = 0; k < 4; ++k)
				accum += m1[i][k] * m2[k][j];
			ret[i][j] = accum;
		}
	}
	return ret;
}

/* ************************************************************************** */

struct Window {
	uint32_t x, y, w, h;
	float f, n;
};

struct Vertex {
	Vec3 pos;
	Vec2 tex;
	Vec3 norm;
};

struct ViewportTransform {
	Vec3 scale, offs;
	Vec3 min_scr;
	Vec3 max_scr;

	inline void set_window(Window const &wnd)
	{
		float x = wnd.x, y = wnd.y, w = wnd.w, h = wnd.h,
		      f = wnd.f, n = wnd.n;

		scale = Vec3 { w/2,     h/2,     (f-n)/2 };
		offs  = Vec3 { x + w/2, y + h/2, (f+n)/2 };

		min_scr = offs - scale;
		max_scr = offs + scale;
	}

	inline Vec3 operator()(Vec3 const &v) const
	{
		Vec3 ret;
		for (int i = 0; i < 3; ++i)
			ret[i] = v[i] * scale[i] + offs[i];
		return ret;
	}
};

inline Vec3 ReinterpVec3(Vec4 const &v)
{
	return Vec3 {v[0], v[1], v[2]};
}

/* ************************************************************************** */

struct Vec2i {
	int32_t x, y;
};
