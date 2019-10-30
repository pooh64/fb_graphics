#pragma once

#include <utility>
#include <fstream>
#include <sstream>
#include <vector>
#include <cassert>
#include <unordered_map>
#include <include/geom.h>
#include <include/ppm.h>

struct ppm_color {
	uint8_t r, g, b;
};

struct ppm_img {
	ppm_color *buf;
	uint32_t w, h;
};

ppm_img import_ppm(char const *path)
{
	std::ifstream in(path);
	if (!in.is_open())
		throw std::invalid_argument("Can not find file " +
					std::string(path));
	ppm_img img;
	std::string line;
	int counter = 0;
	while (counter < 4) {
		assert(std::getline(in, line));
		if(line[0] == '#')
			continue;
		std::istringstream iss(line);
		std::string word;
		while (!iss.eof()) {
			if (counter == 0) {
				iss >> word;
				assert(word == "P6");
				++counter;
			} else if(counter == 1) {
				if(iss >> img.w)
					++counter;
			} else if(counter == 2) {
				if(iss >> img.h)
					++counter;
			} else if(counter == 3) {
				int i_dont_use_this;
				if(iss >> i_dont_use_this)
					++counter;
			} else
				assert(0);
		}
	}
	img.buf = new ppm_color[img.w * img.h];
	in.read(reinterpret_cast<char*>(img.buf),
			img.w * img.h * sizeof(ppm_color));
	return img;
}

struct wfobj_mesh {
	std::vector<vertex> verts;
	std::vector<std::size_t> inds;
};

struct wfobj_mtl {
	enum class illum_type : uint8_t {
		COLOR = 0,
		AMBIENT = 1,
		HIGHLIGHT = 2,
	} illum;
	struct color_intens {
		float r, g, b;
	};
	color_intens amb, diff, spec;
	float ns;
	ppm_img tex_img;
};

struct wfobj_entry {
	std::string name;
	wfobj_mesh mesh;
	bool mtl_loaded;
	wfobj_mtl mtl;
};

void import_mtl_file(char const *path,
	std::unordered_map<std::string, wfobj_entry> &map)
{
	std::ifstream in(path);
	if (!in.is_open())
		throw std::invalid_argument("Can not find file " +
					std::string(path));

	wfobj_mtl *cur;

	std::string line;
	while (std::getline(in, line)) {
		std::istringstream iss(line);
		std::string word;
		iss >> word;
		if (word == "illum") {
			uint8_t tmp;
			if (!(iss >> tmp))
				goto handle_err;
			cur->illum = static_cast<wfobj_mtl::illum_type>(tmp);
		} else if (word == "Ns") {
			if (!(iss >> cur->ns))
				goto handle_err;
		} else if (word == "Ka") {
			if (!(iss >>  cur->amb.r >>  cur->amb.g >>  cur->amb.b))
				goto handle_err;
		} else if (word == "Kd") {
			if (!(iss >> cur->diff.r >> cur->diff.g >> cur->diff.b))
				goto handle_err;
		} else if (word == "Ks") {
			if (!(iss >> cur->spec.r >> cur->spec.g >> cur->spec.b))
				goto handle_err;
		} else if (word == "map_Kd") {
			if (!(iss >> word))
				goto handle_err;
			cur->tex_img = import_ppm(word.c_str());
		} else if (word == "newmtl") {
			if (!(iss >> word))
				goto handle_err;
			map[word].mtl_loaded = true;
			cur = &map[word].mtl;
		}
	}

	return;

handle_err:
	throw std::runtime_error(
		"could not parse line: " + line);
}

void import_obj_file(char const *path,
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
				goto handle_err;
			pos.push_back(v);
		} else if (word == "vt") {
			vec2 v;
			if (!(iss >> v.x >> v.y))
				goto handle_err;
			tex.push_back(v);
		} else if (word == "vn") {
			vec3 v;
			if (!(iss >> v.x >> v.y >> v.z))
				goto handle_err;
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
			if(!(iss >> word))
				goto handle_err;
			cur = &map[word].mesh;
		}
	}
	for (auto &e : map) {
		e.second.name = e.first;
		e.second.mtl_loaded = false;
	}

	return;

handle_err:
	throw std::runtime_error(
		"could not parse line: " + line);
}

void import_wfobj(const char *obj, const char *mtl, std::vector<wfobj_entry> &vec)
{
	std::unordered_map<std::string, wfobj_entry> map;
	import_obj_file(obj, map);
	import_mtl_file(mtl, map);

	for (auto &e : map)
		vec.push_back(std::move(e.second));
}
