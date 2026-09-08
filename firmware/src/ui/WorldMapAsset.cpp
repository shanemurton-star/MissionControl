#include "WorldMapAsset.h"

namespace
{
#include "WorldMapAsset.inc"
}

namespace WorldMapAsset
{
    const lv_img_dsc_t IMAGE = {
        {LV_IMG_CF_RAW_ALPHA, 0, 0, WIDTH, HEIGHT},
        world_map_png_len,
        world_map_png
    };
}
