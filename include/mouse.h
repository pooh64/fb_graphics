#pragma once

extern "C" {
#include <fcntl.h>
#include <unistd.h>
}

struct Mouse {
	struct Event {
		uint8_t flags, dx, dy;

		bool LeftButton() const noexcept
		{
			return flags & 0x1;
		}
		bool RightButton() const noexcept
		{
			return flags & 0x2;
		}
		bool MiddleButton() const noexcept
		{
			return flags & 0x4;
		}
	};

	int Init(const char *path) noexcept;
	int Destroy() noexcept;

	bool Poll(Event &e) noexcept;

private:
	int fd;
};

int Mouse::Init(const char *path) noexcept
{
	return fd = ::open(path, O_RDONLY | O_NONBLOCK);
}

int Mouse::Destroy() noexcept
{
	return close(fd);
}

inline bool Mouse::Poll(Mouse::Event &e) noexcept
{
	return ::read(fd, &e, sizeof(Mouse::Event)) > 0;
}
