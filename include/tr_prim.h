#pragma once

struct TrFragment {
	union {
		struct {
			float bc[3];
			float depth;
		};
		Vec4 sse_data;
	};
	uint32_t prim_id;
	uint32_t model_id;
};
