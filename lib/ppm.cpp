#include <include/ppm.h>

#include <cstdlib>
#include <utility>
#include <fstream>
#include <sstream>
#include <vector>
#include <unordered_map>

int PpmImg::Import(char const *path)
{
	std::ifstream in(path);
	if (!in.is_open())
		return -1;
	std::string line;
	for (int counter = 0; counter < 4;) {
		if (!(std::getline(in, line)))
			return -1;
		if (line[0] == '#')
			continue;
		std::istringstream iss(line);
		std::string word;
		while (!iss.eof()) {
			if (counter == 0) {
				iss >> word;
				if (word != "P6")
					return -1;
				++counter;
			} else if(counter == 1) {
				if (iss >> w)
					++counter;
			} else if(counter == 2) {
				if (iss >> h)
					++counter;
			} else if(counter == 3) {
				int i_dont_use_this;
				if (iss >> i_dont_use_this)
					++counter;
			} else
				return -1;
		}
	}
	buf.resize(w * h);

	in.read(reinterpret_cast<char*>(&buf[0]),
			buf.size() * sizeof(Color));
	if (!in.good())
		return -1;

	return 0;
}
