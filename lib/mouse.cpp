extern "C" {
#include <fcntl.h>
#include <unistd.h>
}

#include <include/mouse.h>

int Mouse::Init(const char *path) noexcept
{
	return fd = open(path, O_RDONLY | O_NONBLOCK);
}

int Mouse::Destroy() noexcept
{
	return close(fd);
}

bool Mouse::Poll(Mouse::Event &e) noexcept
{
	return read(fd, &e, sizeof(Mouse::Event)) > 0;
}
