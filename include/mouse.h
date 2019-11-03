#pragma once

#include <cstdint>

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
