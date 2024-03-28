#include "volume_converter/volume_converter.h"

#include <vector>
#include <span>
#include <cassert>
#include "util/volume_information.h"
#include "util/chunked_volume_information.h"
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

uint16_t
to_big_endian(uint16_t value);

static
std::vector<fs::path>
get_all_tif_files(VolumeInformation const& volume_info, fs::path const& tif_volume_directory);

std::vector<TifFile>
open_tif_files(std::span<fs::path> const& tif_file_paths);

TifFile
open_tif_file(fs::path const& path);

std::vector<fs::path>
get_all_mask_files(VolumeInformation const& volume_info, fs::path const& mask_volume_directory);

std::vector<Mask>
open_mask_files(std::span<fs::path> const& mask_file_paths);

Mask
open_mask_file(fs::path const& path);

using ChunkMemory = std::vector<uint16_t>;

std::vector<Chunk>
chunk_tif_files(std::vector<ChunkMemory>& chunk_memory, std::vector<TifFile> const& tif_files, uint64_t chunk_size, uint64_t z);

void
write_tif_file_to_chunks(TifFile const& tif_file, std::vector<Chunk>& chunks, uint64_t chunk_size, uint64_t depth);

void
write_tif_crop_to_chunk(TifFile const& tif_file, Crop2d crop, Chunk& chunk, uint64_t depth);

std::vector<bool>
create_chunk_mask(std::vector<bool>& chunk_mask, std::vector<fs::path> mask_file_paths, uint64_t chunk_size);

bool
are_all_mask_pixels_false(std::vector<bool> const& mask_pixels);

std::vector<bool>
write_mask_crop_to_vector(std::vector<bool>& mask_chunk, Crop2d crop, std::vector<Mask> const& masks);

std::vector<uint64_t>
create_chunk_map_indices(std::vector<uint64_t>& chunk_map_indices, std::vector<bool> const& chunk_mask);

void
write_chunk_map_indices_to_stream(std::ofstream& stream, std::vector<uint64_t> const& chunk_map_indices);

void
write_chunk_to_stream(std::ofstream& stream, Chunk const& chunk);

void
convert_volume_to_compressed_chunked_volume(std::filesystem::path const& volume_directory, std::filesystem::path const& mask_path, uint64_t chunk_size, std::filesystem::path const& compressed_chunked_volume_path)
{
	std::ofstream stream(compressed_chunked_volume_path / "volume.bin", std::ios::out | std::ios::binary);

	if (!stream.is_open())
	{
		std::cout << "could not open compressed chunked volume file" << std::endl;
		throw std::exception();
	}

	VolumeInformation volume_information = VolumeInformation::read_from_file(volume_directory / "meta.json");

	ChunkedVolumeInformation chunked_volume_information
		                         = ChunkedVolumeInformation::from_volume_information(volume_information, chunk_size);
	ChunkedVolumeInformation::write_to_file(chunked_volume_information,
	                                        compressed_chunked_volume_path / "meta.json");

	std::vector<fs::path> mask_file_paths = get_all_mask_files(volume_information, mask_path);
	std::ranges::sort(mask_file_paths);

	Size3d chunk_count = {
		volume_information.width / chunk_size,
		volume_information.height / chunk_size,
		volume_information.slices / chunk_size
	};

	std::vector<bool> chunk_mask(chunk_count.volume());
	chunk_mask = create_chunk_mask(chunk_mask, mask_file_paths, chunk_size);

	std::vector<uint64_t> chunk_map_indices(chunk_count.volume());
	chunk_map_indices = create_chunk_map_indices(chunk_map_indices, chunk_mask);

	// write chunk_map_index to file
	write_chunk_map_indices_to_stream(stream, chunk_map_indices);

	std::vector<fs::path> tif_file_paths = get_all_tif_files(volume_information, volume_directory);
	std::ranges::sort(tif_file_paths);

	std::vector<ChunkMemory> chunk_memory = {
		chunk_count.width * chunk_count.height,
		ChunkMemory(chunk_size * chunk_size * chunk_size)
	};

	for (uint64_t z_index = 0, z = 0; z_index + chunk_size < volume_information.slices; z_index += chunk_size, z++)
	{
		std::span<fs::path> tif_file_path_span = {tif_file_paths.begin() + z_index, chunk_size};

		std::vector<TifFile> tif_files = open_tif_files(tif_file_path_span);

		std::vector<Chunk> chunks = chunk_tif_files(chunk_memory, tif_files, chunk_size, z_index / chunk_size);

		for (uint64_t i = 0; i < chunks.size(); i++)
		{
			if (chunk_mask[z * chunk_count.height * chunk_count.width + i])
			{ write_chunk_to_stream(stream, chunks[i]); }
		}
	}
}

void
write_chunk_to_stream(std::ofstream& stream, Chunk const& chunk)
{
	for (auto value: chunk.data)
	{
		uint64_t converted_value = htobe16(value);
		stream.write(reinterpret_cast<const char *>(&converted_value), sizeof(uint16_t));
	}
}

