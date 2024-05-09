#include <opencv2/imgcodecs.hpp>
#include "model/mask_file.h"

namespace fs = std::filesystem;

MaskFile
MaskFile::read_from_image_file(fs::path const& path)
{
	cv::Mat image = cv::imread(path, cv::IMREAD_UNCHANGED);

	uint64_t height = image.size[0];
	uint64_t width  = image.size[1];

	MaskFile mask_file = {{width, height}, std::vector<bool>(width * height)};

	for (int64_t y = 0; y < height; y++)
	{
		for (int64_t x = 0; x < width; x++)
		{ mask_file.set({y, x}, image.at<uint8_t>(y, x)); }
	}

	return mask_file;
}

bool
MaskFile::at(Position2d position) const
{
	return data[position.y * size.width + position.x];
}

void
MaskFile::set(Position2d position, bool value)
{
	data[position.y * size.width + position.x] = value;
}
