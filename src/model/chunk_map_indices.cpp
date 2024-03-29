#include "model/chunk_map_indices.h"

ChunkMapIndices::ChunkMapIndices(ChunkMask chunk_mask)
{
	data = std::vector<uint64_t>(chunk_mask.size.volume());
	size = chunk_mask.size;

	for (uint64_t i = 0, index = 0; i < data.size(); i++)
	{
		if (chunk_mask.data[i])
		{ data[i] = index++; }
		else
		{ data[i] = (uint64_t) -1; }
	}
}
