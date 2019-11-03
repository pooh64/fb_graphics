#pragma once

#include <cstdint>
#include <vector>
#include <string>
#include <include/geom.h>
#include <include/ppm.h>

struct Wfobj {
	struct Mesh {
		std::vector<Vertex> verts;
		std::vector<std::size_t> inds;
	} mesh;

	struct Mtl {
		enum class Illum_type : uint8_t {
			COLOR = 0,
			AMBIENT = 1,
			HIGHLIGHT = 2,
		} illum;
		struct Color_intens {
			float r, g, b;
		} amb, diff, spec;
		float ns;
		Ppm_img tex_img;
	} mtl;

	std::string name;

	void Destroy();
};

int ImportWfobj(const char *obj_path, const char *mtl_path,
	std::vector<Wfobj> &vec);
