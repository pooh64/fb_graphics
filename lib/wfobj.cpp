#include <include/wfobj.h>

#include <utility>
#include <fstream>
#include <sstream>
#include <unordered_map>

void ImportMtlFile(char const *path,
	std::unordered_map<std::string, Wfobj::Mtl> &map)
{
	std::ifstream in(path);
	if (!in.is_open())
		throw std::invalid_argument("Can not find file " +
					std::string(path));
	Wfobj::Mtl *cur;

	std::string line;
	while (std::getline(in, line)) {
		std::istringstream iss(line);
		std::string word;
		iss >> word;
		if (word == "illum") {
			int tmp;
			if (!(iss >> tmp))
				goto handle_err;
			cur->illum = static_cast<Wfobj::Mtl::IllumType> (tmp);
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
			if (cur->tex_img.Import(word.c_str()) < 0)
				goto handle_err;
		} else if (word == "newmtl") {
			if (!(iss >> word))
				goto handle_err;
			cur = &map[word];
		}
	}

	return;

handle_err:
	throw std::runtime_error(
		"could not parse line: " + line);
}

void ImportObjFile(char const *path, std::string &mtl_path,
	std::unordered_map<std::string, Wfobj::Wfobj::Mesh> &map)
{
	std::ifstream in(path);
	if (!in.is_open())
		throw std::invalid_argument("Can not find file " +
					std::string(path));
	std::string line;
	std::vector<Vec3> pos;
	std::vector<Vec2> tex;
	std::vector<Vec3> norm;

	Wfobj::Mesh *cur;

	while (std::getline(in, line)) {
		std::istringstream iss(line);
		std::string word;
		iss >> word;
		if (word == "v") {
			Vec3 v;
			if (!(iss >> v.x >> v.y >> v.z))
				goto handle_err;
			pos.push_back(v);
		} else if (word == "vt") {
			Vec2 v;
			if (!(iss >> v.x >> v.y))
				goto handle_err;
			tex.push_back(v);
		} else if (word == "vn") {
			Vec3 v;
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
			if (!(iss >> word))
				goto handle_err;
			cur = &map[word];
		} else if (word == "mtllib") {
			if (!(iss >> word))
				goto handle_err;
			mtl_path = word;
		}
	}
	return;

handle_err:
	throw std::runtime_error(
		"could not parse line: " + line);
}

int ImportWfobj(const char *obj, std::vector<Wfobj> &vec)
{
	std::unordered_map<std::string, Wfobj::Mesh> map_mesh;
	std::unordered_map<std::string, Wfobj::Mtl>  map_mtl;
	try {
		std::string mtl_path;
		ImportObjFile(obj, mtl_path, map_mesh);
		ImportMtlFile(mtl_path.c_str(), map_mtl);
		if (map_mesh.size() != map_mtl.size())
			return -1;

		for (auto &e : map_mesh) {
			Wfobj tmp;
			tmp.name = e.first;
			tmp.mesh = e.second;
			tmp.mtl = map_mtl.at(e.first);
			vec.push_back(tmp);
		}
	} catch (...) {
		return -1;
	}
	return 0;
}
