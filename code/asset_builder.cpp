#include "asset_builder.h"
#include <stdio.h>


#pragma pack(push, 1)
struct WAVE_Header
{
    u32 riff_id;
    u32 size;
    u32 wave_id;
};

#define RIFF_CODE(a, b, c, d) (((uint32)(a) << 0) | ((uint32)(b) << 8) | ((uint32)(c) << 16) | ((uint32)(d) << 24))
enum
{
    WAVE_CHUNKID_FMT = RIFF_CODE('f', 'm', 't', ' '),
    WAVE_CHUNKID_DATA = RIFF_CODE('d', 'a', 't', 'a'),
    WAVE_CHUNKID_RIFF = RIFF_CODE('R', 'I', 'F', 'F'),
    WAVE_CHUNKID_WAVE = RIFF_CODE('W', 'A', 'V', 'E')
};

struct WAVE_Chunk
{
    u32 id;
    u32 size;
};

struct WAVE_Fmt
{
    u16 wFormatTag;
    u16 nChannels;
    u32 nSamplesPerSec;
    u32 nAvgBytesPerSec;
    u16 nBlockAlign;
    u16 wBitsPerSample;
    u16 cbSize;
    u16 wValidBitsPerSample;
    u32 dwChannelmask;
    u8 SubFormat[16];
};
#pragma pack(pop)

struct Riff_Iterator
{
    u8 *at;
    u8 *stop;
};

internal Riff_Iterator parse_chunk_at(void *at, void *stop)
{
    Riff_Iterator iter;

    iter.at = (u8 *)at;
    iter.stop = (u8 *)stop;

    return iter;
}

internal b32 is_valid(Riff_Iterator iter)
{
    bool32 result = (iter.at < iter.stop);
    return result;
}

internal Riff_Iterator next_chunk(Riff_Iterator iter)
{
    WAVE_Chunk *chunk = (WAVE_Chunk *)iter.at;
    u32 size = (chunk->size + 1) & ~1;
    iter.at += sizeof(WAVE_Chunk) + size;

    return iter;
}

internal u32 get_type(Riff_Iterator iter)
{
    WAVE_Chunk *chunk = (WAVE_Chunk *)iter.at;
    
    uint32 result = chunk->id;
    return result;
}

internal void *get_chunk_data(Riff_Iterator iter)
{
    void *result = (iter.at + sizeof(WAVE_Chunk));
    return result;
}

internal Loaded_Sound load_sound(char *filename, f32 loop_point_secs = 0.0f)
{
    Loaded_Sound result = {};

    char *filepath = filename;

    uint32 sample_data_size = 0;
    int16 *sample_data = 0;

    FILE *fp = fopen(filename, "rb");
    if(fp)
    {
        fseek(fp, 0, SEEK_END);
        long file_size = ftell(fp);
        fseek(fp, 0, SEEK_SET);
        void *file_data = malloc(file_size);
        fread(file_data, file_size, 1, fp);
        WAVE_Header *header = (WAVE_Header *)file_data;

        assert(header->riff_id == WAVE_CHUNKID_RIFF);
        assert(header->wave_id == WAVE_CHUNKID_WAVE);

        for(Riff_Iterator iter = parse_chunk_at(header + 1, (u8 *)(header + 1) + header->size - 4); is_valid(iter); iter = next_chunk(iter))
        {
            switch(get_type(iter))
            {
                case WAVE_CHUNKID_FMT:
                {
                    WAVE_Fmt *fmt = (WAVE_Fmt *)get_chunk_data(iter);
                    // NOTE: Only support PCM
                    assert(fmt->wFormatTag == 1);
                    assert(fmt->nSamplesPerSec == 44100);
                    assert(fmt->wBitsPerSample == 16);
                    assert(fmt->nBlockAlign == ((sizeof(int16) * fmt->nChannels)));
                    assert(fmt->nChannels);

                    result.channels = fmt->nChannels;
                    result.samples_per_sec = fmt->nSamplesPerSec;
                } break;

                case WAVE_CHUNKID_DATA:
                {
                    WAVE_Chunk *chunk = (WAVE_Chunk *)iter.at;

                    sample_data = (int16 *)get_chunk_data(iter);
                    sample_data_size = chunk->size;
                } break;
            };
        }

        assert(sample_data);

        //

        result.total_samples = sample_data_size / (result.channels * sizeof(int16));        
        result.loop_point_samples = (u32)((loop_point_secs * (f32)result.samples_per_sec));

        // NOTE: We always pad samples at start and end with zeros in them for SIMD chunks
        result.data_size = sample_data_size + ((sizeof(s16) * result.channels) * sound_total_silent_samples_pad);
        result.data = (s16 *)malloc(result.data_size);

        s16 *dest = result.data;

        // NOTE: Silent samples padding at start
        for(u32 silent_sample_index = 0; silent_sample_index < sound_start_silent_samples_pad; ++silent_sample_index)
        {
            *dest++ = 0;
            *dest++ = 0;
        }
        
        for(uint32 sample_index = 0; sample_index < result.total_samples; ++sample_index)
        {
            uint32 channel_1 = sample_index*result.channels;
            uint32 channel_2 = sample_index*result.channels + (result.channels - 1);
            
            int16 sample1 = sample_data[channel_1];
            int16 sample2 = sample_data[channel_2];

            *dest++ = sample1;
            *dest++ = sample2;            
        }
        
        // NOTE: Silent samples padding at end
        for(uint32 sample_index = 0; sample_index < sound_end_silent_samples_pad; ++sample_index)
        {
            *dest++ = 0;
            *dest++ = 0;
        }

        assert( ((dest - result.data) / result.channels) == (s32)(result.total_samples + sound_total_silent_samples_pad) );
    }    

    return result;
}

