#pragma once

#include <utility>
#include <fstream>
#include <sstream>
#include <vector>
#include <unordered_map>
#include <include/geom.h>

struct wfobj_mesh {
	std::vector<vertex> verts;
	std::vector<std::size_t> inds;
};

struct wfobj_entry {
	std::string name;
	wfobj_mesh mesh;
	// texture
};

void import_mesh_map(char const *path,
	std::unordered_map<std::string, wfobj_entry> &map)
{
	std::ifstream in(path);
	if (!in.is_open())
		throw std::invalid_argument("Can not find file " +
					std::string(path));
	std::string line;
	std::vector<vec3> pos;
	std::vector<vec2> tex;
	std::vector<vec3> norm;

	wfobj_mesh *cur;

	while (std::getline(in, line)) {
		std::istringstream iss(line);
		std::string word;
		iss >> word;
		if (word == "v") {
			vec3 v;
			if (!(iss >> v.x >> v.y >> v.z))
				throw std::runtime_error(
					"could not parse line: " + line);
			pos.push_back(v);
		} else if (word == "vt") {
			vec2 v;
			if (!(iss >> v.x >> v.y))
				throw std::runtime_error(
					"could not parse line: " + line);
			tex.push_back(v);
		} else if (word == "vn") {
			vec3 v;
			if (!(iss >> v.x >> v.y >> v.z))
				throw std::runtime_error(
					"could not parse line: " + line);
			norm.push_back(v);
		} else if (word == "f") {
			std::size_t idx[3];
			char trash;
			auto const vsize = cur->verts.size();
			while (iss >> idx[0] >> trash >> idx[1] >> trash >> idx[2]) {
				cur->verts.push_back({ pos[idx[0] - 1],
						       tex[idx[1] - 1],
						       norm[idx[2] - 1] });
			}
			auto const vcount = cur->verts.size() - vsize;
			for (auto i = 1u; i < vcount - 1; i++) {
				cur->inds.push_back(vsize);
				cur->inds.push_back(vsize + i);
				cur->inds.push_back(vsize + i + 1);
			}
		} else if (word == "usemtl") {
			iss >> word;
			cur = &map[word].mesh;
		}
	}
	for (auto &e : map)
		e.second.name = e.first;
}

void import_wfobj(const char *objpath, std::vector<wfobj_entry> &vec)
{
	std::unordered_map<std::string, wfobj_entry> map;
	import_mesh_map(objpath, map);

	for (auto &e : map)
		vec.push_back(std::move(e.second));
}
