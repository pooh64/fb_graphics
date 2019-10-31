#include <include/ppm.h>

#include <utility>
#include <fstream>
#include <sstream>
#include <vector>
#include <unordered_map>

int ImportPpm(char const *path, Ppm_img &img)
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
				if (word == "P6");
				++counter;
			} else if(counter == 1) {
				if (iss >> img.w)
					++counter;
			} else if(counter == 2) {
				if (iss >> img.h)
					++counter;
			} else if(counter == 3) {
				int i_dont_use_this;
				if (iss >> i_dont_use_this)
					++counter;
			} else
				return -1;
		}
	}
	img.buf = new (std::nothrow) Ppm_img::Color[img.w * img.h];
	if (img.buf == NULL)
		return -1;

	in.read(reinterpret_cast<char*>(img.buf),
			img.w * img.h * sizeof(Ppm_img::Color));
	if (!in.good())
		return -1;
	return 0;
}
