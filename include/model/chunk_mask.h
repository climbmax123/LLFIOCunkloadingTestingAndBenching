#ifndef VWTCHUNKLOADINGTESTINGANDBENCHING_MODEL_CHUNKMASK_H
#define VWTCHUNKLOADINGTESTINGANDBENCHING_MODEL_CHUNKMASK_H


#include <filesystem>
#include <algorithm>
#include <iostream>
#include "util/volume_information.h"
#include "model/size.h"

namespace fs = std::filesystem;

struct ChunkMask
{
private:
	void
	fill_chunk_mask(VolumeInformation const& volume_information, fs::path const& mask_path, uint64_t chunk_size);

public:
	std::vector<bool> data;

	Size3d size{};

	ChunkMask(VolumeInformation const& volume_information, fs::path const& mask_path, uint64_t chunk_size);

	[[nodiscard]] bool
	at(uint64_t z, uint64_t y, uint64_t x) const;

	void
	set_pixel_at(uint64_t z, uint64_t y, uint64_t x, bool set);
};


#endif //VWTCHUNKLOADINGTESTINGANDBENCHING_MODEL_CHUNKMASK_H
