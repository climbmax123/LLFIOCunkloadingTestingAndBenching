#ifndef VWTCHUNKLOADINGTESTINGANDBENCHING_MODEL_CHUNK_H
#define VWTCHUNKLOADINGTESTINGANDBENCHING_MODEL_CHUNK_H

#include <span>

#include "model/chunk_coordinate.h"

struct Chunk
{
	ChunkCoordinate     coordinate;
	std::span<uint16_t> data;
};

#endif //VWTCHUNKLOADINGTESTINGANDBENCHING_MODEL_CHUNK_H
