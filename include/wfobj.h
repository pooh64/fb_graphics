#pragma once

#include <fstream>
#include <sstream>
#include <vector>
#include <include/vector_space.h>

struct mesh {
	struct vertex {
		vector3d pos;
		vector2d tex;
		vector3d norm;
	};

	std::vector<vertex> verts;
	std::vector<std::size_t> inds;
};

inline mesh import_obj(char const *filename)
{
	std::ifstream in(filename);
	mesh out;
	if (!in.is_open())
		throw std::invalid_argument("can not find file " +
					    std::string(filename));
	std::string line;
	std::vector<vector3d> pos;
	std::vector<vector2d> tex;
	std::vector<vector3d> norm;
	while (std::getline(in, line)) {
		std::istringstream iss(line);
		std::string word;
		iss >> word;
		if (word == "v") {
			vector3d v;
			if (!(iss >> v.x >> v.y >> v.z))
				throw std::runtime_error(
					"could not parse line: " + line);
			pos.push_back(v);
		} else if (word == "vt") {
			vector2d v;
			if (!(iss >> v.x >> v.y))
				throw std::runtime_error(
					"could not parse line: " + line);
			tex.push_back(v);
		} else if (word == "vn") {
			vector3d v;
			if (!(iss >> v.x >> v.y >> v.z))
				throw std::runtime_error(
					"could not parse line: " + line);
			norm.push_back(v);
		} else if (word == "f") {
			std::size_t idx[3];
			char trash;
			auto const vsize = out.verts.size();
			while (iss >> idx[0] >> trash >> idx[1] >> trash >>
			       idx[2])
				out.verts.push_back({ pos[idx[0] - 1],
						      tex[idx[1] - 1],
						      norm[idx[2] - 1] });
			auto const vcount = out.verts.size() - vsize;
			for (auto i = 1u; i < vcount - 1; i++) {
				out.inds.push_back(vsize);
				out.inds.push_back(vsize + i);
				out.inds.push_back(vsize + i + 1);
			}
		}
	}
	return out;
}
