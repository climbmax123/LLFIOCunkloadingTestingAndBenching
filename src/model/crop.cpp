#include "model/crop.h"
#include <sstream>

// Function to replace all occurrences of 'toReplace' with 'replacement' in 'str'
void replaceAll(std::string& str, const std::string& toReplace, const std::string& replacement) {
	size_t position = str.find(toReplace);
	while (position != std::string::npos) {
		str.replace(position, toReplace.length(), replacement);
		position = str.find(toReplace, position + replacement.length());
	}
}

std::string
Crop1d::to_string() const
{
	std::stringstream ss;
	ss << "Crop1d" << std::endl;
	ss << "{" << std::endl;
	std::string position_string = position.to_string();
	replaceAll(position_string, std::string("\n"), std::string("\n\t"));
	ss << "\t" << "position: " << position_string << std::endl;

	std::string size_string = size.to_string();
	replaceAll(size_string, std::string("\n"), std::string("\n\t"));
	ss << "\t" << "size: " << size_string << std::endl;
	ss << "};";
	return ss.str();
}

std::string
Crop2d::to_string() const
{
	std::stringstream ss;
	ss << "Crop2d" << std::endl;
	ss << "{" << std::endl;
	std::string position_string = position.to_string();
	replaceAll(position_string, std::string("\n"), std::string("\n\t"));
	ss << "\t" << "position: " << position_string << std::endl;

	std::string size_string = size.to_string();
	replaceAll(size_string, std::string("\n"), std::string("\n\t"));
	ss << "\t" << "size: " << size_string << std::endl;
	ss << "};";
	return ss.str();
}

std::string
Crop3d::to_string() const
{
	std::stringstream ss;
	ss << "Crop3d" << std::endl;
	ss << "{" << std::endl;
	std::string position_string = position.to_string();
	replaceAll(position_string, std::string("\n"), std::string("\n\t"));
	ss << "\t" << "position: " << position_string << std::endl;

	std::string size_string = size.to_string();
	replaceAll(size_string, std::string("\n"), std::string("\n\t"));
	ss << "\t" << "size: " << size_string << std::endl;
	ss << "};";
	return ss.str();
}
