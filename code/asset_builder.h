#ifndef ASSET_BUILDER_H
#define ASSET_BUILDER_H


#include "twosome_platform.h"
#include "twosome_shared.h"
#include "twosome_render_group.h"
#include "twosome_asset.h"

struct ASSET_Info
{
    char *filenames[8];
    union
    {
        struct
        {
            f32 loop_point_secs[8];
        };
    };
    
    s32 count;
};

struct Asset_Setup
{
    Asset_Header headers[asset_count];
    ASSET_Info infos[asset_count];
    Memory_Index header_size;
    u32 total_sound_variations;

    int asset_id;
    int asset_type;
};

#endif
