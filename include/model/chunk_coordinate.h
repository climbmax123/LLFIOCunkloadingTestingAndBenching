#ifndef VWTCHUNKLOADINGTESTINGANDBENCHING_MODEL_CHUNK_COORDINATE_H
#define VWTCHUNKLOADINGTESTINGANDBENCHING_MODEL_CHUNK_COORDINATE_H

#include <cstdint>

struct ChunkCoordinate
{
	int64_t x, y, z;

	int64_t
	operator==(ChunkCoordinate other) const
	{
		return x == other.x && y == other.y && z == other.z;
	};

	int64_t
	operator<(ChunkCoordinate other) const
	{
		if (x != other.x)
		{
			return x < other.x;
		}
		if (y != other.y)
		{
			return y < other.y;
		}
		return z < other.z;
	};
};

#endif //VWTCHUNKLOADINGTESTINGANDBENCHING_MODEL_CHUNK_COORDINATE_H
