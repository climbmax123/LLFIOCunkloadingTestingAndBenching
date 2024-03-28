#include "chunk_loader/fast_chunk_loader.h"

#include <bit>
#include <llfio.hpp>
#include <ranges>

namespace fs = std::filesystem;
namespace llfio = LLFIO_V2_NAMESPACE;

static llfio::mapped_file_handle mapped_file_handle;

static inline uint16_t
from_big_endian(uint16_t value);

static inline uint16_t
to_big_endian(uint16_t value);

FastChunkLoader::FastChunkLoader(std::filesystem::path const& chunked_volume_path, uint64_t maximum_memory_consumption)
{
	info = ChunkedVolumeInformation::read_from_file(chunked_volume_path / "meta.json");

	uint64_t chunk_pixel_count = info.chunk_size * info.chunk_size * info.chunk_size;
	uint64_t chunk_memory_size = chunk_pixel_count * sizeof(uint16_t);

	uint64_t chunk_count_for_memory = maximum_memory_consumption / chunk_memory_size;

	memory_pool = std::vector<uint16_t>(chunk_count_for_memory * chunk_pixel_count);

	free_list = std::vector<std::span<uint16_t>>(chunk_count_for_memory);

	for (int i = 0; i < chunk_count_for_memory; i++)
	{ free_list[i] = std::span<uint16_t>(memory_pool.data() + i * chunk_pixel_count, chunk_pixel_count); }

	extern llfio::mapped_file_handle mapped_file_handle;

	mapped_file_handle = llfio::mapped_file({}, chunked_volume_path
	                                            / "volume.bin", llfio::file_handle::mode::write).value();

	chunk_dimension.width  = info.chunk_size;
	chunk_dimension.height = info.chunk_size;
	chunk_dimension.depth  = info.chunk_size;

	chunk_count.x = info.width / chunk_dimension.width;
	chunk_count.y = info.height / chunk_dimension.height;
	chunk_count.z = info.slices / chunk_dimension.depth;

	chunk_indices = read_chunk_position_indices_from_file();
}

ChunkLoader::Chunks
FastChunkLoader::load(ChunkCoordinates const& coordinates)
{
	// prepare return values
	std::vector<Chunk> return_values(coordinates.size());

	uint64_t index = 0;

	std::vector<ChunkCoordinate> not_found_coordinates;
	not_found_coordinates.reserve(coordinates.size());

	// search already stored chunks + add to list
	for (ChunkCoordinate const& coordinate: coordinates)
	{
		if (occupied_list.find(coordinate) == occupied_list.end())
		{ not_found_coordinates.push_back(coordinate); }
		else
		{ return_values[index++] = {coordinate, occupied_list[coordinate]}; }
	}

	if (free_list.size() < not_found_coordinates.size())
	{
		throw std::exception();
	}

	// fill chunks
#pragma omp parallel for
	for (uint64_t i = 0; i < not_found_coordinates.size(); i++)
	{ load_chunk(not_found_coordinates[i], free_list[free_list.size() - 1 - i]); }

	// take chunks
	for (ChunkCoordinate const& coordinate: not_found_coordinates)
	{
		occupied_list
			.insert(std::pair<ChunkCoordinate, std::span<uint16_t>>(
				ChunkCoordinate(coordinate), std::span<uint16_t>(free_list.back())));
		return_values[index++] = {coordinate, free_list.back()};
		free_list.pop_back();
	}

	return return_values;
}

void
FastChunkLoader::load_chunk(ChunkCoordinate coordinate, ChunkMemory& memory)
{
	// NOTE(Lukas Karafiat): the index -1 should be large enough to not be a problem until around 18 quintillion chunks
	uint64_t chunk_index = chunk_indices[flatten_coordinate(coordinate)];
	if (chunk_index == (int64_t) -1)
	{
		// pretend to load new empty chunk
		std::fill(memory.begin(), memory.end(), 0);
	}
	else
	{ load_chunk_from_file(coordinate, memory); }
}