void
write_chunk_map_indices_to_stream(std::ofstream& stream, std::vector<uint64_t> const& chunk_map_indices)
{
	for (uint64_t map_index: chunk_map_indices)
	{
		uint64_t converted_value = htobe64(map_index);
		stream.write(reinterpret_cast<const char *>(&converted_value), sizeof(uint64_t));
	}
}

std::vector<uint64_t>
create_chunk_map_indices(std::vector<uint64_t>& chunk_map_indices, std::vector<bool> const& chunk_mask)
{
	assert(chunk_mask.size() == chunk_map_indices.size());

	for (uint64_t i = 0, index = 0; i < chunk_map_indices.size(); i++)
	{
		if (chunk_mask[i])
		{
			chunk_map_indices[i] = index;
			index++;
		}
		else
		{ chunk_map_indices[i] = (uint64_t) -1; }
	}
	return chunk_map_indices;
}

std::vector<bool>
create_chunk_mask(std::vector<bool>& chunk_mask, std::vector<fs::path> mask_file_paths, uint64_t chunk_size)
{
	std::vector<bool> pixel_mask(chunk_size * chunk_size * chunk_size);

	for (uint64_t z_index = 0, z = 0; z_index + chunk_size < mask_file_paths.size(); z_index += chunk_size, z++)
	{
		std::span<fs::path> mask_file_path_span = {mask_file_paths.begin() + z_index, chunk_size};

		std::vector<Mask> masks = open_mask_files(mask_file_path_span);

		uint64_t width  = masks[0].width / chunk_size;
		uint64_t height = masks[0].height / chunk_size;

		for (uint64_t y_index = 0, y = 0; y_index + chunk_size < masks[0].height; y_index += chunk_size, y++)
		{
			for (uint64_t x_index = 0, x = 0;
			     x_index + chunk_size < masks[0].width;
			     x_index += chunk_size, x++)
			{
				Crop2d crop = {.position={x_index, y_index}, .size={chunk_size, chunk_size}};
				pixel_mask = write_mask_crop_to_vector(pixel_mask, crop, masks);

				chunk_mask[z * height * width + y * width + x] = are_all_mask_pixels_false(pixel_mask);
			}
		}
	}
	return chunk_mask;
}

std::vector<bool>
write_mask_crop_to_vector(std::vector<bool>& mask_chunk, Crop2d crop, std::vector<Mask> const& masks)
{
	for (uint64_t z = 0; z < masks.size(); z++)
	{
		for (uint64_t y_index = crop.position.y, y = 0; y_index < crop.size.height; y_index++, y++)
		{
			for (uint64_t x_index = crop.position.x, x = 0; x_index < crop.size.width; x_index++, x++)
			{
				mask_chunk[z * crop.size.height * crop.size.width + y * crop.size.width + x]
					= masks[z].data[y_index * masks[z].width + x_index];
			}
		}
	}
	return mask_chunk;
}

bool
are_all_mask_pixels_false(std::vector<bool> const& mask_pixels)
{
	return std::all_of(mask_pixels.begin(), mask_pixels.end(), [](bool value)
	{ return !value; });
}

std::vector<fs::path>
get_all_tif_files(VolumeInformation const& volume_info, fs::path const& tif_volume_directory)
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
	{ tif_files.push_back(open_tif_file(tif_file_path)); }

	return tif_files;
}

TifFile
open_tif_file(fs::path const& path)
{
	cv::Mat image = cv::imread(path, cv::IMREAD_UNCHANGED);

	uint64_t height = image.size[0];
	uint64_t width  = image.size[1];

	TifFile tif_file = {width, height, std::vector<uint16_t>(width * height)};

	for (uint64_t index = 0, y = 0; y < height; y++)
	{
		for (uint64_t x = 0; x < width; x++, index++)
		{ tif_file.data[index] = image.at<uint16_t>(y, x); }
	}

	return tif_file;
}

std::vector<fs::path>
get_all_mask_files(VolumeInformation const& volume_info, fs::path const& mask_volume_directory)
{
	std::vector<fs::path> files;
	files.reserve(volume_info.slices);

	for (fs::directory_entry const& entry: fs::directory_iterator(mask_volume_directory))
	{
		if (entry.path().extension() == ".png")
		{ files.push_back(entry.path()); }
	}

	return files;
}

std::vector<Mask>
open_mask_files(std::span<fs::path> const& mask_file_paths)
{
	std::vector<Mask> masks;

	for (fs::path const& mask_file_path: mask_file_paths)
	{ masks.emplace_back(open_mask_file(mask_file_path)); }

	return masks;
}

