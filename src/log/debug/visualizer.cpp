#include "log/debug/visualizer.h"

#include <cassert>
#include "open3d/Open3D.h"

void
visualize_2d_vector_as_image(const std::vector<uint16_t>& array, Size2d size)
{
	assert(array.size() == size.area());

	// Create an Open3D image
	open3d::geometry::Image img;
	img.Prepare(size.width, size.height, 1, 1); // 1 channel, 2 bytes per channel

	for (int y = 0; y < size.height; ++y)
	{
		for (int x = 0; x < size.width; ++x)
		{
			int idx = x * size.height + y;
			img.data_[idx] = (array[idx] / 256);
		}
	}

	// Display the image using Open3D
	open3d::visualization::DrawGeometries({std::make_shared<open3d::geometry::Image>(img)}, "Image", 800, 600);
}

void
visualize_2d_vector_as_image(const std::vector<bool>& array, Size2d size)
{
	assert(array.size() == size.area());

	// Create an Open3D image
	open3d::geometry::Image img;
	img.Prepare(size.width, size.height, 1, 1); // 1 channel, 2 bytes per channel

	for (int y = 0; y < size.height; ++y)
	{
		for (int x = 0; x < size.width; ++x)
		{
			int idx = y * size.width + x;
			img.data_[idx] = array[idx] ? 255 : 0;
		}
	}

	// Display the image using Open3D
	open3d::visualization::DrawGeometries({std::make_shared<open3d::geometry::Image>(img)}, "Image", 800, 600);
}

void
visualize_2d_show_crop_in_vector_as_image(const std::vector<bool>& array, Size2d size, Crop2d crop)
{
	assert(array.size() == size.area());

	// Create an Open3D image
	open3d::geometry::Image img;
	img.Prepare(size.width, size.height, 1, 1); // 1 channel, 2 bytes per channel

	for (int y = 0; y < size.height; ++y)
	{
		for (int x = 0; x < size.width; ++x)
		{
			int idx = y * size.width + x;
			img.data_[idx] = array[idx] ? 255 : 0;
		}
	}

	for (int y = crop.position.y; y < crop.position.y + crop.size.height; y++) {
		for (int x = crop.position.x; x < crop.position.x + crop.size.width; x++) {
			int index = y * size.width + x;
			img.data_[index] = 128;
		}
	}

	// Display the image using Open3D
	open3d::visualization::DrawGeometries({std::make_shared<open3d::geometry::Image>(img)}, "Image", 800, 600);
}

void
visualize_3d_vector_as_point_cloud(const std::vector<uint16_t>& array, Size3d size, uint16_t threshold)
{
	assert(array.size() == size.volume());

	// Create a point cloud and corresponding colors and sizes.
	open3d::geometry::PointCloud point_cloud;

	// Define the color map: you can customize this gradient.
	std::vector<Eigen::Vector3d> color_map;
	// Here we add two colors for the gradient: blue for the lowest value, green for the highest.
	color_map.emplace_back(0, 0, 1); // Blue
	color_map.emplace_back(0, 1, 0); // Green

	auto cloud = std::make_shared<open3d::geometry::PointCloud>();

	// Process data and create spheres.
	for (int z = 0; z < size.depth; ++z)
	{
		for (int y = 0; y < size.height; ++y)
		{
			for (int x = 0; x < size.width; ++x)
			{
				uint64_t index = z * size.width * size.height + y * size.width + x;
				uint16_t value = array[index];

				if (value < threshold)
				{ continue; }

				double normalized_value = static_cast<double>(value) / 65535.0;

				// Interpolate color based on value.
				Eigen::Vector3d color = color_map[0] * (1.0 - normalized_value)
				                        + color_map[1] * normalized_value;

				cloud->points_.emplace_back(x, y, z);
				cloud->colors_.emplace_back(color);
			}
		}
	}

	// Visualize the point cloud.
	open3d::visualization::DrawGeometries({cloud}, "3D Data Visualization", 1600, 900);
}

void
visualize_3d_vector_as_point_cloud(const std::vector<bool>& array, Size3d size)
{
	assert(array.size() == size.volume());

	// Create a point cloud and corresponding colors and sizes.
	open3d::geometry::PointCloud point_cloud;

	// Define the color map: you can customize this gradient.
	Eigen::Vector3d color;
	// Here we add two colors for the gradient: blue for the lowest value, green for the highest.
	color = {0, 0, 1}; // Blue

	auto cloud = std::make_shared<open3d::geometry::PointCloud>();

	// Process data and create spheres.
	for (int z = 0; z < size.depth; ++z)
	{
		for (int y = 0; y < size.height; ++y)
		{
			for (int x = 0; x < size.width; ++x)
			{
				uint64_t index = z * size.width * size.height + y * size.width + x;
				if (!array[index])
				{ continue; }

				cloud->points_.emplace_back(x, y, z);
				cloud->colors_.emplace_back(color);
			}
		}
	}

	// Visualize the point cloud.
	open3d::visualization::DrawGeometries({cloud}, "3D Data Visualization", 1600, 900);
}