#ifndef VWTCHUNKLOADINGTESTINGANDBENCHING_MODEL_LAYER_H
#define VWTCHUNKLOADINGTESTINGANDBENCHING_MODEL_LAYER_H

#include <vector>
#include <cstdint>
#include "size.h"

struct Layer
{
	Size2d size;
	std::vector<uint16_t> data;
};

#endif //VWTCHUNKLOADINGTESTINGANDBENCHING_MODEL_LAYER_H
