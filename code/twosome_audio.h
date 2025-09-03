#ifndef TWOSOME_AUDIO_H
#define TWOSOME_AUDIO_H


enum Playing_Sound_Flags
{
    playing_sound_flag_reverse                       = 1 << 0,
    playing_sound_flag_loop                          = 1 << 1,
    // NOTE: Stops someone from outside audio system fading out sound (for stage transitions or whatever)
    playing_sound_flag_no_fade                       = 1 << 2,
    playing_sound_flag_no_pause_capture              = 1 << 3,
    // NOTE: This is used by game level to exclude some sounds from fading of audio sounds
    playing_sound_flag_game_level_stage_no_fade      = 1 << 4,
};

struct Playing_Sound
{
    Loaded_Sound *loaded_sound;
        
    u32 playing_flags;
    
    b32 playing;
    b32 paused;

    f32 sample;
    
    f32 speed;
    
    u32 id;

    b32 _owned_by_audio_system;

    f32 volume;    
    f32 _mix_volume;
    
    f32 target_volume;
    f32 delta_current_volume;

    f32 target_cross_fade_volume;
    f32 delta_cross_fade_volume;

    f32 _cross_fade_volume;
    
    Playing_Sound *next;
};

struct Overlapping_Sound
{
    // NOTE: The primary sound is always behind the secondary one
    Playing_Sound *primary;
    Playing_Sound *secondary;

    b32 cross_fade;
    
    Overlapping_Sound *next;
};

enum Captured_Sound_Type
{
    captured_sound_type_sound,
    captured_sound_type_overlapping
};

struct Captured_Sound
{
    s32 type;
    union
    {
        Playing_Sound *sound;
        Overlapping_Sound *overlapping_sound;
    };
    
    Captured_Sound *next;
};

struct Captured_Sounds_Arena
{
    Captured_Sound *first_captured_sound;
    Captured_Sound *first_free_captured_sound;

    Memory_Arena *perm_arena;
    struct Game_Audio *audio;
};

struct Game_Audio
{
    Memory_Arena *perm_arena;
    Game_Assets *game_assets;

    f32 master_volume;
    
    Playing_Sound *first_playing_sound;
    Playing_Sound *first_free_playing_sound;

    Overlapping_Sound *first_playing_overlapping_sound;
    Overlapping_Sound *first_free_overlapping_sound;

    Captured_Sounds_Arena pause_captured_sounds;

    f32 sound_mix[asset_count];

    RNG *rng;
};

struct Play_Sound_Result
{
    u32 id;
    b32 backwards;
    int variation;
    f32 volume;
};

#endif
