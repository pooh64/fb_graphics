#pragma once

#include <cstdio>
#include <cmath>

struct vector2d {
	double x, y;
	vector2d(double _x, double _y) : x(_x), y(_y) { }
	vector2d() = default;

	vector2d operator*(double m) const { return vector2d(x * m, y * m); }

	void print() const { std::printf("(%lg, %lg)\n", x, y); }
};

struct vector3d {
	double x, y, z;
	vector3d(double _x, double _y, double _z) : x(_x), y(_y), z(_z) { }
	vector3d() = default;

	vector3d operator*(double m) const
	{ return vector3d(x * m, y * m, z * m); }

	void rotate_x(double a);
	void rotate_y(double a);

	vector2d direct_project2d() const;


	void print() const { std::printf("(%lg, %lg, %lg)\n", x, y, z); }
};

inline vector2d vector3d::direct_project2d() const
{
	return vector2d(x, y);
}

inline void vector3d::rotate_x(double a)
{
	vector3d new_vec;
	double cos = std::cos(a);
	double sin = std::sin(a);

	new_vec.x = x                    ;
	new_vec.y =    cos * y - sin * z;
	new_vec.z =    sin * y + cos * z;

	(*this) = new_vec;
}

inline void vector3d::rotate_y(double a)
{
	vector3d new_vec;
	double cos = std::cos(a);
	double sin = std::sin(a);

	new_vec.x =   cos * x           + sin * z;
	new_vec.y =                 y            ;
	new_vec.z = - sin * x           + cos * z;

	(*this) = new_vec;
}
