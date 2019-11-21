#pragma once

#include <include/geom.h>

struct Sphere {
	Vec3 pos;
	float R;

	Vec3 color;
};

struct Ray {
	Vec3 pos;
	Vec3 dir;
};

inline Vec3 RayVec(Ray const &r, float const t) noexcept
{
	return r.pos + t * r.dir;
}

inline float IntersectSphere(Sphere const &s, Ray const &r) noexcept
{
	Vec3 const rp = r.pos - s.pos;
	float const ra_rp = DotProd(r.dir, rp);
	if (ra_rp > 0.f)
		return 0.f;
	float const d = ra_rp * ra_rp - DotProd(rp, rp) + s.R * s.R;
	if (d < 0.f)
		return 0.f;
	return -ra_rp - sqrtf(d);
}

struct Camera {
	float fov;
	float ratio;
	Vec3 pos;
	Vec3 at;
	Vec3 up;
};

inline Ray CastFromCam(Camera const &cam, Vec2 const &r) noexcept
{
	Vec3 const n = Normalize(cam.at - cam.pos);
	Vec3 const right = CrossProd(n, cam.up);
	Vec3 const up = CrossProd(right, n);
	Vec3 const x = (cam.fov * r.x * cam.ratio) * Normalize(right);
	Vec3 const y = (cam.fov * r.y) * Normalize(up);
	return Ray{ cam.pos, Normalize(n + x + y) };
}

struct Intersection {
	float t;
	int idx;
};

inline Intersection FindIntersection(Ray const &r, Sphere const *sph,
				      int sph_sz) noexcept
{
	float depth = 1e6;
	float const dmin = 1e-3;
	int idx = sph_sz;
	for (int i = 0; i < sph_sz; i++) {
		float const d = IntersectSphere(sph[i], r);
		if (d > dmin && d < depth) {
			depth = d;
			idx = i;
		}
	}
	return Intersection{ depth, idx == sph_sz ? -1 : idx };
}
