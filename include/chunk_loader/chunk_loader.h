#ifndef VWTCHUNKLOADINGTESTINGANDBENCHING_CHUNKLOADER_CHUNKLOADER_H
#define VWTCHUNKLOADINGTESTINGANDBENCHING_CHUNKLOADER_CHUNKLOADER_H

#include <vector>

#include "model/chunk_coordinate.h"
#include "model/chunk.h"

class ChunkLoader
{
public:
	using ChunkCoordinates = std::vector<ChunkCoordinate>;
	using Chunks = std::vector<Chunk>;

	// get_all_chunks_by_coordinate_in
	virtual Chunks
	load(ChunkCoordinates const& coordinates) = 0;

	// change_all_chunks_in
	virtual void
	write(Chunks const& chunks) = 0;

	// additional:

	// preload_all_chunks_by_coordinate_in
	virtual void
	preload(ChunkCoordinates const& coordinates) = 0;

	// get_all_prepared_chunks_as_coordinates
	virtual ChunkCoordinates
	prepared() = 0;

	// drop_all_chunks_by_coordinate_in
	virtual void
	drop(ChunkCoordinates const& coordinates) = 0;

	virtual ~ChunkLoader() = default;
};

#endif // VWTCHUNKLOADINGTESTINGANDBENCHING_CHUNKLOADER_CHUNKLOADER_H
