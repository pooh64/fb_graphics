extern "C" {
	#include <sys/types.h>
	#include <sys/stat.h>
	#include <sys/mman.h>
	#include <sys/ioctl.h>
	#include <fcntl.h>
	#include <unistd.h>
};

#include "include/fbuffer.h"

int fbuffer::init(const char *path)
{
	void *map_region;

	fd = ::open(path, O_RDWR);
	if (fd < 0)
		goto handle_err_0;

	if (::ioctl(fd, FBIOGET_VSCREENINFO, (fb_var_screeninfo*) this) < 0)
		goto handle_err_1;

	if (::ioctl(fd, FBIOGET_FSCREENINFO, (fb_fix_screeninfo*) this) < 0)
		goto handle_err_1;

	map_region = ::mmap(NULL, xres * yres * sizeof(*buf),
			    PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (map_region == MAP_FAILED)
		goto handle_err_1;
	buf = static_cast<color*>(map_region);

	return 0;

handle_err_1:
	::close(fd);
handle_err_0:
	return -1;
}

int fbuffer::destroy()
{
	int rc = 0;
	if (::munmap(buf, xres * yres * sizeof(*buf)) < 0)
		rc = -1;
	if (::close(fd) < 0)
		rc = -1;
	return rc;
}
