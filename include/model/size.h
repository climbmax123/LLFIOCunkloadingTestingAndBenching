#ifndef VWTCHUNKLOADINGTESTINGANDBENCHING_SIZE_H
#define VWTCHUNKLOADINGTESTINGANDBENCHING_SIZE_H

#include <cstdint>

struct Size1d
{
	uint64_t width;
};

struct Size2d
{
	uint64_t width, height;
};

struct Size3d
{
	uint64_t width, height, depth;
};

#endif //VWTCHUNKLOADINGTESTINGANDBENCHING_SIZE_H
