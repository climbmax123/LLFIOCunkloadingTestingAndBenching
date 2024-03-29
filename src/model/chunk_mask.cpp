#include "model/chunk_mask.h"

#include <span>
#include <opencv2/core/mat.hpp>
#include <opencv2/imgcodecs.hpp>
#include "model/mask.h"
#include "model/crop.h"


static
std::vector<fs::path>
get_all_mask_files(VolumeInformation const& volume_info, fs::path const& mask_volume_directory);

static
std::vector<Mask>
open_mask_files(std::span<fs::path> const& mask_file_paths);

static
Mask
open_mask_file(fs::path const& path);

static
std::vector<bool>
write_mask_crop_to_vector(std::vector<bool>& mask_chunk, Crop2d crop, std::vector<Mask> const& masks);

static
bool
are_all_mask_pixels_false(std::vector<bool> const& mask_pixels);

ChunkMask::ChunkMask(VolumeInformation const& volume_information, fs::path const& mask_path, uint64_t chunk_size)
{
	size = {
		.width  = volume_information.width / chunk_size,
		.height = volume_information.height / chunk_size,
		.depth  = volume_information.slices / chunk_size
	};

	data = std::vector<bool>(size.volume());

	fill_chunk_mask(volume_information, mask_path, chunk_size);
}

void
ChunkMask::fill_chunk_mask(const VolumeInformation& volume_information, const fs::path& mask_path, uint64_t chunk_size)
{
	std::vector<fs::path> mask_file_paths = get_all_mask_files(volume_information, mask_path);
	std::ranges::sort(mask_file_paths);
	std::cout << "found " << mask_file_paths.size() << " mask files" << std::endl;

	std::vector<bool> pixel_mask(chunk_size * chunk_size * chunk_size);

	for (uint64_t z = 0; z < size.depth; z++)
	{
		uint64_t index_z = z * chunk_size;

		std::span<fs::path> mask_file_path_span = {mask_file_paths.begin() + index_z, chunk_size};

		std::vector<Mask> masks = open_mask_files(mask_file_path_span);

		for (uint64_t y = 0; y < size.height; y++)
		{
			uint64_t index_y = y * chunk_size;

			for (uint64_t x = 0; x < size.width; x++)
			{
				uint64_t index_x = x * chunk_size;
				Crop2d   crop    = {.position={index_x, index_y}, .size={chunk_size, chunk_size}};

				pixel_mask = write_mask_crop_to_vector(pixel_mask, crop, masks);

				set_pixel_at(z, y, x, are_all_mask_pixels_false(pixel_mask));
			}
		}
		std::cout << "created chunk mask from layer " << index_z << " to layer " << (index_z + chunk_size)
		          << std::endl;
	}
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
		for (uint64_t x = 0; x < width; x++, index++)
		{ mask.data[index] = 0 < image.at<uint16_t>(y, x); }
	}

	return mask;
}

std::vector<bool>
write_mask_crop_to_vector(std::vector<bool>& mask_chunk, Crop2d crop, std::vector<Mask> const& masks)
{
	for (uint64_t z = 0; z < masks.size(); z++)
	{
		for (uint64_t index_y = crop.position.y, y = 0; index_y < crop.size.height; index_y++, y++)
		{
			for (uint64_t index_x = crop.position.x, x = 0; index_x < crop.size.width; index_x++, x++)
			{
				mask_chunk[z * crop.size.area() + y * crop.size.width + x]
					= masks[z].data[index_y * masks[z].width + index_x];
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

[[nodiscard]]
bool
ChunkMask::at(uint64_t z, uint64_t y, uint64_t x) const
{ return data[z * size.height * size.width + y * size.width + x]; }

void
ChunkMask::set_pixel_at(uint64_t z, uint64_t y, uint64_t x, bool value)
{ data[z * size.height * size.width + y * size.width + x] = value; }
