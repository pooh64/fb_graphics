#include <include/geom.h>
#include <iostream>

void DumpVec4(Vec4 const &v)
{
	for (int i = 0; i < 4; ++i)
		std::cout << v[i] << " ";
}

void DumpMat4(Mat4 const &m)
{
	for (int i = 0; i < 4; ++i) {
		for (int j = 0; j < 4; ++j)
			std::cout << m[i][j] << " ";
		std::cout << std::endl;
	}
}

int main()
{
	Mat4 sc = MakeMat4Scale(Vec3{1, 2, 3});
	Mat4 id = MakeMat4Identy();

	Mat4 vw = MakeMat4LookAt(Vec3{1, 1, 1}, Vec3{0, 0, 0}, Vec3{0, 1, 0});
	DumpMat4(vw);

	Vec4 vec = {0, 0, 0, 1};
	DumpVec4((sc * vw) * vec);

	std::cout << "\n\n";

	Vec3 eye {0, 0, 10};
	Vec3 at  {0, 0, 0};
	Vec3 up  {0, 1, 0};
	Mat4 view = MakeMat4LookAt(eye, at, up);

	DumpMat4(view);

	return 0;
}
