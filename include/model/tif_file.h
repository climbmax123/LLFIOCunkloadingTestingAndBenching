#ifndef VWTCHUNKLOADINGTESTINGANDBENCHING_MODEL_TIFFILE_H
#define VWTCHUNKLOADINGTESTINGANDBENCHING_MODEL_TIFFILE_H


#include <filesystem>
#include "size.h"
#include "position.h"

struct TifFile
{
	Size2d size;

	std::vector<uint16_t> data;

	[[nodiscard]] static TifFile
	read_from_image_file(std::filesystem::path const& path);

	[[nodiscard]] uint16_t& at(Position2d position);

	[[nodiscard]] uint16_t const& at(Position2d position) const;
};

#endif //VWTCHUNKLOADINGTESTINGANDBENCHING_MODEL_TIFFILE_H
