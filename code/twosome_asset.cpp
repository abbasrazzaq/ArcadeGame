#include "twosome_asset.h"


struct Load_Asset_Data_Work
{
    Task_With_Memory *task;
    Game_Assets *assets;
    Platform_File_Handle asset_file;
};

internal PLATFORM_WORK_QUEUE_CALLBACK(load_asset_data_work)
{
    Load_Asset_Data_Work *work = (Load_Asset_Data_Work *)data;

    Game_Assets *assets = work->assets;    

    b32 result = false;    

    if(work->asset_file.handle)
    {
        Platform_File_Handle *asset_file = &work->asset_file;
        
        // NOTE: Read in header
        {
            platform.read_data_from_file(asset_file, (u8*)&assets->file_header, 0, sizeof(assets->file_header), true);
    
            Memory_Index header_block_size = assets->file_header.asset_headers_size + assets->file_header.sound_variations_size;
            u8 *header_block = (u8 *)push_size(&assets->perm_arena, header_block_size);
            platform.read_data_from_file(asset_file, header_block, sizeof(assets->file_header), header_block_size, true);

            assets->asset_headers = (Asset_Header *)header_block;
            assets->sound_variations = (Asset_Sound_Variation_Header *)(header_block + assets->file_header.asset_headers_size);

            assets->data_start_offset = sizeof(assets->file_header) + header_block_size, assets->file_header.data_size;
        }

        //
        // NOTE: Read in payload
        //
        assets->asset_data = (u8 *)push_size(&assets->perm_arena, assets->file_header.data_size);    
        platform.read_data_from_file(asset_file, ((u8 *)assets->asset_data), assets->data_start_offset, assets->file_header.data_size, true);

        platform.close_file(asset_file);
        
        result = true;
    }
    
    complete_previous_writes_before_future_writes;

    assets->asset_load_result = result ? game_assets_load_result_success : game_assets_load_result_failed;
        
    end_task_with_memory(work->task);
}

internal void initialize_game_assets(Transient_State *tran_state)
{
    TIMED_BLOCK();
    
#if TWOSOME_INTERNAL
    {
        // NOTE: Want to check that none of the data files have a newer timestamp than the asset file, because if it does then it probably means the asset file is stale.    
        Platform_Date_Time latest_data_file_timestamp = platform.DEBUG_get_latest_file_write_in_directory(DEBUG_path_from_build_path_to_src_data);
      
        Platform_Date_Time asset_file_timestamp = platform.DEBUG_get_last_file_write_time(asset_filename);

        // NOTE: Check data files against asset file
        assert(compare_platform_date_times(asset_file_timestamp, latest_data_file_timestamp) != platform_date_time_compare_first_earlier);
    }
#endif
    
    // NOTE: Do loading of assets from asset file in separate thread
    Task_With_Memory *task = begin_task_with_memory(tran_state);
    assert(task);
    Load_Asset_Data_Work *work = push_struct(&task->arena, Load_Asset_Data_Work);
    work->task = task;
    work->assets = tran_state->assets;
    work->asset_file = platform.open_file(asset_filename, platform_directory_type_app, platform_file_access_mode_read);

    platform.add_entry(tran_state->work_queue, load_asset_data_work, work);
}

internal Asset_Header *get_asset_header(Game_Assets *assets, u32 asset_id)
{
    assert(asset_id < asset_count);
    
    Asset_Header *header = &assets->asset_headers[asset_id];    
    return header;
}

