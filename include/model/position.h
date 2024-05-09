#ifndef VWTCHUNKLOADINGTESTINGANDBENCHING_MODEL_POSITION_H
#define VWTCHUNKLOADINGTESTINGANDBENCHING_MODEL_POSITION_H

#include <cstdint>
#include <string>

struct Position1d
{
	int64_t x;

	[[nodiscard]] std::string
	to_string(bool compress = false) const;
};

struct Position2d
{
	int64_t y, x;

	[[nodiscard]] std::string
	to_string(bool compress = false) const;

	int64_t
	operator==(Position2d other) const
	{
		return y == other.y && x == other.x;
	};

	int64_t
	operator<(Position2d other) const
	{
		if (y != other.y)
		{ return y < other.y; }
		else
		{ return x < other.x; }
	};
};

struct Position3d
{
	int64_t z, y, x;

	[[nodiscard]] std::string
	to_string(bool compress = false) const;

	int64_t
	operator==(Position3d other) const
	{
		return z == other.z && y == other.y && x == other.x;
	};

	int64_t
	operator<(Position3d other) const
	{
		if (z != other.z)
		{ return z < other.z; }
		if (y != other.y)
		{ return y < other.y; }
		else
		{ return x < other.x; }
	};
};

#endif //VWTCHUNKLOADINGTESTINGANDBENCHING_MODEL_POSITION_H
