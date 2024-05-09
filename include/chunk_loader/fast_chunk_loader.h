#ifndef VWTCHUNKLOADINGTESTINGANDBENCHING_CHUNKLOADER_FASTCHUNKLOADER_H
#define VWTCHUNKLOADINGTESTINGANDBENCHING_CHUNKLOADER_FASTCHUNKLOADER_H

#include "chunk_loader.h"

#include <vector>
#include <span>
#include <map>
#include <filesystem>

#include "util/chunked_volume_information.h"
#include "model/crop.h"

class FastChunkLoader : public ChunkLoader
{
private:
	using ChunkMemory = std::span<uint16_t>;

	ChunkedVolumeInformation info;

	Size3d chunk_count;
	Size3d chunk_size;

	std::vector<uint16_t> memory_pool;

	std::vector<ChunkMemory>               free_list;
	std::map<ChunkCoordinate, ChunkMemory> occupied_list;

	std::vector<uint64_t> chunk_indices;

	[[nodiscard]] std::vector<uint64_t>
	read_chunk_position_indices_from_file() const;

	void
	load_chunk(ChunkCoordinate coordinate, ChunkMemory& memory);

	void
	write_chunk(ChunkCoordinate coordinate, ChunkMemory const& memory);

	void
	load_chunk_from_file(ChunkCoordinate coordinate, ChunkMemory& memory) const;

	void
	write_chunk_to_file(ChunkCoordinate coordinate, ChunkMemory const& memory) const;

	[[nodiscard]] uint64_t
	flatten_coordinate(ChunkCoordinate coordinate) const;

	static void
	copy_chunk_layer_to_layer_crop(std::span<uint16_t> const& chunk_layer, Layer& layer, Crop2d crop);

public:
	explicit FastChunkLoader(std::filesystem::path const& chunked_volume_path, uint64_t maximum_memory_consumption);

	~FastChunkLoader() override = default;

	Chunks
	load(ChunkCoordinates const& coordinates) override;

	void
	write(Chunks const& chunks) override;

	Layer
	load_layer(uint64_t z_level, int level_of_detail) override;

	void
	preload(ChunkCoordinates const& coordinates) override;

	ChunkCoordinates
	prepared() override;

	void
	drop(ChunkCoordinates const& coordinates) override;
};

#endif // VWTCHUNKLOADINGTESTINGANDBENCHING_CHUNKLOADER_FASTCHUNKLOADER_H
