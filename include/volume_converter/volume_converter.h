#ifndef VWTCHUNKLOADINGTESTINGANDBENCHING_VOLUMECONVERTER_VOLUMECONVERTER_H
#define VWTCHUNKLOADINGTESTINGANDBENCHING_VOLUMECONVERTER_VOLUMECONVERTER_H

#include <filesystem>
#include "model/chunk.h"
#include "model/mask.h"
#include "model/tif_file.h"
#include "model/crop.h"

void
convert_volume_to_compressed_chunked_volume(std::filesystem::path volume_path, std::filesystem::path mask_path);

void
convert_volume_to_chunk_files(std::filesystem::path const& volume_directory, int64_t chunk_size);

#endif //VWTCHUNKLOADINGTESTINGANDBENCHING_VOLUMECONVERTER_VOLUMECONVERTER_H
