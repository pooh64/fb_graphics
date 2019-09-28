#pragma once

extern "C"
{
	#include <fcntl.h>
	#include <unistd.h>
}

struct mouse {
	struct event {
		uint8_t flags, dx, dy;

		bool   left_button() const noexcept { return flags & 0x1; }
		bool  right_button() const noexcept { return flags & 0x2; }
		bool middle_button() const noexcept { return flags & 0x4; }
	};

	int init(const char *path) noexcept;
	int destroy() noexcept;

	bool poll(event &e) noexcept;

private:
	int fd;
};

int mouse::init(const char *path) noexcept
{
	return fd = ::open(path, O_RDONLY | O_NONBLOCK);
}

int mouse::destroy() noexcept
{
	return close(fd);
}

inline bool mouse::poll(mouse::event &e) noexcept
{
	return ::read(fd, &e, sizeof(mouse::event)) > 0;
}
