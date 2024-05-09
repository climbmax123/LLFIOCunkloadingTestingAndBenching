#ifndef VWTCHUNKLOADINGTESTINGANDBENCHING_VISUALIZER_H
#define VWTCHUNKLOADINGTESTINGANDBENCHING_VISUALIZER_H

#include <vector>
#include "model/size.h"
#include "model/crop.h"

void
visualize_2d_vector_as_image(const std::vector<uint16_t>& array, Size2d size);

void
visualize_2d_vector_as_image(const std::vector<bool>& array, Size2d size);

void
visualize_2d_show_crop_in_vector_as_image(const std::vector<bool>& array, Size2d size, Crop2d crop);

void
visualize_3d_vector_as_point_cloud(const std::vector<uint16_t>& array, Size3d size, uint16_t threshold);

void
visualize_3d_vector_as_point_cloud(const std::vector<bool>& array, Size3d size);

#endif //VWTCHUNKLOADINGTESTINGANDBENCHING_VISUALIZER_H
