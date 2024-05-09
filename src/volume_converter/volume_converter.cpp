#include "volume_converter/volume_converter.h"

#include <vector>
#include <span>
#include <cassert>
#include "util/volume_information.h"
#include "util/chunked_volume_information.h"
#include "model/chunk_mask.h"
#include "model/chunk_map_indices.h"
#include "log/debug/visualizer.h"
#include <opencv2/opencv.hpp>
#include <fstream>

#if defined(__linux__)

#include <endian.h>

#elif defined(__APPLE__)
#include <libkern/OSByteOrder.h>
#define htobe64 OSSwapHostToBigInt64
#define htobe16 OSSwapHostToBigConstInt16
#elif defined(_WIN32)
#include <winsock2.h>
#include <sys/param.h>
// Windows-specific implementation of htobe64 if not available
uint64_t htobe64(uint64_t host_64bits) {
    uint64_t retval = htonll(host_64bits);
    return retval;
}
#define htobe16 htons
#else
#error Platform not supported
#endif


namespace fs = std::filesystem;

static
void
write_chunk_map_indices_to_stream(std::ofstream& stream, ChunkMapIndices const& chunk_map_indices);

static
void
write_chunks_to_stream(std::ofstream& stream, ChunkedVolumeInformation const& volume_information, fs::path const& volume_path, ChunkMask const& chunk_mask);

static
std::vector<fs::path>
get_all_tif_file_paths(ChunkedVolumeInformation const& volume_info, fs::path const& tif_volume_directory);

using ChunkMemory = std::vector<uint16_t>;

static
std::vector<TifFile>
open_tif_files(std::span<fs::path> const& tif_file_paths);

static
void
write_tif_files_to_chunks(std::vector<ChunkMemory>& chunks, std::vector<TifFile> const& tif_files, Size3d chunk_size);

static
void
write_tif_file_to_chunks(std::vector<ChunkMemory>& chunks, TifFile const& tif_file, Size3d chunk_size, int64_t depth);

static
void
write_tif_crop_to_chunk(ChunkMemory& chunk, TifFile const& tif_file, Crop2d crop, int64_t depth);

static
void
write_chunk_to_stream(std::ofstream& stream, ChunkMemory const& chunk);

void
convert_volume_to_compressed_chunked_volume(fs::path const& volume_path, fs::path const& mask_path, uint64_t chunk_size, fs::path const& compressed_and_chunked_volume_path)
{
	// TODO(Lukas Karafiat): silently create the directory structure
	VolumeInformation volume_information = VolumeInformation::read_from_file(volume_path / "meta.json");
	std::cout << "volume_information = " << volume_information.to_string() << std::endl;

	ChunkedVolumeInformation chunked_volume_information;

	// copy the volume information
	{
		using CVI = ChunkedVolumeInformation;
		chunked_volume_information = CVI::from_volume_information(volume_information, chunk_size);
		CVI::write_to_file(chunked_volume_information, compressed_and_chunked_volume_path / "meta.json");
	}
	std::cout << "copied volume information" << std::endl;

	std::ofstream stream(compressed_and_chunked_volume_path / "volume.bin", std::ios::out | std::ios::binary);
	if (!stream.is_open())
	{
		std::stringstream ss;
		ss << "could not open compressed chunked volume file: " << compressed_and_chunked_volume_path;
		throw std::runtime_error(ss.str());
	}

	ChunkMask chunk_mask(volume_information, mask_path, chunk_size);
	std::cout << "created the chunk mask" << std::endl;

	ChunkMapIndices chunk_map_indices(chunk_mask);
	std::cout << "created the chunk index map" << std::endl;

	write_chunk_map_indices_to_stream(stream, chunk_map_indices);
	std::cout << "written all chunk map indices" << std::endl;

	write_chunks_to_stream(stream, chunked_volume_information, volume_path, chunk_mask);
	std::cout << "written all chunks" << std::endl;
	stream.flush();
	stream.close();
}

