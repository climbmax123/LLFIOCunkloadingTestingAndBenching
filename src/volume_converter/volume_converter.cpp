#include "volume_converter/volume_converter.h"

#include <vector>
#include <span>
#include <cassert>
#include "util/volume_information.h"
#include <opencv2/opencv.hpp>
#include <fstream>

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

std::vector<Chunk>
cull_chunks(std::vector<Chunk> const& chunks, std::vector<Mask> const& masks, Size2d chunk_count, uint64_t chunk_size);

bool
should_chunk_be_removed(const Chunk& chunk, std::vector<Mask> const& masks, uint64_t chunk_size);

void
convert_volume_to_compressed_chunked_volume()
{
	fs::path          volume_directory   = "../data/campfire/tif-volume";
	VolumeInformation volume_information = VolumeInformation::read_from_file(volume_directory / "meta.json");

	std::vector<fs::path> tif_file_paths = get_all_tif_files(volume_information, volume_directory);
	std::ranges::sort(tif_file_paths);

	std::vector<fs::path> mask_file_paths = get_all_mask_files(volume_information, volume_directory);
	std::ranges::sort(mask_file_paths);

	uint64_t chunk_size = 128;

	Size2d flat_chunk_count = {volume_information.width / chunk_size, volume_information.height / chunk_size};

	Size3d chunk_count = {
		volume_information.width / chunk_size, volume_information.height / chunk_size,
		volume_information.slices / chunk_size
	};

	uint64_t running_chunk_index = 0;

	std::vector<uint64_t>
		chunk_mapping_array(volume_information.slices / chunk_size * chunk_count.height * chunk_count.width);

	for (uint64_t z_index = 0; z_index + chunk_size < volume_information.slices; z_index += chunk_size)
	{
		std::span<fs::path> tif_file_path_span = {tif_file_paths.begin() + z_index, chunk_size};

		std::vector<TifFile> tif_files = open_tif_files(tif_file_path_span);

		std::vector<ChunkMemory> chunk_memory = {
			chunk_count.width * chunk_count.height,
			ChunkMemory(chunk_size * chunk_size * chunk_size)
		};

		std::vector<Chunk> chunks = chunk_tif_files(chunk_memory, tif_files, chunk_size, z_index / chunk_size);

		std::span<fs::path> mask_file_path_span = {mask_file_paths.begin() + z_index, chunk_size};

		std::vector<Mask> masks = open_mask_files(mask_file_path_span);

		chunks = cull_chunks(chunks, masks, flat_chunk_count, chunk_size);

		for (Chunk const& chunk: chunks)
		{
			uint64_t index = chunk.coordinate.z * chunk_count.height * chunk_count.width
			                 + chunk.coordinate.y * chunk_count.width
					 + chunk.coordinate.x;
			chunk_mapping_array[index] = running_chunk_index;
			running_chunk_index++;
		}
		// add chunks to mapping array and increase running sum
		// TODO: write chunks to file
	}

	// TODO: prepend the mapping array to file
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

std::vector<Chunk>
cull_chunks(std::vector<Chunk>& chunks, std::vector<Mask> const& masks, Size2d chunk_count, uint64_t chunk_size)
{
	std::vector<bool> marked_deleted;

	// Iterate in reverse to safely remove elements without affecting the iteration
	for (int64_t i = chunks.size() - 1; i >= 0; i--)
	{
		if (should_chunk_be_removed(chunks[i], masks, chunk_size))
		{
			chunks.erase(chunks.begin() + i);
		}
	}

	return chunks;
}

bool
should_chunk_be_removed(const Chunk& chunk, std::vector<Mask> const& masks, uint64_t chunk_size)
{
	for (uint64_t z = 0; z < chunk_size; z++)
	{
		for (uint64_t y = 0; y < chunk_size; y++)
		{
			for (uint64_t x = 0; x < chunk_size; x++)
			{
				uint64_t y_index = chunk.coordinate.y * chunk_size + y;
				uint64_t x_index = chunk.coordinate.x * chunk_size + x;
				if (masks[z].data[y_index * masks[z].width + x_index])
				{
					return true;
				}
			}
		}
	}

	return false;
}

bool
is_mask_crop_empty(std::vector<Mask> const& masks, Crop3d crop)
{
	for (uint64_t z = crop.position.z; z <= crop.position.z + crop.size.depth; ++z)
	{
		for (uint64_t y = crop.position.y; y <= crop.position.y + crop.size.height; ++y)
		{
			for (uint64_t x = crop.position.x; x <= crop.position.x + crop.size.width; ++x)
			{
				if (masks[z].data[y + x])
				{ return false; }
			}
		}
	}

	// If we went through the entire crop region without finding a true value, it's empty
	return true;
}

*/

namespace chunk_files
{
	void
	write_chunk_to_file(ChunkCoordinate coordinate, std::vector<TifFile> const& tif_files, int64_t chunk_size);
}


void
convert_volume_to_chunk_files(std::filesystem::path const& volume_directory, int64_t chunk_size)
{
	VolumeInformation volume_information = VolumeInformation::read_from_file(volume_directory / "meta.json");

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
				chunk_files::write_chunk_to_file(coordinate, tif_files, chunk_size);
			}
		}
	}
}

void
chunk_files::write_chunk_to_file(ChunkCoordinate coordinate, std::vector<TifFile> const& tif_files, int64_t chunk_size)
{
	std::stringstream file_name;

	file_name << std::setfill('0') << std::setw(4) << coordinate.z << "_";
	file_name << std::setfill('0') << std::setw(4) << coordinate.y << "_";
	file_name << std::setfill('0') << std::setw(4) << coordinate.x;
	file_name << ".bin";

	// Open a file in binary mode
	std::ofstream file(file_name.str(), std::ios::out | std::ios::binary);
	if (!file)
	{
		std::cerr << "Failed to open file for writing." << std::endl;
		throw std::exception();
	}

	for (int64_t z = 0; z < chunk_size; z++)
	{
		for (int64_t y = 0; y < chunk_size; y++)
		{
			for (int64_t x = 0; x < chunk_size; x++)
			{
				uint64_t xy_index = (coordinate.y * chunk_size + y) * tif_files[0].width
				                    + coordinate.x * chunk_size + x;

				uint64_t z_index = coordinate.z * chunk_size + z;
				uint16_t value   = to_big_endian(tif_files[z_index].data[xy_index]);
				file.write(reinterpret_cast<const char *>(&value), sizeof(uint16_t));
			}
		}
	}

	file.close();
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