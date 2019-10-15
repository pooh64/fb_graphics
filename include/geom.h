#pragma once

struct vec2 {
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

inline vec2
operator+(vec2 const &v1, vec2 const &v2)
{
	vec2 res;
	for (int i = 0; i < 2; ++i)
		res = v1[i] + v2[i];
	return res;
}

inline vec2 operator-(vec2 const &v1, vec2 const &v2)
{
	vec2 res;
	for (int i = 0; i < 2; ++i)
		res = v1[i] - v2[i];
	return res;
}

inline vec2 dot_prod(vec2 const &v1, vec2 const &v2)
{
	float accum = 0;
	for (int i = 0; i < 2; ++i)
		accum += v1[i] * v2[i];
	return accum
}

inline float lenght(vec2 const &v)
{
	return std::sqrt(dot_prod(v, v));
}

/* ************************************************************************* */

struct vec3 {
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

inline vec3 operator+(vec3 const &v1, vec3 const &v2)
{
	vec3 res;
	for (int i = 0; i < 3; ++i)
		res = v1[i] + v2[i];
	return res;
}

inline vec3 operator-(vec3 const &v1, vec3 const &v2)
{
	vec3 res;
	for (int i = 0; i < 3; ++i)
		res = v1[i] - v2[i];
	return res;
}

inline vec3 dot_prod(vec3 const &v1, vec3 const &v2)
{
	float accum = 0;
	for (int i = 0; i < 3; ++i)
		accum += v1[i] * v2[i];
	return accum
}

inline vec3 cross_prod(vec3 const &v1, vec3 const &v2)
{
	return vec3{ v1.y * v2.z - v1.z * v2.y,
		     v1.z * v2.x - v1.x * v2.z,
		     v1.x * v2.y - v1.y * v2.x };
}

inline float lenght(vec3 const &v)
{
	return std::sqrt(dot_prod(v, v));
}

inline vec3 normalize(vec3 const &v)
{
	float len = lenght(v);
	vec3 res;
	for (int i = 0; i < 3; ++i)
		res[i] = v[i] / len;
	return res;
}

/* ************************************************************************** */

struct mat3 {
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

/* ************************************************************************** */

struct vec4 {
	union {
		struct {
			float x, y, z, w;
		};
		float data[4];
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

/* ************************************************************************** */

struct mat4 {
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

inline mat4 make_mat4_view(vec3 const &ev, vec3 const &cv, vec3 const &uv)
{
	vec3 n = normalize(ev - cv);
	vec3 u = normalize(cross_prod(uv, n));
	vec3 v = cross_prod(n, u);

	mat4 ret = { u[0], v[0], n[0], 0.0f,
		     u[1], v[1], n[1], 0.0f,
		     u[2], v[2], n[2], 0.0f,
		     -dot_prod(u, ev),
		     -dot_prod(v, ev),
		     -dot_prod(n, ev),
		     1.0f };
	return ret;
}

inline mat4 make_mat4_translate(vec3 const &v)
{
	mat4 ret = { 1.0f, 0,    0,    v[0],
		     0,    1.0f, 0,    v[1],
		     0,    0,    1.0f, v[2],
		     0,    0,    0,    1.0f };
	return ret;
}

inline mat4 make_mat4_scale(vec3 const &v)
{
	mat4 ret = { v[0], 0,    0,    0,
		     0,    v[1], 0,    0,
		     0,    0,    v[2], 0,
		     0,    0,    0,    1.0f };
	return ret;
}

inline mat4 make_mat4_rotate(vec3 const &v, float a)
{
	float c = std::cos(a);
	float s = std::sin(a);
	float t = 1 - c;

	mat4 ret = { c+x*x*t,   y*x*t+z*s, z*x*t-y*s, 0,
		     x*y*t-z*s, c+y*y*t,   z*y*t+x*s, 0,
		     x*z*t+y*s, y*z*t-x*s, z*z*t+c,   0,
		     0,         0,         0,         1 };

	return ret;
}

inline mat4 make_mat4_projection(float r, float l,
				 float u, float b,
				 float n, float f)
{
	mat4 ret = { 2*n/(r-l), 0,         (r+l)/(r-l),  0,
		     0,         2*n/(t-b), (t+b)/(t-b),  0,
		     0,         0,        -(f+n)/(f-n), -2*f*n/(f-n),
		     0,         0,        -1,            0           };
	return ret;
}

inline mat4 inverse(mat4 const &mat)
{
	mat4 ret;
	float *inv = ret.data;
	float *m = mat.data;
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

	for (i = 0; i < 16; i++)
		inv[i] = inv[i] * det;

	return ret;
}

inline mat4 transpose(mat4 const &m)
{
	mat4 ret = { m[0][0], m[1][0], m[2][0],
		     m[0][1], m[1][1], m[2][1],
		     m[0][2], m[1][2], m[2][2] };
	return ret;
}

inline vec3 to_vec3(vec4 const &v)
{
	return vec3 { v.x/v.w, v.y/v.w, v.z/v.w };
}

inline vec4 to_vec4(vec3 const &v)
{
	return vec4 { v.x, v.y, v.z, 1 };
}

/* ************************************************************************** */

struct screen_viewport {
	vec3 scale, offs;

	void set(uint32_t x_, uint32_t y_, uint32_t w_, uint32_t h_)
	{
		float x = x_, y = y_, w = w_, h = h_;
		scale = vec3 { w/2,      h/2,     (f-n)/2 };
		offs  = vec3 { x + w/2,  y + h/2, (f+n)/2 };
	}

	inline vec3 operator()(vec3 const &v) const
	{
		vec3 ret;
		for (int i = 0; i < 3; ++i)
			ret[i] = v[i] * scale[i] + offs;
		return ret;
	}
};