void
write_chunk_map_indices_to_stream(std::ofstream& stream, ChunkMapIndices const& chunk_map_indices)
{
	for (uint64_t map_index: chunk_map_indices.data)
	{
		uint64_t converted_value = htobe64(map_index);
		stream.write(reinterpret_cast<const char *>(&converted_value), sizeof(uint64_t));
	}
}

void
write_chunks_to_stream(std::ofstream& stream, ChunkedVolumeInformation const& volume_information, fs::path const& volume_path, ChunkMask const& chunk_mask)
{
	std::vector<fs::path> tif_file_paths = get_all_tif_file_paths(volume_information, volume_path);
	std::ranges::sort(tif_file_paths);

	Size3d chunk_size = {
		.depth  = volume_information.chunk_size,
		.height = volume_information.chunk_size,
		.width  = volume_information.chunk_size
	};

	Size3d chunk_count = {
		.depth  = volume_information.slices / chunk_size.depth,
		.height = volume_information.height / chunk_size.height,
		.width  = volume_information.width / chunk_size.width
	};

	std::vector<ChunkMemory> chunks = {
		chunk_count.area_top(),
		ChunkMemory(chunk_size.volume())
	};

	for (int64_t z = 0; z < chunk_count.depth; z++)
	{
		auto index_z = static_cast<int64_t>(z * chunk_size.depth);

		std::vector<TifFile> tif_files = open_tif_files({tif_file_paths.begin() + index_z, chunk_size.depth});

		write_tif_files_to_chunks(chunks, tif_files, chunk_size);

		for (uint64_t i = 0; i < chunks.size(); i++)
		{
			if (chunk_mask.at(z, i / chunk_count.width, i % chunk_count.width))
			{ write_chunk_to_stream(stream, chunks[i]); }
		}
		std::cout << "written chunk data from " << index_z << " to " << (index_z + chunk_size.depth)
		          << std::endl;
	}
}

std::vector<fs::path>
get_all_tif_file_paths(ChunkedVolumeInformation const& volume_info, fs::path const& tif_volume_directory)
{
	std::vector<fs::path> files;
	files.reserve(volume_info.slices);

	for (fs::directory_entry const& entry: fs::directory_iterator(tif_volume_directory))
	{
		if (entry.path().extension() == ".tif")
		{ files.push_back(entry.path()); }
	}

	return files;
}

std::vector<TifFile>
open_tif_files(std::span<fs::path> const& tif_file_paths)
{
	std::vector<TifFile> tif_files;

	for (fs::path const& tif_file_path: tif_file_paths)
	{ tif_files.push_back(TifFile::read_from_image_file(tif_file_path)); }

	return tif_files;
}

void
write_tif_files_to_chunks(std::vector<ChunkMemory>& chunks, std::vector<TifFile> const& tif_files, Size3d chunk_size)
{
	assert(tif_files.size() == chunk_size.depth && "count of tif files is not equal to chunk size");

	Size2d chunk_count = {
		.height = tif_files[0].size.height / chunk_size.height,
		.width  = tif_files[0].size.width / chunk_size.width
	};

	assert(chunk_count.area() == chunks.size() && "chunk count and reserved buffer have not same size");

	// fill in data
	for (int64_t depth = 0; depth < tif_files.size(); depth++)
	{ write_tif_file_to_chunks(chunks, tif_files[depth], chunk_size, depth); }
}

void
write_tif_file_to_chunks(std::vector<ChunkMemory>& chunks, TifFile const& tif_file, Size3d chunk_size, int64_t depth)
{
	Size2d chunk_count = {
		.height = tif_file.size.width / chunk_size.depth,
		.width = tif_file.size.height / chunk_size.height
	};

	for (int64_t y = 0; y < chunk_count.height; y++)
	{
		for (int64_t x = 0; x < chunk_count.width; x++)
		{
			Crop2d crop = {
				.position={
					.y = static_cast<int64_t>(y * chunk_size.height),
					.x = static_cast<int64_t>(x * chunk_size.width)
				},
				.size={chunk_size.height, chunk_size.width}};
			write_tif_crop_to_chunk(chunks[y * chunk_count.width + x], tif_file, crop, depth);
		}
	}
}

