#ifndef TWOSOME_ASSET_H
#define TWOSOME_ASSET_H



#define sound_start_silent_samples_pad 4
#define sound_end_silent_samples_pad sound_start_silent_samples_pad
#define sound_total_silent_samples_pad (sound_start_silent_samples_pad + sound_end_silent_samples_pad)

enum Asset_Type
{
    asset_type_null = 0,
    asset_type_sound,
};

enum Asset_Type_Id
{
    asset_none = 0,
    
    //
    // NOTE: Sounds
    //
    asset_connected_small,
    asset_connected_medium,
    asset_connected_high,
    asset_spinner_laser,
    asset_life_lost,
    asset_boys_song,
    asset_boys_song_whistle,
    asset_player_colour_change,
    asset_level_stage_transition,
    asset_level_score_beaten,
    asset_end_of_level_score_increase,
    asset_spikes_barrier_spawn,
    asset_spikes_barrier_safe_hit,
    asset_spikes_barrier_haze,
    asset_shield_collectable,
    asset_expanding_shield,
    asset_shield_deployed,
    asset_hub_firework_unexplode,
    asset_hub_firework_explodes_v1,
    asset_hub_firework_explodes_v2,
    asset_girl_fades_in,
    asset_level_select_hover,
    asset_boy_about_to_speak,
    asset_boy_sighs,
    asset_hub_ambience,
    asset_boy_yawn,
    asset_boy_clears_throat,
    asset_girl_saying_credits,

    //
    // NOTE: Fonts
    //
    asset_DEBUG_font,
    
    asset_count
};

struct Loaded_Sound
{
    int16 *data;
    Memory_Index data_size;
    
    u32 total_samples;
    s32 channels;
    u32 loop_point_samples;
    u32 samples_per_sec;
    
    s32 variation;
};

struct Asset
{
    uint32 id;
    u32 index;

    union
    {
        Loaded_Sound sound;
#if TWOSOME_INTERNAL
        DEBUG_Loaded_Font DEBUG_font;
#endif
    };

    Asset *next;
};

#pragma pack(push, 1)

struct Asset_Sound_Header
{
    u32 variations_count;
    u32 variations_start_index;
};

struct Asset_Font_Header
{
    Memory_Index data_offset;
    Memory_Index data_size;
};

struct Asset_Header
{
    b32 filled;
    int type;
    union
    {
        Asset_Sound_Header sound;
        Asset_Font_Header font;
    };
};

struct Asset_Sound_Variation_Header
{
    Memory_Index data_offset;

    Loaded_Sound sound;
};

struct Asset_File_Header
{
    Memory_Index asset_headers_size;
    Memory_Index sound_variations_size;
    Memory_Index data_size;
};

#pragma pack(pop)

enum Game_Assets_Load_Result
{
    game_assets_load_result_null    = 0,
    game_assets_load_result_success = 1,
    game_assets_load_result_failed  = 2
};

struct Game_Assets
{
    Memory_Arena perm_arena;

    Asset *first_loaded_asset;

    Asset_File_Header file_header;
    Asset_Header *asset_headers;
    Asset_Sound_Variation_Header *sound_variations;

    Memory_Index data_start_offset;
    u8 *asset_data;

    u32 volatile asset_load_result;
};

#endif
