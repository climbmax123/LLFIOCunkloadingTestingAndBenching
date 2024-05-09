#include <opencv2/imgcodecs.hpp>
#include "model/tif_file.h"

namespace fs = std::filesystem;

TifFile
TifFile::read_from_image_file(fs::path const& path)
{
	cv::Mat image = cv::imread(path, cv::IMREAD_UNCHANGED);

	uint64_t height = image.size[0];
	uint64_t width  = image.size[1];

	TifFile tif_file = {{width, height}, std::vector<uint16_t>(width * height)};

	for (int64_t y = 0; y < height; y++)
	{
		for (int64_t x = 0; x < width; x++)
		{ tif_file.at({y, x}) = image.at<uint16_t>(y, x); }
	}

	return tif_file;
}

uint16_t const&
TifFile::at(Position2d position) const
{
	return data[position.y * size.width + position.x];
}

uint16_t&
TifFile::at(Position2d position)
{
	return data[position.y * size.width + position.x];
}