internal void begin_asset_type(Asset_Setup *setup, int asset_type)
{
    setup->asset_type = asset_type;
}

internal void begin_asset(Asset_Setup *setup, int asset_id)
{
    Asset_Header *asset_header = &setup->headers[asset_id];
    assert(!asset_header->filled);
    asset_header->filled = true;
    asset_header->type = setup->asset_type;

    setup->asset_id = asset_id;
}

internal void add_asset(Asset_Setup *setup, char *filename, f32 loop_point_secs = 0.0f)
{
    printf("adding asset '%s'\n", filename);
    
    Asset_Header *asset_header = &setup->headers[setup->asset_id];
    ASSET_Info *asset_info = &setup->infos[setup->asset_id];

    asset_info->filenames[asset_info->count] = filename;
    asset_info->loop_point_secs[asset_info->count] = loop_point_secs;
    ++asset_info->count;
    
    switch(asset_header->type)
    {
        case asset_type_sound:
        {
            Asset_Sound_Header *sound_header = &asset_header->sound;
            if(sound_header->variations_count == 0)
            {
                sound_header->variations_start_index = setup->total_sound_variations;
            }
            
            ++sound_header->variations_count;
            ++setup->total_sound_variations;
            setup->header_size += sizeof(Asset_Sound_Header);
        } break;
    };    
}

internal void add_single_asset(Asset_Setup *setup, int asset_id, char *filename, f32 loop_point_secs = 0.0f)
{
    begin_asset(setup, asset_id);
    add_asset(setup, filename, loop_point_secs);
}