void
FastChunkLoader::load_chunk_from_file(ChunkCoordinate coordinate, ChunkMemory& memory) const
{
	uint64_t offset       = chunk_count.z * chunk_count.y * chunk_count.x * sizeof(uint64_t);
	auto     *file_memory = reinterpret_cast<uint16_t *>(mapped_file_handle.address() + offset);

	uint64_t chunk_size = chunk_dimension.depth * chunk_dimension.height * chunk_dimension.width;

	uint64_t first_index = chunk_indices[flatten_coordinate(coordinate)] * chunk_size;

	std::span<uint16_t> needed_file_span(file_memory + first_index, chunk_size);

	std::ranges::copy(needed_file_span, memory.begin());

	// convert from big-endian format to native
	std::ranges::transform(memory, memory.begin(), from_big_endian);
}

void
FastChunkLoader::write(Chunks const& chunks)
{
#pragma omp parallel for
	for (Chunk const& chunk: chunks)
	{ write_chunk(chunk.coordinate, chunk.data); }
}

void
FastChunkLoader::write_chunk(ChunkCoordinate coordinate, ChunkMemory const& memory)
{
	if (chunk_indices[flatten_coordinate(coordinate)] == (uint64_t) -1)
	{ /* ignore */ }
	else
	{ write_chunk_to_file(coordinate, memory); }
}

void
FastChunkLoader::write_chunk_to_file(ChunkCoordinate coordinate, ChunkMemory const& memory) const
{
	uint64_t offset       = chunk_count.z * chunk_count.y * chunk_count.x * sizeof(uint64_t);
	auto     *file_memory = reinterpret_cast<uint16_t *>(mapped_file_handle.address() + offset);

	uint64_t chunk_size = chunk_dimension.depth * chunk_dimension.height * chunk_dimension.width;

	uint64_t first_index = chunk_indices[flatten_coordinate(coordinate)] * chunk_size;

	std::span<uint16_t> needed_file_span(file_memory + first_index, chunk_size);

	std::ranges::copy(memory, needed_file_span.begin());

	// convert from native format to big-endian
	std::ranges::transform(needed_file_span, needed_file_span.begin(), to_big_endian);
}

void
FastChunkLoader::preload(ChunkCoordinates const& coordinates)
{}

ChunkLoader::ChunkCoordinates
FastChunkLoader::prepared()
{
	std::ranges::elements_view   chunk_coordinates = std::views::keys(occupied_list);
	std::vector<ChunkCoordinate> prepared{chunk_coordinates.begin(), chunk_coordinates.end()};

	return prepared;
}

void
FastChunkLoader::drop(ChunkCoordinates const& coordinates)
{
	for (ChunkCoordinate const& coordinate: coordinates)
	{
		if (occupied_list.find(coordinate) != occupied_list.end())
		{
			free_list.push_back(occupied_list.at(coordinate));
			occupied_list.erase(coordinate);
		}
	}
}

std::vector<uint64_t>
FastChunkLoader::read_chunk_position_indices_from_file() const
{
	uint64_t chunks_count = chunk_count.z * chunk_count.y * chunk_count.x;

	auto *file_memory = reinterpret_cast<uint64_t *>(mapped_file_handle.address());

	std::span<uint64_t>   needed_file_span(file_memory, chunks_count);
	std::vector<uint64_t> chunk_indices(chunks_count);

	std::ranges::copy(needed_file_span, chunk_indices.begin());

	// convert from native format to big-endian
	std::ranges::transform(chunk_indices, chunk_indices.begin(), to_big_endian);

	return chunk_indices;
}

uint64_t
FastChunkLoader::flatten_coordinate(ChunkCoordinate coordinate) const
{
	return coordinate.z * chunk_count.y * chunk_count.x + coordinate.y * chunk_count.x + coordinate.x;
}

Layer
FastChunkLoader::load_layer(uint64_t z_level, int level_of_detail)
{
	Layer layer;

	layer.size = {
		.width = this->chunk_count.x * this->chunk_dimension.width,
		.height = this->chunk_count.y * this->chunk_dimension.height
	};

	// TODO load layer from chunks, with specific resolution

	return layer;
}

// Function to convert uint16_t from big endian
uint16_t
from_big_endian(uint16_t value)
{
	if constexpr (std::endian::native == std::endian::little)
	{
		return (value >> 8) | (value << 8);
	}
	else
	{
		return value; // already big-endian
	}
}

// Function to convert uint16_t to big endian
uint16_t
to_big_endian(uint16_t value)
{
	if constexpr (std::endian::native == std::endian::little)
	{
		return (value >> 8) | (value << 8);
	}
	else
	{
		return value; // already big-endian
	}
}