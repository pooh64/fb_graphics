#pragma once

#include <include/geom.h>

template<typename _intr_prim, typename _intr_in, typename _intr_out>
struct Interpolator {
	using intr_prim = _intr_prim;
	using intr_in   = _intr_in;
	using intr_out  = _intr_out;
	virtual intr_out Interpolate(intr_prim const &, intr_in const &)
		const = 0;
};

struct TrInterpolator final: public Interpolator<Vertex[3], float[3], Vertex> {
	intr_out Interpolate(intr_prim const &tr, intr_in const &bc)
		const override
	{
		Vertex v;
		v.pos  = Vec3 {0, 0, 0};
		v.tex  = Vec2 {0, 0};
		v.norm = Vec3 {0, 0, 0};
		Vec3 bc_corr {bc[0], bc[1], bc[2]};

		for (int i = 0; i < 3; ++i)
			bc_corr[i] = bc_corr[i] / tr[i].pos.z;

		float mp = 0;
		for (int i = 0; i < 3; ++i)
			mp = mp + bc_corr[i];

		for (int i = 0; i < 3; ++i)
			bc_corr[i] = bc_corr[i] / mp;


		for (int i = 0; i < 3; ++i) {
			v.pos  = v.pos  + bc_corr[i] * tr[i].pos;
			v.norm = v.norm + bc_corr[i] * tr[i].norm;
			v.tex  = v.tex  + bc_corr[i] * tr[i].tex;
		}

		v.norm = Normalize(v.norm);
		return v;
	}
};