internal void build_assets(void)
{
    printf("building assets..\n");
    
    FILE *file = fopen(asset_filename, "wb");
    if(file)
    {
        printf("created asset file..\n");
        
        Asset_File_Header file_header = {};
        Asset_Setup setup = { 0 };
        setup.header_size = sizeof(setup.headers);

        // NOTE: Sounds
        begin_asset_type(&setup, asset_type_sound);
        {
            begin_asset(&setup, asset_connected_small);
            add_asset(&setup, "connected_small_01.wav");
            add_asset(&setup, "connected_small_02.wav");
            add_asset(&setup, "connected_small_03.wav");
            add_asset(&setup, "connected_small_04.wav");

            begin_asset(&setup, asset_connected_medium);
            add_asset(&setup, "connected_medium_01.wav");
            add_asset(&setup, "connected_medium_02.wav");
            add_asset(&setup, "connected_medium_03.wav");
            add_asset(&setup, "connected_medium_04.wav");

            begin_asset(&setup, asset_connected_high);
            add_asset(&setup, "connected_high_01.wav");
            add_asset(&setup, "connected_high_02.wav");
            add_asset(&setup, "connected_high_03.wav");
            add_asset(&setup, "connected_high_04.wav");

            begin_asset(&setup, asset_spinner_laser);
            add_asset(&setup, "spinner_laser.wav");

            begin_asset(&setup, asset_life_lost);
            add_asset(&setup, "life_lost.wav");           

            add_single_asset(&setup, asset_boys_song, "boys_song.wav");
            add_single_asset(&setup, asset_boys_song_whistle, "boys_song_whistle.wav");

            begin_asset(&setup, asset_player_colour_change);
            add_asset(&setup, "player_colour_change_01.wav");
            add_asset(&setup, "player_colour_change_02.wav");

            begin_asset(&setup, asset_level_stage_transition);
            add_asset(&setup, "level_stage_transition_01.wav");
            add_asset(&setup, "level_stage_transition_02.wav");
            add_asset(&setup, "level_stage_transition_03.wav");
            add_asset(&setup, "level_stage_transition_04.wav");

            add_single_asset(&setup, asset_level_score_beaten, "level_score_beaten.wav");

            add_single_asset(&setup, asset_end_of_level_score_increase, "end_of_level_score_increase.wav");

            add_single_asset(&setup, asset_spikes_barrier_spawn, "scrolling_spikes_spawn.wav");

            begin_asset(&setup, asset_spikes_barrier_safe_hit);
            add_asset(&setup, "scrolling_spikes_safe_hit_01.wav");
            add_asset(&setup, "scrolling_spikes_safe_hit_02.wav");
            add_asset(&setup, "scrolling_spikes_safe_hit_03.wav");

            add_single_asset(&setup, asset_spikes_barrier_haze, "scrolling_spikes_haze.wav");

            begin_asset(&setup, asset_expanding_shield);
            add_asset(&setup, "expanding_shield_01.wav");
            add_asset(&setup, "expanding_shield_02.wav");

            add_single_asset(&setup, asset_shield_collectable, "shield_collectable.wav");

            add_single_asset(&setup, asset_shield_deployed, "shield_deployed.wav");

            begin_asset(&setup, asset_hub_firework_unexplode);
            add_asset(&setup, "hub_firework_unexplode_01.wav");
            add_asset(&setup, "hub_firework_unexplode_02.wav");
            add_asset(&setup, "hub_firework_unexplode_03.wav");
            add_asset(&setup, "hub_firework_unexplode_04.wav");

            begin_asset(&setup, asset_hub_firework_explodes_v1);
            add_asset(&setup, "hub_firework_explodes_v1_01.wav");
            add_asset(&setup, "hub_firework_explodes_v1_02.wav");
            add_asset(&setup, "hub_firework_explodes_v1_03.wav");
            add_asset(&setup, "hub_firework_explodes_v1_04.wav");

            begin_asset(&setup, asset_hub_firework_explodes_v2);
            add_asset(&setup, "hub_firework_explodes_v2_01.wav");
            add_asset(&setup, "hub_firework_explodes_v2_02.wav");
            add_asset(&setup, "hub_firework_explodes_v2_03.wav");
            add_asset(&setup, "hub_firework_explodes_v2_04.wav");

            add_single_asset(&setup, asset_girl_fades_in, "girl_fades_in.wav");
            
            add_single_asset(&setup, asset_level_select_hover, "level_select_hover.wav");

            add_single_asset(&setup, asset_boy_about_to_speak, "boy_about_to_speak.wav");

            add_single_asset(&setup, asset_boy_sighs, "boy_sighs.wav");

            add_single_asset(&setup, asset_hub_ambience, "hub_ambience.wav", 27.0f);           

            add_single_asset(&setup, asset_boy_yawn, "boy_yawn.wav");

            add_single_asset(&setup, asset_boy_clears_throat, "boy_clears_throat.wav");
            
            add_single_asset(&setup, asset_girl_saying_credits, "girl_saying_credits.wav");
        }
        
        file_header.asset_headers_size = sizeof(setup.headers);
        file_header.sound_variations_size = sizeof(Asset_Sound_Variation_Header) * setup.total_sound_variations;;
        
        Asset_Sound_Variation_Header *variations = (Asset_Sound_Variation_Header *)malloc(file_header.sound_variations_size);

        fseek(file, safe_truncate_uint64(sizeof(file_header) + file_header.asset_headers_size + file_header.sound_variations_size), SEEK_SET);

        long file_data_start = ftell(file);
        for(int header_index = 0; header_index < array_count(setup.headers); ++header_index)
        {
            Asset_Header *asset_header = &setup.headers[header_index];
            ASSET_Info *asset_info = &setup.infos[header_index];
            if(asset_header->filled)
            {
                char filepath[256] = { 0 };
                sprintf(filepath, DEBUG_path_from_build_path_to_src_data);                
                char *append_filename_pt = filepath + strlen(filepath);
                
                switch(asset_header->type)
                {
                    case asset_type_sound:
                    {
                        Asset_Sound_Header *sound_header = &asset_header->sound;
                        Asset_Sound_Variation_Header *variation = variations + sound_header->variations_start_index;
                        
                        for(u32 variation_index = 0; variation_index < sound_header->variations_count; ++variation_index)
                        {
                            char *filename = asset_info->filenames[variation_index];
                            sprintf(append_filename_pt, "%s", filename);

                            variation->sound = load_sound(filepath, asset_info->loop_point_secs[variation_index]);
                            variation->sound.variation = variation_index;
                            variation->data_offset = ftell(file);
                            
                            assert(variation->sound.loop_point_samples < variation->sound.total_samples);

                            fwrite(variation->sound.data, variation->sound.data_size, 1, file);

                            ++variation;
                        }
                    } break;
                    
                    invalid_default_case;
                };   
            }
        }

        file_header.data_size = (ftell(file) - file_data_start);
        
        fseek(file, 0, SEEK_SET);
        fwrite(&file_header, sizeof(file_header), 1, file);
        fwrite(setup.headers, file_header.asset_headers_size, 1, file);
        fwrite(variations, file_header.sound_variations_size, 1, file);

        fclose(file);

        printf("finished writing asset file..\n");
    }
}

int main(int argc, char **args)
{
    build_assets();
}
