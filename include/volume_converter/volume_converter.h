#ifndef VWTCHUNKLOADINGTESTINGANDBENCHING_VOLUMECONVERTER_VOLUMECONVERTER_H
#define VWTCHUNKLOADINGTESTINGANDBENCHING_VOLUMECONVERTER_VOLUMECONVERTER_H

#include <filesystem>
#include "model/chunk.h"
#include "model/mask_file.h"
#include "model/tif_file.h"
#include "model/crop.h"

void
convert_volume_to_compressed_chunked_volume(std::filesystem::path const& volume_path, std::filesystem::path const& mask_path, uint64_t chunk_size, std::filesystem::path const& compressed_and_chunked_volume_path);

void
convert_volume_to_chunk_files(std::filesystem::path const& volume_directory, std::filesystem::path const& chunked_volume_path, int64_t chunk_size);

#endif //VWTCHUNKLOADINGTESTINGANDBENCHING_VOLUMECONVERTER_VOLUMECONVERTER_H
