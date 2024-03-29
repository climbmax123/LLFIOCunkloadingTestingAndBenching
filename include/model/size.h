#ifndef VWTCHUNKLOADINGTESTINGANDBENCHING_SIZE_H
#define VWTCHUNKLOADINGTESTINGANDBENCHING_SIZE_H

#include <cstdint>
#include <string>
#include <sstream>

struct Size1d
{
	uint64_t width;

	[[nodiscard]] std::string to_string() const {
		std::stringstream ss;
		ss << "Size1d" << std::endl;
		ss << "{" << std::endl;
		ss << "\t" << "width:  " << width << std::endl;
		ss << "};";
	}
};

struct Size2d
{
	uint64_t width, height;

	[[nodiscard]] uint64_t
	area() const
	{
		return height * width;
	}

	[[nodiscard]] std::string to_string() const {
		std::stringstream ss;
		ss << "Size2d" << std::endl;
		ss << "{" << std::endl;
		ss << "\t" << "width:  " << width << std::endl;
		ss << "\t" << "height: " << height << std::endl;
		ss << "};";
	}
};

struct Size3d
{
	uint64_t width, height, depth;

	[[nodiscard]] uint64_t
	volume() const
	{
		return depth * height * width;
	}

	[[nodiscard]] std::string to_string() const {
		std::stringstream ss;
		ss << "Size3d" << std::endl;
		ss << "{" << std::endl;
		ss << "\t" << "width:  " << width << std::endl;
		ss << "\t" << "height: " << height << std::endl;
		ss << "\t" << "depth:  " << depth << std::endl;
		ss << "};";
		return ss.str();
	}
};

#endif //VWTCHUNKLOADINGTESTINGANDBENCHING_SIZE_H
