#ifndef VWTCHUNKLOADINGTESTINGANDBENCHING_MODEL_SIZE_H
#define VWTCHUNKLOADINGTESTINGANDBENCHING_MODEL_SIZE_H

#include <cstdint>
#include <string>

struct Size1d
{
	uint64_t length;

	[[nodiscard]] std::string
	to_string() const;
};

struct Size2d
{
	uint64_t height, width;

	[[nodiscard]] uint64_t
	area() const;

	[[nodiscard]] std::string
	to_string() const;
};

struct Size3d
{
	uint64_t depth, height, width;

	[[nodiscard]] uint64_t
	volume() const;

	[[nodiscard]] uint64_t
	area_top() const;

	[[nodiscard]] uint64_t
	area_side() const;

	[[nodiscard]] uint64_t
	area_front() const;

	[[nodiscard]] std::string
	to_string() const;
};

#endif //VWTCHUNKLOADINGTESTINGANDBENCHING_MODEL_SIZE_H
