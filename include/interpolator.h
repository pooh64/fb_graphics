#pragma once
#include <include/geom.h>

template<typename _intr_prim, typename _intr_in, typename _intr_out>
struct interpolator {
	using intr_prim = _intr_prim;
	using intr_in   = _intr_in;
	using intr_out  = _intr_out;
	virtual intr_out interpolate(intr_prim const &, intr_in const &)
		const = 0;
};

struct tr_interpolator final: public interpolator<vertex[3], float[3], vertex> {
	intr_out interpolate(intr_prim const &tr, intr_in const &bc)
		const override
	{
		vertex v;
		v.pos  = vec3 {0, 0, 0};
		v.tex  = vec2 {0, 0};
		v.norm = vec3 {0, 0, 0};
		for (int i = 0; i < 3; ++i) {
			v.pos  = v.pos  + bc[i] * tr[i].pos;
			v.norm = v.norm + bc[i] * tr[i].norm;
			v.tex  = v.tex  + bc[i] * tr[i].tex;
		}
		v.norm = normalize(v.norm);
		return v;
	}
};
