#include "model/position.h"
#include <sstream>

std::string
Position1d::to_string(bool compress) const
{
	std::stringstream ss;

	if (compress)
	{
		ss << "Position1d";
		ss << "{";
		ss << "x: " << x << ", ";
		ss << "};";
	}
	else
	{
		ss << "Position1d" << std::endl;
		ss << "{" << std::endl;
		ss << "\t" << "x: " << x << std::endl;
		ss << "};";
	}

	return ss.str();
}

std::string
Position2d::to_string(bool compress) const
{
	std::stringstream ss;

	if (compress)
	{
		ss << "Position2d";
		ss << "{";
		ss << "y: " << y << ", ";
		ss << "x: " << x << ", ";
		ss << "};";
	}
	else
	{
		ss << "Position2d" << std::endl;
		ss << "{" << std::endl;
		ss << "\t" << "y: " << y << std::endl;
		ss << "\t" << "x: " << x << std::endl;
		ss << "};";
	}
	return ss.str();
}

std::string
Position3d::to_string(bool compress) const
{
	std::stringstream ss;
	if (compress)
	{
		ss << "Position3d";
		ss << "{";
		ss << "z: " << z << ", ";
		ss << "y: " << y << ", ";
		ss << "x: " << x << ", ";
		ss << "};";
	}
	else
	{
		ss << "Position3d" << std::endl;
		ss << "{" << std::endl;
		ss << "\t" << "z: " << z << std::endl;
		ss << "\t" << "y: " << y << std::endl;
		ss << "\t" << "x: " << x << std::endl;
		ss << "};";
	}
	return ss.str();
}