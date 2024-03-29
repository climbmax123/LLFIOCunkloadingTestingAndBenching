//
// Created by lukas on 28.03.24.
//

#ifndef VWTCHUNKLOADINGTESTINGANDBENCHING_MODEL_CHUNKMAPINDICES_H
#define VWTCHUNKLOADINGTESTINGANDBENCHING_MODEL_CHUNKMAPINDICES_H


#include "model/chunk_mask.h"

#include <filesystem>
#include <algorithm>
#include <iostream>
#include "util/volume_information.h"
#include "model/size.h"


struct ChunkMapIndices
{
	std::vector<uint64_t> data;

	Size3d size{};

	explicit ChunkMapIndices(ChunkMask chunk_mask);
};

#endif //VWTCHUNKLOADINGTESTINGANDBENCHING_MODEL_CHUNKMAPINDICES_H
