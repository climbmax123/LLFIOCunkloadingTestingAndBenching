#ifndef VWTCHUNKLOADINGTESTINGANDBENCHING_MODEL_CROP_H
#define VWTCHUNKLOADINGTESTINGANDBENCHING_MODEL_CROP_H

#include "position.h"
#include "size.h"
#include <string>


struct Crop1d
{
	Position1d position;
	Size1d size;

	[[nodiscard]] std::string
	to_string() const;
};

struct Crop2d
{
	Position2d position;
	Size2d size;

	[[nodiscard]] std::string
	to_string() const;
};

struct Crop3d
{
	Position3d position;
	Size3d size;

	[[nodiscard]] std::string
	to_string() const;
};

#endif //VWTCHUNKLOADINGTESTINGANDBENCHING_MODEL_CROP_H
