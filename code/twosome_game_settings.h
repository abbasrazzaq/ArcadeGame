#ifndef TWOSOME_GAME_SETTINGS_H
#define TWOSOME_GAME_SETTINGS_H


enum Serialization_Data_Type
{
    serialization_type_b32,
    serialization_type_f32
};

enum Serialization_Data_Flags
{
    serialization_data_flag_clamp01 = 1 << 0,
};

struct Serialization_Data
{
    char *name;
    s32 type;
    u32 flags;
    void *data;
};

struct Serialization_Meta
{
    Memory_Index data_offset;

    Serialization_Data serialization;
};

global_variable Serialization_Meta game_settings_meta[] =
{
    { get_member_offset(Game_Settings, initial_master_volume), "master_volume", serialization_type_f32, serialization_data_flag_clamp01 },
    { get_member_offset(Game_Settings, fullscreen), "fullscreen", serialization_type_b32 },
    { get_member_offset(Game_Settings, vsync), "vsync", serialization_type_b32 }
};
#define game_settings_count (array_count(game_settings_meta))


internal Game_Settings game_settings(f32 initial_master_volume = 1.0f, b32 fullscreen = true, b32 vsync = true)
{
    Game_Settings settings = {};
    settings.initial_master_volume = initial_master_volume;
    settings.fullscreen = fullscreen;
    settings.vsync = vsync;

    return settings;
}

internal void build_game_settings_serialization(Game_Settings *game_settings, Serialization_Data *settings)
{
    for(s32 settings_index = 0; settings_index < game_settings_count; ++settings_index)
    {
        Serialization_Data *setting = &settings[settings_index];
        Serialization_Meta *meta = &game_settings_meta[settings_index];

        *setting = meta->serialization;
        setting->data = ((u8 *)game_settings) + meta->data_offset;
        // NOTE: Check that we're using clamp01 with a floating point value only
        assert( !(setting->flags & serialization_data_flag_clamp01) || setting->type == serialization_type_f32 );
    }
}

internal Memory_Index serialize_game_settings(Game_Settings game_settings, Platform_API *platform, char *buffer)
{
    // NOTE: The serialized string doesn't have a null terminator
    
    Serialization_Data settings[game_settings_count];
    build_game_settings_serialization(&game_settings, settings);
    
    char *str = buffer;
    for(s32 setting_index = 0; setting_index < game_settings_count; ++setting_index)
    {
        Serialization_Data *setting = &settings[setting_index];
        // NOTE: setting name
        str = string_copy(setting->name, str);
        // NOTE: space
        str = string_copy(" ", str);
        // NOTE: setting value
        switch(setting->type)
        {
            case serialization_type_f32:
            {
                f32 value = *((f32 *)setting->data);
                if(setting->flags & serialization_data_flag_clamp01)
                {
                    value = clamp01(value);
                }
                str = f32_to_string(value, str);
            } break;
            
            case serialization_type_b32:
            {
                b32 value = *((b32 *)setting->data);
                str = b32_to_string(value, str);
            } break;

            invalid_default_case;
        };

        // newline
        str = string_copy(platform->text_line_ending, str);
    }

    Memory_Index serialized_buffer_size = ((u8 *)str - (u8 *)buffer);
    return serialized_buffer_size;
}

internal char *parse_line_for_settings_value(char *text, char *setting_name)
{
    // IMPORTANT: If the setting is found, this function actually modifies the string
    // by assigning null-terminator at end of value string (so it doesn't include
    // rest of settings_text) Is that bad? I dunno...
    
    char *s = text;
    char *value_start = 0;
    b32 started_parsing_setting_name = false;
    b32 parsing_value = false;
    
    while(*s)
    {
        char c = *s;

        if(parsing_value)
        {
            if(value_start)
            {
                if(is_whitespace(c) || is_newline(c))
                {
                    *s = '\0';
                    break;
                }
            }
            else
            {
                if(!is_whitespace(c) && !is_newline(c))
                {
                    value_start = s;
                }
            }
        }
        else if(c == *setting_name)
        {
            started_parsing_setting_name = true;
            ++setting_name;
            // NOTE: We've hit end of setting so have matched it
            if(*setting_name == '\0')
            {
                parsing_value = true;
            }
        }
        else
        {
            if(is_whitespace(c) && !started_parsing_setting_name)
            {
                // NOTE: Ignore whitespace at start of line    
            }
            else
            {
                break;
            }
        }

        ++s;
    }

    return value_start;
}

