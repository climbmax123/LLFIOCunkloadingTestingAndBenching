#include "model/size.h"
#include <sstream>

std::string
Size1d::to_string() const {
	std::stringstream ss;
	ss << "Size1d" << std::endl;
	ss << "{" << std::endl;
	ss << "\t" << "length: " << length << std::endl;
	ss << "};";
	return ss.str();
}

uint64_t
Size2d::area() const
{
	return height * width;
}

std::string
Size2d::to_string() const {
	std::stringstream ss;
	ss << "Size2d" << std::endl;
	ss << "{" << std::endl;
	ss << "\t" << "height: " << height << std::endl;
	ss << "\t" << "width:  " << width << std::endl;
	ss << "};";
	return ss.str();
}

uint64_t
Size3d::volume() const
{
	return depth * height * width;
}

uint64_t
Size3d::area_top() const
{
	return height * width;
}

uint64_t
Size3d::area_side() const
{
	return depth * height;
}

uint64_t
Size3d::area_front() const
{
	return depth * width;
}

std::string
Size3d::to_string() const
{
	std::stringstream ss;
	ss << "Size3d" << std::endl;
	ss << "{" << std::endl;
	ss << "\t" << "depth:  " << depth << std::endl;
	ss << "\t" << "height: " << height << std::endl;
	ss << "\t" << "width:  " << width << std::endl;
	ss << "};";
	return ss.str();
}