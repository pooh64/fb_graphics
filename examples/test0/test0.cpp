#include <include/fbuffer.h>
#include <include/mouse.h>
#include <include/tr_pipeline.h>
#include <iostream>
#include <chrono>
#include <utility>

#include <cstring>

extern "C"
{
#include <unistd.h>
}

int main(int argc, char *argv[])
{
	Fbuffer fb;
	Mouse ms;
	if (fb.Init("/dev/fb0") < 0) {
		perror("fb0");
		return 1;
	}
	if (ms.Init("/dev/input/mice") < 0) {
		perror("Mouse");
		return 1;
	}

	float pos = 20;

	Window wnd = { .x = 0, .y = 0, .w = fb.xres, .h = fb.yres,
	 	       .f = pos * 100, .n = pos / 10 };


	return 0;
}
