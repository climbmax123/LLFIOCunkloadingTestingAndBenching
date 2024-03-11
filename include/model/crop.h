#ifndef VWTCHUNKLOADINGTESTINGANDBENCHING_MODEL_CROP_H
#define VWTCHUNKLOADINGTESTINGANDBENCHING_MODEL_CROP_H

#include "position.h"
#include "size.h"


struct Crop1d
{
	uint64_t position;
	uint64_t size;
};

struct Crop2d
{
	Position2d position;
	Size2d size;
};

struct Crop3d
{
	Position3d position;
	Size3d size;
};

#endif //VWTCHUNKLOADINGTESTINGANDBENCHING_MODEL_CROP_H