internal char *get_line(char **text)
{
    // NOTE: This null terminates the actual text pointer so line pointer
    // actually ends at the end of the line (not including newline characters)
    char *str = *text;
    char *result = str;
    b32 found_new_line = false;
    
    while(*str)
    {
        char c = *str;

        if(is_newline(c))
        {
            *str = '\0';
            found_new_line = true;
        }
        else
        {
            if(found_new_line)
            {
                break;
            }
        }

        ++str;
    }

    // NOTE: This'll set the text pointer to after the line
    *text = str;
    
    return result;
}

internal b32 parse_for_game_settings(char *text, Game_Settings *game_settings)
{
    b32 result = false;
    
    Serialization_Data settings[game_settings_count];
    build_game_settings_serialization(game_settings, settings);
    
    b32 found[game_settings_count] = { 0 };
    u32 game_settings_found = 0;
    u32 line_index = 0;
    u32 line_fail_safe_count = 1024;

    /// NOTE: As a fail safe we quit the loop if hit fail safe count
    while(*text && line_index < line_fail_safe_count)
    {
        ++line_index;
        
        char *line = get_line(&text);
        
        // NOTE: We quit the loop if we've found all settings or (as a fail-safe) if we've been through this
        // loop like loads
        if(game_settings_found < game_settings_count)
        {
            for(s32 settings_index = 0; settings_index < game_settings_count; ++settings_index)
            {
                if(!found[settings_index])
                {
                    Serialization_Data *setting = &settings[settings_index];
                    char *value_string = parse_line_for_settings_value(line, setting->name);
                    if(value_string)
                    {
                        switch(setting->type)
                        {
                            case serialization_type_f32:
                            {
                                f32 *value = (f32 *)setting->data;
                                parse_string_for_f32(value_string, value);
                                if(setting->flags & serialization_data_flag_clamp01)
                                {
                                    *value = clamp01(*value);
                                }
                            } break;

                            case serialization_type_b32:
                            {
                                b32 *value = (b32 *)setting->data;                                
                                parse_string_for_b32(value_string, value);
                            } break;

                            invalid_default_case;
                        };

                        found[settings_index] = true;
                        ++game_settings_found;
                        break;
                    }
                }
            }   
        }
        else
        {
            break;
        }        
    }

    assert(line_index < line_fail_safe_count);
    
    return result;
}

internal void write_game_settings_to_file(Game_Settings settings, Platform_API *platform, Memory_Arena *temp_arena)
{
    Temporary_Memory temp_mem = begin_temporary_memory(temp_arena);
    
    Memory_Index file_buffer_size = megabytes(1);
    char *file_buffer = (char *)push_size(temp_arena, file_buffer_size);

    Memory_Index serialized_buffer_size = serialize_game_settings(settings, platform, file_buffer);
    assert(serialized_buffer_size <= file_buffer_size);

    platform->write_entire_file_to_app_output_directory(settings_filename, file_buffer, serialized_buffer_size);

    end_temporary_memory(temp_mem);

    log_information("Saved game settings");
}

internal Game_Settings load_game_settings_from_file(Platform_API *platform, Memory_Arena *temp_arena)
{
    Game_Settings settings = game_settings();
    
    Platform_File_Handle file_handle = platform->open_file(settings_filename, platform_directory_type_output, platform_file_access_mode_read);
    if(file_handle.handle)
    {
        Temporary_Memory temp_mem = begin_temporary_memory(temp_arena);

        Memory_Index file_buffer_size = kilobytes(2);
        // NOTE: Leave room for file text, and a null-terminator
        char *settings_file_string = (char *)push_size(temp_arena, file_buffer_size + 1, clear_to_zero());
        if(platform->read_data_from_file(&file_handle, (u8 *)settings_file_string, 0, file_buffer_size, false))
        {
            parse_for_game_settings(settings_file_string, &settings);
        }
        else
        {
            invalid_code_path;
        }
                        
        end_temporary_memory(temp_mem);

        platform->close_file(&file_handle);
    }

    log_information("Loaded game settings");
    
    return settings;
}

#endif