internal Asset *load_asset(Game_Assets *assets, u32 asset_id, u32 index = 0)
{
    Asset *result = 0;
    
    assert(asset_id);
    
    for(Asset *asset = assets->first_loaded_asset; asset; asset = asset->next)
    {
        if(asset->id == asset_id && asset->index == index)
        {
            result = asset;

#if TWOSOME_INTERNAL
            if(asset_id == asset_DEBUG_font && assets->asset_load_result && !result->DEBUG_font.font_file_buffer)
            {
                Debug_Read_File_Result read_result = platform.DEBUG_read_entire_file(DEBUG_font_path);
                assert(read_result.contents);
                u8 *asset_data = (u8 *)push_size(&assets->perm_arena, read_result.contents_size);
                memcpy(asset_data, read_result.contents, read_result.contents_size);

                platform.DEBUG_free_file_memory(read_result.contents);               
                result->DEBUG_font.font_file_buffer = (unsigned char *)asset_data;
                
                if(stbtt_InitFont(&result->DEBUG_font.stb_font, result->DEBUG_font.font_file_buffer, 0) == 0)
                {
                    log_error("Failed to init font.");
                }
            }
#endif
            
            break;
        }
    }

    if(!result)
    {
        Asset *next = assets->first_loaded_asset;

        assets->first_loaded_asset = push_struct(&assets->perm_arena, Asset);
        zero_object(Asset, *assets->first_loaded_asset);
        assets->first_loaded_asset->next = next;

        result = assets->first_loaded_asset;
        result->id = asset_id;
        result->index = index;

#if TWOSOME_INTERNAL
        if(asset_id == asset_DEBUG_font)
        {
#if 0
            Debug_Read_File_Result read_result = platform.DEBUG_read_entire_file(DEBUG_font_path);
            assert(read_result.contents);
            u8 *asset_data = (u8 *)push_size(&assets->perm_arena, read_result.contents_size);
            memcpy(asset_data, read_result.contents, read_result.contents_size);

            platform.DEBUG_free_file_memory(read_result.contents);               
            result->DEBUG_font.font_file_buffer = (unsigned char *)asset_data;
                
            if(stbtt_InitFont(&result->DEBUG_font.stb_font, result->DEBUG_font.font_file_buffer, 0) == 0)
            {
                log_error("Failed to init font.");
            }
#endif
        }
        else
#endif
        {
            Asset_Header *header = get_asset_header(assets, asset_id);        
            switch(header->type)
            {
                case asset_type_sound:
                {
                    // NOTE: Check we're not passing invalid sound variation in
                    assert(index < header->sound.variations_count);
                
                    Asset_Sound_Variation_Header *sound_header = assets->sound_variations + header->sound.variations_start_index + index;

                    u8 *asset_data = assets->asset_data + (sound_header->data_offset - assets->data_start_offset);
                    Loaded_Sound *loaded_sound = &result->sound;
                    *loaded_sound = sound_header->sound;
                    loaded_sound->data = (s16 *)asset_data;
                    assert( ( (loaded_sound->data_size / sizeof(s16)) / loaded_sound->channels ) == (loaded_sound->total_samples + sound_total_silent_samples_pad) );
                    loaded_sound->data += (sound_start_silent_samples_pad * loaded_sound->channels);
                
                } break;

                case asset_type_null:
                {
#if !TWOSOME_INTERNAL
                    if(asset_id != asset_DEBUG_font)
                    {
                        invalid_code_path;   
                    }                
#endif
                } break;

                invalid_default_case;            
            };   
        }
    }

    return result;
}

internal Asset *load_sound(Game_Assets *assets, Game_Audio *audio, uint32 sound_id, int force_variation_index = -1)
{
    int variation_index_to_use = force_variation_index;
    if(variation_index_to_use < 0)
    {
        // NOTE: Pick random variation
        Asset_Header *header = get_asset_header(assets, sound_id);
        variation_index_to_use = random_exclusive(audio->rng, 0, header->sound.variations_count);
    }
    
    Asset *asset = load_asset(assets, sound_id, variation_index_to_use);    
    return asset;
}

#if TWOSOME_INTERNAL
internal Asset *DEBUG_load_font(Game_Assets *assets, uint32 font_id)
{
    Asset *asset = load_asset(assets, font_id, asset_type_null);
    return asset;
}
#endif