void
write_tif_crop_to_chunk(ChunkMemory& chunk, TifFile const& tif_file, Crop2d crop, int64_t depth)
{
	auto index = static_cast<int64_t>(depth * crop.size.height * crop.size.width);

	for (int64_t y = crop.position.y; y < crop.position.y + crop.size.height; y++)
	{
		for (int64_t x = crop.position.x; x < crop.position.x + crop.size.width; x++, index++)
		{ chunk[index] = tif_file.at({y, x}); }
	}
}

void
write_chunk_to_stream(std::ofstream& stream, ChunkMemory const& chunk)
{
	for (auto value: chunk)
	{
		uint64_t converted_value = htobe16(value);
		stream.write(reinterpret_cast<const char *>(&converted_value), sizeof(uint16_t));
	}
}

namespace chunk_files
{
	void
	write_chunk_to_file(std::ofstream& stream, std::vector<TifFile> const& tif_files, int64_t chunk_size);
}

void
convert_volume_to_chunk_files(std::filesystem::path const& volume_directory, std::filesystem::path const& chunked_volume_path, uint64_t chunk_size)
{
	VolumeInformation volume_information = VolumeInformation::read_from_file(volume_directory / "meta.json");

	ChunkedVolumeInformation chunked_volume_information;

	// copy the volume information
	{
		using CVI = ChunkedVolumeInformation;
		chunked_volume_information = CVI::from_volume_information(volume_information, chunk_size);
		CVI::write_to_file(chunked_volume_information, chunked_volume_path / "meta.json");
	}

	std::vector<fs::path> tif_file_paths = get_all_tif_file_paths(chunked_volume_information, volume_directory);
	std::ranges::sort(tif_file_paths);

	Size3d chunk_count = {
		.depth  = volume_information.slices / chunk_size,
		.height = volume_information.height / chunk_size,
		.width  = volume_information.width / chunk_size,
	};

	for (int64_t z = 0; z < chunk_count.depth; z++)
	{
		auto index_z = static_cast<int64_t>(z * chunk_size);

		std::vector<TifFile> tif_files = open_tif_files({tif_file_paths.begin() + index_z, chunk_size});

		for (int64_t y = 0; y < chunk_count.height; y++)
		{
			for (int64_t x = 0; x < chunk_count.width; x++)
			{
				ChunkCoordinate coordinate = {.z=z, .y=y, .x=x};

				std::stringstream file_name;
				file_name << std::setfill('0') << std::setw(4) << coordinate.z << "_";
				file_name << std::setfill('0') << std::setw(4) << coordinate.y << "_";
				file_name << std::setfill('0') << std::setw(4) << coordinate.x;
				file_name << ".bin";

				std::ofstream
					stream(chunked_volume_path / file_name.str(), std::ios::out | std::ios::binary);

				if (!stream.is_open())
				{
					std::cout << "could not open chunked file" << std::endl;
					throw std::exception();
				}

				chunk_files::write_chunk_to_file(stream, tif_files, chunk_size);
			}
		}
	}
}

void
chunk_files::write_chunk_to_file(std::ofstream& stream, std::vector<TifFile> const& tif_files, int64_t chunk_size)
{
	for (int64_t z = 0; z < chunk_size; z++)
	{
		for (int64_t y = 0; y < chunk_size; y++)
		{
			for (int64_t x = 0; x < chunk_size; x++)
			{
				uint16_t converted_value = htobe16(tif_files[z].at({y, x}));
				stream.write(reinterpret_cast<const char *>(&converted_value), sizeof(uint16_t));
			}
		}
	}
}
