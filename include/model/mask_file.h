#ifndef VWTCHUNKLOADINGTESTINGANDBENCHING_MODEL_MASKFILE_H
#define VWTCHUNKLOADINGTESTINGANDBENCHING_MODEL_MASKFILE_H

#include <vector>
#include <filesystem>
#include "position.h"
#include "size.h"

struct MaskFile
{
	Size2d size;

	std::vector<bool> data;

	[[nodiscard]] static MaskFile
	read_from_image_file(std::filesystem::path const& path);

	[[nodiscard]] bool at(Position2d position) const;

	void set(Position2d position, bool value);
};

#endif //VWTCHUNKLOADINGTESTINGANDBENCHING_MODEL_MASKFILE_H