Mask
open_mask_file(fs::path const& path)
{
	cv::Mat image = cv::imread(path, cv::IMREAD_UNCHANGED);

	uint64_t height = image.size[0];
	uint64_t width  = image.size[1];

	Mask mask = {width, height, std::vector<bool>(width * height)};

	for (uint64_t index = 0, y = 0; y < height; y++)
	{
		for (int x = 0; x < width; x++, index++)
		{ mask.data[index] = 0 < image.at<uint16_t>(y, x); }
	}

	return mask;
}

std::vector<Chunk>
chunk_tif_files(std::vector<ChunkMemory>& chunk_memory, std::vector<TifFile> const& tif_files, uint64_t chunk_size, uint64_t z)
{
	assert(tif_files.size() == chunk_size && "count of tif files is not equal to chunk size");

	Size2d chunk_count = {.width=tif_files[0].width / chunk_size, .height=tif_files[0].height / chunk_size};

	std::vector<Chunk> chunks(chunk_count.width * chunk_count.height);

	// fill in coordinates
	for (int64_t index = 0, y = 0; y < chunk_count.width; y++)
	{
		for (int64_t x = 0; x < chunk_count.width; x++, index++)
		{
			chunks[index].coordinate = {x, y, static_cast<int64_t>(z)};
			chunks[index].data       = chunk_memory[index];
		}
	}

	// fill in data
	for (uint64_t depth = 0; depth < tif_files.size(); depth++)
	{ write_tif_file_to_chunks(tif_files[depth], chunks, chunk_size, depth); }

	return chunks;
}

void
write_tif_file_to_chunks(TifFile const& tif_file, std::vector<Chunk>& chunks, uint64_t chunk_size, uint64_t depth)
{
	Size2d chunk_count = {.width = tif_file.height / chunk_size, .height = tif_file.width / chunk_size};

	for (uint64_t y = 0; y < chunk_count.height; y++)
	{
		for (uint64_t x = 0; x < chunk_count.width; x++)
		{
			Crop2d crop = {.position={x * chunk_size, y * chunk_size}, .size={chunk_size, chunk_size}};
			write_tif_crop_to_chunk(tif_file, crop, chunks[y * chunk_count.width + x], depth);
		}
	}
}

void
write_tif_crop_to_chunk(TifFile const& tif_file, Crop2d crop, Chunk& chunk, uint64_t depth)
{
	uint64_t index = depth * crop.size.height * crop.size.width;

	for (uint64_t y = crop.position.y; y < crop.position.y + crop.size.height; y++)
	{
		for (uint64_t x = crop.position.x; x < crop.position.x + crop.size.width; x++, index++)
		{ chunk.data[index] = tif_file.data[y * tif_file.width + x]; }
	}
}

namespace chunk_files
{
	void
	write_chunk_to_file(std::ofstream& stream, ChunkCoordinate coordinate, std::vector<TifFile> const& tif_files, int64_t chunk_size);
}

void
convert_volume_to_chunk_files(std::filesystem::path const& volume_directory, std::filesystem::path const& chunked_volume_path, int64_t chunk_size)
{
	VolumeInformation volume_information = VolumeInformation::read_from_file(volume_directory / "meta.json");

	ChunkedVolumeInformation chunked_volume_information
		                         = ChunkedVolumeInformation::from_volume_information(volume_information, chunk_size);
	ChunkedVolumeInformation::write_to_file(chunked_volume_information,
	                                        chunked_volume_path / "meta.json");

	std::vector<fs::path> tif_file_paths = get_all_tif_files(volume_information, volume_directory);
	std::ranges::sort(tif_file_paths);

	// chunks in z direction
	for (int64_t z = 0; z + chunk_size < volume_information.slices; z += chunk_size)
	{
		std::vector<TifFile> tif_files = open_tif_files(std::span(tif_file_paths.begin() + z, chunk_size));

		// chunks in y direction
		for (int64_t y = 0; y + chunk_size < volume_information.height; y += chunk_size)
		{
			// chunks in x direction
			for (int64_t x = 0; x + chunk_size < volume_information.width; x += chunk_size)
			{
				ChunkCoordinate coordinate = {.x=x / chunk_size, .y=y / chunk_size, .z=z / chunk_size};

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

				chunk_files::write_chunk_to_file(stream, coordinate, tif_files, chunk_size);
			}
		}
	}
}

void
chunk_files::write_chunk_to_file(std::ofstream& stream, ChunkCoordinate coordinate, std::vector<TifFile> const& tif_files, int64_t chunk_size)
{
	for (int64_t z = 0; z < chunk_size; z++)
	{
		for (int64_t y = 0; y < chunk_size; y++)
		{
			for (int64_t x = 0; x < chunk_size; x++)
			{
				uint64_t xy_index = (coordinate.y * chunk_size + y) * tif_files[0].width
				                    + coordinate.x * chunk_size + x;

				uint16_t converted_value = htobe16(tif_files[z].data[xy_index]);
				stream.write(reinterpret_cast<const char *>(&converted_value), sizeof(uint16_t));
			}
		}
	}
}
