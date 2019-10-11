#pragma once

#include <cmath>

struct vector2d {
	double x, y;
};

struct vector3d {
	double x, y, z;
};

struct matrix3d {
	double data[3][3];

	double *operator[](std::size_t i)
	{
		return data[i];
	}

	double const *operator[](std::size_t i) const
	{
		return data[i];
	}
};

inline vector2d operator*(vector2d const &v, double k)
{
	return vector2d{ v.x * k, v.y * k };
}

inline vector3d operator+(vector3d const &v1, vector3d const &v2)
{
	return vector3d{ v1.x + v2.x, v1.y + v2.y, v1.z + v2.z };
}

inline vector3d operator-(vector3d const &v1, vector3d const &v2)
{
	return vector3d{ v1.x - v2.x, v1.y - v2.y, v1.z - v2.z };
}

inline vector2d operator-(vector2d const &v1, vector2d const &v2)
{
	return vector2d{ v1.x - v2.x, v1.y - v2.y };
}

inline vector3d operator*(vector3d const &v, double k)
{
	return vector3d{ v.x * k, v.y * k, v.z * k };
}

inline double dot_product(vector3d const &v1, vector3d const &v2)
{
	return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z;
}

inline vector3d cross_product(vector3d const &v1, vector3d const &v2)
{
	return vector3d{ v1.y * v2.z - v1.z * v2.y, v1.z * v2.x - v1.x * v2.z,
			 v1.x * v2.y - v1.y * v2.x };
}

inline double lenght(vector3d const &v)
{
	return std::sqrt(dot_product(v, v));
}

inline vector3d operator*(matrix3d const &m, vector3d const &vec)
{
	vector3d ret{ m[0][0] * vec.x + m[0][1] * vec.y + m[0][2] * vec.z,
		      m[1][0] * vec.x + m[1][1] * vec.y + m[1][2] * vec.z,
		      m[2][0] * vec.x + m[2][1] * vec.y + m[2][2] * vec.z };
	return ret;
}

inline vector2d direct_proj(vector3d const &vec)
{
	return vector2d{ vec.x, vec.y };
}

inline matrix3d rotation_matrix3d_euler(vector3d ang)
{
	double c[3] = { std::cos(ang.x), std::cos(ang.y), std::cos(ang.z) };
	double s[3] = { std::sin(ang.x), std::sin(ang.y), std::sin(ang.z) };
	struct matrix3d m;

#if 0
	m[0][0] =  c[1] * c[2];
	m[0][1] = -c[1] * s[2];
	m[0][2] = -s[1];

	m[1][0] =  c[0] * s[2] - c[2] * s[0] * s[1];
	m[1][1] =  c[0] * c[2] - s[0] * s[1] * s[2];
	m[1][2] = -c[1] * s[0];

	m[2][0] =  s[0] * s[2] - c[0] * c[2] * s[1];
	m[2][1] =  c[2] * s[0] - c[0] * s[1] * s[2];
	m[2][2] =  c[0] * c[1];
#endif

	m[0][0] = c[1] * c[2];
	m[0][1] = -c[1] * s[2];
	m[0][2] = s[1];

	m[1][0] = s[0] * s[1] * c[2] + c[0] * s[2];
	m[1][1] = -s[0] * s[1] * s[2] + c[0] * c[2];
	m[1][2] = -s[0] * c[1];

	m[2][0] = -c[0] * s[1] * c[2] + s[0] * s[2];
	m[2][1] = c[0] * s[1] * s[2] + s[0] * c[2];
	m[2][2] = c[0] * c[1];

	return m;
}

inline vector2d simple_camera_transform(vector3d vec, double dist,
					matrix3d rot_mat)
{
	vector3d rot = rot_mat * vec;
	return direct_proj(rot) * (1 / (dist - rot.z));
}

#if 0
/* add e */
inline vector2d camera_transform(vector3d vec, vector3d cam,
				 matrix3d rot_mat)
{
	vector3d rot = rot_mat * (vec - cam);
	return vector2d((-cam.z / rot.z) * rot.x - cam.x,
			(-cam.z / rot.z) * rot.y - cam.y);
}
#endif
