#import "game_view.h"
#import <AudioToolBox/AudioToolbox.h>

/*
TODO:
- Can't move the window
         - We don't get the resize cursor icon because using CGDisplayHideCursor
- Close button doesn't quit application
       - Can we do single compilation unit?
       - Can we build using a batch file
       - Can we use HID Manager for input?
       - Earliser mac version we OSX v10.5
       - When centering cursor the delta is really high so cursor jumps
        so maybe record how much we jumped and subtract that?
*/


#include "../twosome_opengl.cpp"

global_variable Macosx_State global_macosx_state;

// TODO: Check how much of this is actually used (i.e. can we keep
// it as a circular buffer)

Macosx_Input_Events_Buffer global_input_events_buffers[2];
Macosx_Input_Events_Buffer *global_pushing_events_buffer = &global_input_events_buffers[0];
Macosx_Input_Events_Buffer *global_pulling_events_buffer = &global_input_events_buffers[1];

global_variable s32 global_screen_width = minimum_screen_width;
global_variable s32 global_screen_height = minimum_screen_height;

#define macosx_text_line_ending "\n"

internal PLATFORM_GET_CURRENT_TIME(macosx_get_current_time)
{
    Platform_Date_Time time;
    
#if 0
    NSCalendar *calendar = [[NSCalendar alloc] initWithCalendarIdentifier:NSGregorianCalendar];
    NSCalendarUnit unit_flags = NSYearCalendarUnit | NSMonthCalendarUnit | NSDayCalendarUnit | NSHourCalendarUnit | NSMinuteCalendarUnit | NSSecondCalendarUnit;
#else
    NSCalendar *calendar = [[NSCalendar alloc] initWithCalendarIdentifier:NSCalendarIdentifierGregorian];
    NSCalendarUnit unit_flags = NSCalendarUnitYear | NSCalendarUnitMonth | NSCalendarUnitDay | NSCalendarUnitHour | NSCalendarUnitMinute | NSCalendarUnitSecond | NSCalendarUnitNanosecond;
#endif
    
    NSDate *date = [NSDate date];
    time.milliseconds = (int)([date timeIntervalSinceNow] * -1000.0);
    
    //NSDateComponents *date_components = [calendar components:unit_flags fromDate:date];
    NSDateComponents *date_components = [calendar components:unit_flags fromDate:date];
    
    time.year = (u32)[date_components year];
    time.month = (u32)[date_components month];
    time.day = (u32)[date_components day];
    time.hour = (u32)[date_components hour];
    time.minute = (u32)[date_components minute];
    time.second = (u32)[date_components second];
    
    return time;
}

internal void *macosx_allocate_memory(Memory_Index size, Memory_Index starting_address = 0)
{
    void *result = 0;
    // TODO: Use the starting_address
    vm_allocate((vm_map_t)mach_task_self(), (vm_address_t *)&result, size, VM_FLAGS_ANYWHERE);
    
    assert(result);
    return result;
}

internal void macosx_free_memory(void *memory)
{
    if(memory)
    {
        //vm_deallocate((vm_map_t)mach_task_self(), (vm_address_t)result.contents, (vm_size_t)result.contents_size);
        // TODO: Don't think we can pass in 0 for size,
        // test by seeing if accessing memory again fails.
        // If can't pass in 0 try mmap, munmap
        vm_deallocate((vm_map_t)mach_task_self(), (vm_address_t)memory, (vm_size_t)0);
        
    }
}

internal void macosx_core_audio_callback(void *user_data, AudioQueueRef queue, AudioQueueBufferRef buffer)
{
    UInt32 bytes_to_write = buffer->mAudioDataBytesCapacity;
    
    Macosx_Sound_Output *sound_output = (Macosx_Sound_Output *)user_data;
    
    Game_Sound_Output_Buffer sound_buffer = {};
    sound_buffer.samples_per_second = sound_output->samples_per_second;
    // NOTE: Always ask for multiple of 4 samples
    sound_buffer.sample_count = align4(bytes_to_write / sound_output->bytes_per_sample);
    sound_buffer.samples = (s16 *)buffer->mAudioData;
    
    // TODO: If this is on a separate thread we're gonna have
    // well threading issues..
    sound_output->game->get_sound_samples(sound_output->game_memory, &sound_buffer);
    
    // NOTE: Actual bytes written
    buffer->mAudioDataByteSize = sound_buffer.sample_count * sound_output->bytes_per_sample;
    OSStatus audio_queue_enqueue_buffer_result = AudioQueueEnqueueBuffer(queue, buffer, 0, 0);
    if(audio_queue_enqueue_buffer_result != 0)
    {
        // TODO: Do.. something..
    }
}

internal NSString *alloc_and_init_game_directory_filepath(char *filename, s32 game_directory)
{
    NSString *directory_path = 0;
    switch(game_directory)
    {
        case platform_directory_type_output:
        {
            directory_path = global_macosx_state.app_output_path;
        } break;
        
        case platform_directory_type_app:
        {
            directory_path = global_macosx_state.exe_directory_path;
        } break;
        
        invalid_default_case;
    }
    
    assert(directory_path);
    
    NSString *full_path = [directory_path stringByAppendingFormat:@"%s", filename];
    
    NSString *result = [[NSString alloc] initWithString:full_path];
    return result;
}

internal PLATFORM_OPEN_FILE(macosx_open_file)
{
    Platform_File_Handle result = {};
    
    NSString *filepath = alloc_and_init_game_directory_filepath(filename, game_directory);
    
    NSStream *stream = 0;
    switch(file_access_mode)
    {
        case platform_file_access_mode_write:
        {
            // TODO: Need to check on the append parameter -
            // does it mean that the file position is not 
            // pushed forward when there is a write? Or does
            // it just mean that on open the file position
            // is not set to end of current content?
            stream = [[NSOutputStream alloc] initToFileAtPath:filepath append:false];
        } break;
        
        case platform_file_access_mode_read:
        {
            stream = [[NSInputStream alloc] initWithFileAtPath:filepath];
        } break;
        
        invalid_default_case;
    };
    
    [filepath release];
    filepath = 0;
    
    if(stream)
    {
        [stream open];
        if(stream.streamStatus == NSStreamStatusOpen)
        {
            Macosx_Platform_File_Handle *macosx_file_handle = (Macosx_Platform_File_Handle *)macosx_allocate_memory(sizeof(*macosx_file_handle));
            
            if(macosx_file_handle)
            {
                macosx_file_handle->stream = stream;
                macosx_file_handle->access_mode = file_access_mode;
            }
            
            result.handle = macosx_file_handle;
        }
        else
        {
            // TODO: Log error
        }
    }
    else
    {
        // TODO: Log
    }
    
    return result;
}

internal PLATFORM_READ_DATA_FROM_FILE(macosx_read_data_from_file)
{
    b32 result = false;
    
    Macosx_Platform_File_Handle *macosx_file_handle = (Macosx_Platform_File_Handle *)source->handle;
    assert(macosx_file_handle->access_mode == platform_file_access_mode_read);
    
    NSInputStream *input_stream = (NSInputStream *)macosx_file_handle->stream;
    
    // TODO: Not sure about the casting situation here? 
    // Does u64 need to be cast? Are we using the right type
    // for this property?
    [input_stream setProperty:[NSNumber numberWithUnsignedLong:offset] forKey:NSStreamFileCurrentOffsetKey];
    
    NSUInteger bytes_to_read = safe_truncate_uint64(size);
    NSInteger bytes_read = [input_stream read:(uint8_t *)buffer maxLength:bytes_to_read];
    
    if(bytes_read > 0)
    {
        if((expect_exact_size && bytes_read == bytes_to_read)
           || (!expect_exact_size && bytes_read <= bytes_to_read))
        {
            result = true;
        }
    }
    else
    {
        // TODO: Log error
    }
    
    return result;
}

internal void _macosx_write_data_to_file(Platform_File_Handle *source, u8 *buffer, u64 size, u64 offset, b32 ignore_offset)
{
    Macosx_Platform_File_Handle *macosx_file_handle = (Macosx_Platform_File_Handle *)source->handle;
    assert(macosx_file_handle->access_mode == platform_file_access_mode_write);
    
    NSOutputStream *output_stream = (NSOutputStream *)macosx_file_handle->stream;
    
    if(!ignore_offset)
    {
        // TODO: Similar to read, not sure about casting situation
        // here and number type 
        [output_stream setProperty:[NSNumber numberWithUnsignedLong:offset] forKey:NSStreamFileCurrentOffsetKey];
    }
    
    NSInteger bytes_written = [output_stream write:(uint8_t *)buffer maxLength:size];
    
    if(bytes_written != size)
    {
        // LOG:
    }
}

internal PLATFORM_WRITE_DATA_TO_FILE(macosx_write_data_to_file)
{
    _macosx_write_data_to_file(source, buffer, size, offset, false);
}

internal void macosx_write_data_to_file_append(Platform_File_Handle *source, u8 *buffer, u64 size)
{
    _macosx_write_data_to_file(source, buffer, size, 0, true);
}

internal PLATFORM_CLOSE_FILE(macosx_close_file)
{
    Macosx_Platform_File_Handle *macosx_file_handle = (Macosx_Platform_File_Handle *)file->handle;
    
    [macosx_file_handle->stream close];
    [macosx_file_handle->stream release];
    
    macosx_free_memory(file->handle);
    file->handle = 0;
}

internal PLATFORM_READ_ENTIRE_FILE(macosx_read_entire_file)
{
    b32 result = false;
    
    Platform_File_Handle file_handle = macosx_open_file(filename, game_directory, platform_file_access_mode_read);
    
    if(file_handle.handle)
    {
        result = macosx_read_data_from_file(&file_handle, (u8 *)memory, 0, memory_size, true);
        macosx_close_file(&file_handle);
    }
    
    return result;
}

internal PLATFORM_WRITE_ENTIRE_FILE_TO_APP_OUTPUT_DIRECTORY(macosx_write_entire_file_to_app_output_directory)
{
    Platform_File_Handle file_handle = macosx_open_file(filename, platform_directory_type_output, platform_file_access_mode_write);
    if(file_handle.handle)
    {
        macosx_write_data_to_file(&file_handle, (u8 *)memory, 0, memory_size);
        macosx_close_file(&file_handle);
    }
}

internal PLATFORM_LOAD_GAME_SAVE_FILE(macosx_load_game_save_file)
{
    // TODO: This looks to be the save as win32 layer, can
    // we move this to game code?
    
    // NOTE: If save file doesn't exist, or doesn't match the expected size of the save data, try to load the backup
    b32 result = false;
    if(macosx_read_entire_file(save_filename, platform_directory_type_output, save.data, save.data_size))
    {
        result = true;
    }
    else if(macosx_read_entire_file(backup_save_filename, platform_directory_type_output, save.data, save.data_size))
    {
        // TOOD: Log warning
        // NOTE: If we had to use the backup file, then the current save is corrupted/missing and needs to be updated with the one that is legit. This is important because if save file is corrupted then backup mechanism could potentially backup a corrupted save file.
        
        macosx_write_entire_file_to_app_output_directory(save_filename, save.data, save.data_size);
        
        result = true;
    }
    
    return result;
}

internal PLATFORM_WRITE_GAME_SAVE_FILE(macosx_write_game_save_file)
{
    // TODO: Can move this to game code as looks similar
    // to win32 layer? Just would need to move some code out
    // into a "backup file" platform function
    
    Temporary_Memory temp_mem = begin_temporary_memory(&global_macosx_state.temp_arena);
    // NOTE: Backup existing save file before writing to it
    {
        NSString *save_filepath = alloc_and_init_game_directory_filepath(save_filename, platform_directory_type_output);
        
        NSString *backup_save_filepath = alloc_and_init_game_directory_filepath(backup_save_filename, platform_directory_type_output);
        
        // NOTE: Copy File
        NSError *copy_error;
        bool copy_result = [[NSFileManager defaultManager] copyItemAtPath:save_filepath toPath:backup_save_filepath error:&copy_error];
        if(!copy_result)
        {
            if(copy_error)
            {
                // TODO: Log - NSLog(@"%@", [create_error description]);
            }
        }
        
        [save_filepath release];
        [backup_save_filepath release];
    }
    end_temporary_memory(temp_mem);
    
    macosx_write_entire_file_to_app_output_directory(save_filename, save.data, save.data_size);
}

internal b32 macosx_get_exe_filepath(Macosx_State *state)
{
    b32 result = false;
    
    NSString *exe_directory_path_sans_end_slash = [[NSFileManager defaultManager] currentDirectoryPath];
    
    NSString *exe_directory_path_inc_slash = [exe_directory_path_sans_end_slash stringByAppendingString:@"/"];
    
    state->exe_directory_path = [[NSString alloc] initWithString:exe_directory_path_inc_slash];
    
    return result;
}

internal b32 macosx_get_app_output_path(Macosx_State *state)
{
    b32 result = false;
    
    NSArray *paths = NSSearchPathForDirectoriesInDomains(NSApplicationSupportDirectory, NSUserDomainMask, true);
    NSString *partial_app_output_path = paths[0];
    
    NSString *full_app_output_path = [partial_app_output_path stringByAppendingString:@"/Twosome/"];
    
    // NOTE: Append game's own folder name
    global_macosx_state.app_output_path = [[NSString alloc] initWithString:full_app_output_path];
    
    if(global_macosx_state.app_output_path)
    {
        NSError *create_error;
        b32 create_directory_result = [[NSFileManager defaultManager] createDirectoryAtPath:global_macosx_state.app_output_path withIntermediateDirectories:false attributes:nil error:&create_error];
        
        if(create_directory_result)
        {
            result = true;
        }
        else if(create_error)
        {
            if(create_error.code == NSFileWriteFileExistsError)
            {
                result = true;
            }
            else
            {
                // TODO: Log - NSLog(@"%@", [create_error description]);
                
            }
        }
        else
        {
            // TODO: Log generic error message
            
        }
    }
    
    
    return result;
}

internal PLATFORM_ADD_ENTRY(macosx_add_entry)
{
    // TODO: Do work queue, for now just do work here
    callback(queue, data);
    
}

extern "C" GAME_UPDATE_AND_RENDER(game_update_and_render);
extern "C" GAME_GET_SOUND_SAMPLES(game_get_sound_samples);

internal Game_Code load_game_code(void)
{
    Game_Code result = {};
    result.update_and_render = game_update_and_render;
    result.get_sound_samples = game_get_sound_samples;
    
    return result;
}

internal void macosx_buffer_input_event(Macosx_Input_Event event)
{
    // TODO: Lock
    global_pushing_events_buffer->events[global_pushing_events_buffer->events_count++] = event;
    // TODO: Unlock
}

internal void macosx_buffer_keyboard_event(unsigned short key_code, b32 down)
{
    Macosx_Input_Event event = {};
    event.key_code = key_code;
    event.key_down = down;
    event.type = macosx_input_event_type_keyboard;
    
    macosx_buffer_input_event(event);
}

internal void macosx_buffer_mouse_event(u32 mouse_button, b32 down)
{
    Macosx_Input_Event event = {};
    event.type = macosx_input_event_type_mouse_clicked;
    event.mouse_button = mouse_button;
    event.mouse_button_down = down;
    
    macosx_buffer_input_event(event);
}

internal void macosx_process_input_message(Game_Button_State *new_state, b32 is_down)
{
    if(new_state->down != is_down)
    {
        new_state->down = is_down;
        ++new_state->half_transition_count;
    }
}

@implementation Game_View

- (void) app_activeness_changed:(b32)active
{
    CGDirectDisplayID display_id = CVDisplayLinkGetCurrentCGDisplay(display_link);
    
    if(active)
    {
        CGAssociateMouseAndMouseCursorPosition(false);
        
        [self center_cursor_in_window];
        
        CGDisplayHideCursor(display_id);
    }
    else
    {
        CGDisplayShowCursor(display_id);
    }
}

- (CVReturn) get_frame_for_time:(const CVTimeStamp *)output_time
{
    // NOTE: Do input swapping from last frame
    for(s32 button_index = 0; button_index < array_count(new_input->buttons); ++button_index)
    {
        zero_object(Game_Button_State, new_input->buttons[button_index]);
        new_input->buttons[button_index].down = old_input->buttons[button_index].down;
    }
    
    f32 target_seconds_per_frame = (f32)(1.0 / game_update_hz);
    
    new_input->mouse_x = 0;
    new_input->mouse_y = 0;
    
    new_input->dt = target_seconds_per_frame;
    new_input->settings = game_settings;
    
    Game_Render_Commands render_commands = {};
    render_commands.push_buffer_base = (u8 *)render_commands_push_buffer;
    render_commands.push_buffer_size = render_commands_push_buffer_size;
    render_commands.push_buffer_at = render_commands.push_buffer_base + render_commands.push_buffer_size;
    render_commands.window_width = global_screen_width;
    render_commands.window_height = global_screen_height;
    
    //
    // TODO: Collect input
    //
    {
        
        // TODO: Lock
        
        Macosx_Input_Events_Buffer *temp = global_pulling_events_buffer;
        global_pulling_events_buffer = global_pushing_events_buffer;
        global_pushing_events_buffer = temp;
        global_pushing_events_buffer->events_count = 0;
        
        // TODO: Unlock
        
        
        // TODO: Check this makes sense when we go over buffer
        Macosx_Input_Events_Buffer *events_buffer = global_pulling_events_buffer;
        // TODO: Work this out properly like
        u32 events_start_index = 0; // (events_buffer->events_count % array_count(events_buffer->events));
        
        u32 events_count = min(events_buffer->events_count, array_count(events_buffer->events));
        
        for(u32 overrunning_event_index = events_start_index; overrunning_event_index < events_count; ++overrunning_event_index)
        {
            u32 event_index = overrunning_event_index % array_count(events_buffer->events);
            
            Macosx_Input_Event *event = &events_buffer->events[event_index];
            
            if(event->type == macosx_input_event_type_keyboard)
            {
                switch(event->key_code)
                {
                    case kVK_Escape:
                    {
                        macosx_process_input_message(&new_input->pause_button, event->key_down);
                    } break;
                }
            }
            else if(event->type == macosx_input_event_type_mouse_clicked)
            {
                switch(event->mouse_button)
                {
                    case macosx_mouse_event_button_left:
                    {
                        macosx_process_input_message(&new_input->change_colour_button, event->mouse_button_down);
                    } break;
                    
                    case macosx_mouse_event_button_right:
                    {        macosx_process_input_message(&new_input->activate_shield_button, event->mouse_button_down);
                    } break;
                    
                    invalid_default_case;
                };
            }
            else if(event->type == macosx_input_event_type_mouse_moved)
            {
                new_input->mouse_x += event->mouse_moved.x;
                new_input->mouse_y -= event->mouse_moved.y;
            }
            else
            {
                invalid_code_path;
            }
        }
    }
    
    //
    // NOTE: Update Game
    //
    {
        Game_Update_And_Render_Result update_and_render_result = game.update_and_render(&game_memory, new_input, &render_commands);
        if(update_and_render_result.quit_game)
        {
            [NSApp terminate:self];
        }
        if(update_and_render_result.game_restarted)
        {
            // NOTE: If the game restarted, the game settings may have be updated, and we need to make
            // sure we've got the up to date settings for when game inits
            game_settings = load_game_settings_from_file(&platform_api, &global_macosx_state.temp_arena);
        }
        if(update_and_render_result.toggle_fullscreen)
        {
            // TODO
        }
        //if(update_and_render_result.game_assets_load_result == game_assets_load_result_failed)
        {
            // TODO:
        }
    }
    
    //
    // TODO: Sound
    //
    {
        // TODO: Can remove? Now that callback is setup?
        /*Game_Sound_Output_Buffer sound_buffer = {};
        sound_buffer.samples_per_second = 44100;
        sound_buffer.sample_count = (1.0f / 30.0f) * 44100.0f; // TODO: Enough for one frame
        sound_buffer.samples = samples;
        
        game.get_sound_samples(&game_memory, &sound_buffer);*/
    }
    
    CGLLockContext(opengl_context.CGLContextObj);
    [opengl_context makeCurrentContext];
    
    // NOTE: Execute render commands
    {
        Temporary_Memory temp_mem = begin_temporary_memory(&global_macosx_state.temp_arena);
        
        Game_Render_Prep prep = sort_render_entries(&render_commands, &global_macosx_state.temp_arena);
        
        opengl_execute_render_commands(&render_commands, &prep, &global_macosx_state.temp_arena);
        
        end_temporary_memory(temp_mem);
    }
    
    [opengl_context flushBuffer];
    
    CGLUnlockContext(opengl_context.CGLContextObj);
    
    Game_Input *temp = new_input;
    new_input = old_input;
    old_input = temp;
    
    return kCVReturnSuccess;
}

static CVReturn display_link_callback(CVDisplayLinkRef display_link, const CVTimeStamp *now, const CVTimeStamp *output_time, CVOptionFlags flags_in, CVOptionFlags *flags_out, void *display_link_context)
{
    CVReturn result = [(Game_View *)display_link_context get_frame_for_time:output_time];
    return result;
}

- (float) transform_y:(f32 )y
{
    CGDirectDisplayID display_id = CVDisplayLinkGetCurrentCGDisplay(display_link);
    
    float height = CGDisplayBounds(display_id).size.height;
    f32 result = height - y;
    return result;
}

- (void) center_cursor_in_window
{
    f32 x = global_screen_width/2.0f;
    f32 y = global_screen_height/2.0f;
    
    NSRect content_rect = [self frame];
    
    NSRect local_rect = NSMakeRect(x, content_rect.size.height - y - 1, 0, 0);
    NSRect global_rect = [_window convertRectToScreen:local_rect];
    NSPoint global_point = global_rect.origin;
    
    f32 transformed_y = [self transform_y:global_point.y];
    
    CGWarpMouseCursorPosition(CGPointMake(global_point.x, transformed_y));
    
    cursor_centering_jump_delta_offset_count = 2;
}

- (void) awakeFromNib
{
    CVDisplayLinkStart(display_link);
}

struct Platform_Work_Queue
{
    // TODO: Implement!
};

- (id)initWithCoder:(NSCoder *)decoder
{
    
    Platform_Work_Queue work_queue = {};
    // TODO: macosx_make_queue
    
    
    //
    // NOTE: Platform-layer services to game
    //
    platform_api = {};
    platform_api.open_file = macosx_open_file;
    platform_api.read_data_from_file = macosx_read_data_from_file;
    platform_api.close_file = macosx_close_file;
    platform_api.get_current_time = macosx_get_current_time;
    
    platform_api.write_entire_file_to_app_output_directory =macosx_write_entire_file_to_app_output_directory;
    
    platform_api.load_game_save_file = macosx_load_game_save_file;
    platform_api.write_game_save_file = macosx_write_game_save_file;
    platform_api.add_entry = macosx_add_entry;
    // TODO: platform_api.complete_all_work
    platform_api.text_line_ending = macosx_text_line_ending;
    platform_api.player_platform_option_flags = player_platform_option_fullscreen_flag | player_platform_option_quit_flag;
    
    //
    // NOTE: Allocate game memory
    //
    game_memory = {};
    game_memory.permanent_storage_size = megabytes(32);
    game_memory.transient_storage_size = megabytes(192);
    
    u32 game_memory_block_size = game_memory.permanent_storage_size + game_memory.transient_storage_size;
    
    Memory_Index base_address = 0;
#if TWOSOME_INTERNAL
    base_address = terabytes(5);
#endif
    
    void *game_memory_block = macosx_allocate_memory(game_memory_block_size, base_address);
    assert(!game_memory_block);
    
    game_memory.permanent_storage = game_memory_block;
    game_memory.transient_storage = ((u8 *)game_memory.permanent_storage + game_memory.permanent_storage_size);
    
    game_memory.work_queue = &work_queue;
    game_memory.platform_api = platform_api;
    
    set_global_debug_state(&game_memory);
    
    // NOTE: Global Macosx State Temporary Arena
    {
        Memory_Index macosx_temp_buffer_size = megabytes(16);
        void *macosx_temp_buffer = macosx_allocate_memory(macosx_temp_buffer_size);
        if(macosx_temp_buffer)
        {
            initialize_arena(&global_macosx_state.temp_arena, macosx_temp_buffer_size, macosx_temp_buffer);
            TRACK_MEMORY_ARENA(global_macosx_state.temp_arena);
        }
        else
        {
            // TODO: Log
        }
    }
    
    // NOTE: Get app output directory path
    if(!macosx_get_app_output_path(&global_macosx_state))
    {
        // TODO: Log
    }
    
    // TODO: Startup log file
    
    // NOTE: Get path to game directory
    if(!macosx_get_exe_filepath(&global_macosx_state))
    {
        // TODO: Log
    }
    
    // TODO: Setup logging
    
    game_settings = load_game_settings_from_file(&platform_api, &global_macosx_state. temp_arena);
    
    //
    // TODO: Go into fullscreen mode based on game settings
    //
    
    //
    // TODO: Enable/Disable V-sync based on game settings
    //
    
    zero_buffer(input, sizeof(input));
    
    // TODO: Probably don't need this now that we got buffers
    // for input
    new_input = &input[0];
    old_input = &input[1];
    
    render_commands_push_buffer_size = megabytes(32);
    render_commands_push_buffer = macosx_allocate_memory(render_commands_push_buffer_size);
    TRACK_BUFFER_USAGE(render_commands_push_buffer, render_commands_push_buffer_size);
    if(!render_commands_push_buffer)
    {
        // TODO: Log
    }
    
    NSOpenGLPixelFormatAttribute attribs[] =
    {
        NSOpenGLPFAOpenGLProfile, NSOpenGLProfileVersion3_2Core,
        NSOpenGLPFADepthSize, 24,
        NSOpenGLPFADoubleBuffer,
        NSOpenGLPFAAccelerated,
        0
    };
    
    pixel_format = [[NSOpenGLPixelFormat alloc] initWithAttributes:attribs];
    
    opengl_context = [[NSOpenGLContext alloc] initWithFormat:pixel_format shareContext:nil];
    
    if(self = [super initWithCoder:decoder])
    {
        [opengl_context makeCurrentContext];
        
        GLint swap_int = 1;
        [opengl_context setValues:&swap_int forParameter:NSOpenGLCPSwapInterval];
        
        // Setup Display Link
        // TODO: What happens if can't do vsync?
        {
            // TODO: Check return codes of these functions
            
            CVDisplayLinkCreateWithActiveCGDisplays(&display_link);
            
            //CGDisplayModeGetRefreshRate(CGMainDisplayID());
            
            //CGDisplayModeRef display_mode_ref;
            //CGDisplayModeGetRefreshRate(display_mode_ref);
            
            CVDisplayLinkSetOutputCallback(display_link, &display_link_callback, self);
            
            CVDisplayLinkSetCurrentCGDisplayFromOpenGLContext(display_link, opengl_context.CGLContextObj, pixel_format.CGLPixelFormatObj);
            
            // NOTE: Get monitor refresh rate
            CGDirectDisplayID display_id = CVDisplayLinkGetCurrentCGDisplay(display_link);
            
            CGDisplayModeRef display_mode = CGDisplayCopyDisplayMode(display_id);
            
            double monitor_hz = CGDisplayModeGetRefreshRate(display_mode);
            
            game_update_hz = monitor_hz;
            
            CGDisplayModeRelease(display_mode);
            
        }
        
        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector (reshape) name:NSViewGlobalFrameDidChangeNotification object:self];
        
        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(windowWillClose:) name:NSWindowWillCloseNotification object:_window];
    }
    
    check_gl_and_glsl_compatibility();
    
    opengl_load_resources(&global_macosx_state.temp_arena);
    
    game = load_game_code();
    
    //
    // TODO: Setup CoreAudio
    //
    AudioStreamBasicDescription format = {};
    
    zero_object(Macosx_Sound_Output, sound_output);
    
    s32 channels = 2;
    sound_output.bytes_per_sample = sizeof(s16)*channels;
    s32 bit_depth = (sound_output.bytes_per_sample / channels) * 8;
    sound_output.samples_per_second = 44100;
    
    sound_output.game_memory = &game_memory;
    sound_output.game = &game;
    
    format.mBitsPerChannel = bit_depth;
    format.mBytesPerFrame = sound_output.bytes_per_sample;
    format.mBytesPerPacket = sound_output.bytes_per_sample;
    format.mChannelsPerFrame = channels;
    format.mFormatFlags = kLinearPCMFormatFlagIsBigEndian | kLinearPCMFormatFlagIsSignedInteger | kLinearPCMFormatFlagIsPacked;
    format.mFormatID = kAudioFormatLinearPCM;
    format.mFramesPerPacket = 1;
    format.mSampleRate = sound_output.samples_per_second;
    
    AudioQueueRef queue;
    OSStatus audio_queue_new_output_result = AudioQueueNewOutput(&format, macosx_core_audio_callback, &sound_output, CFRunLoopGetCurrent(), kCFRunLoopCommonModes, 0, &queue);
    // TODO: Do something with audio_queue_new_output_result if error
    
    AudioQueueBufferRef buffer_one;
    AudioQueueBufferRef buffer_two;
    
    // TODO: This is already calculated further down, use the actual monitor hz
    s32 sound_update_hz = 60;
    // TODO: Do we need latency bytes? How do we line it up so sound
    // updates happen at the same time as game updates?
    s32 expected_sound_bytes_per_tick = (f32)(sound_output.samples_per_second * sound_output.bytes_per_sample) / (f32)sound_update_hz;
    UInt32 buffer_byte_size = expected_sound_bytes_per_tick;
    AudioQueueAllocateBuffer(queue, buffer_byte_size, &buffer_one);
    AudioQueueAllocateBuffer(queue, buffer_byte_size, &buffer_two);
    
    macosx_core_audio_callback(&sound_output, queue, buffer_one);
    macosx_core_audio_callback(&sound_output, queue, buffer_two);
    
    AudioQueueSetParameter(queue, kAudioQueueParam_Volume, 1.0f);
    OSStatus audio_queue_start_result = AudioQueueStart(queue, 0);
    
    samples_buffer_size = kilobytes(512);
    
    samples = (s16 *)macosx_allocate_memory(samples_buffer_size);
    
    
    NSRect view_bounds = [self bounds];
    
    // NOTE: Track mouse inside window
    // TODO: Should we move this code earlier (as soon as we can start buffering messages?)
    // TODO: Do we still need this?
    NSTrackingArea *tracking_area = [[NSTrackingArea alloc] initWithRect:view_bounds
            options: (NSTrackingMouseMoved | NSTrackingActiveInActiveApp | NSTrackingMouseEnteredAndExited)
            owner:self userInfo:nil];
    [self addTrackingArea:tracking_area];
    
    return self;
}

- (void)windowWillClose:(NSNotification *)notification
{
    [NSApp terminate:self];
}

- (void) lockFocus
{
    [super lockFocus];
    
    if(opengl_context.view != self)
    {
        [opengl_context setView:self];
    }
}

- (void) reshape
{
    CGLLockContext(opengl_context.CGLContextObj);
    [opengl_context update];
    CGLUnlockContext(opengl_context.CGLContextObj);
}

- (BOOL) acceptsFirstResponder
{
    return YES;
}

- (void) keyDown:(NSEvent *)ns_event
{
    // TODO: WOuld be good to only collect events for the
    // keys that we care about.
    macosx_buffer_keyboard_event(ns_event.keyCode, true);
}

- (void) keyUp:(NSEvent *)ns_event
{
    // TODO: WOuld be good to only collect events for the
    // keys that we care about.
    macosx_buffer_keyboard_event(ns_event.keyCode, false);
}

- (void) mouseDown:(NSEvent *)ns_event
{
    macosx_buffer_mouse_event(macosx_mouse_event_button_left, true);
}

- (void) mouseUp:(NSEvent *)ns_event
{
    macosx_buffer_mouse_event(macosx_mouse_event_button_left, false);
}

- (void) rightMouseDown:(NSEvent *)ns_event
{
    macosx_buffer_mouse_event(macosx_mouse_event_button_right, true);
}

- (void) rightMouseUp:(NSEvent *)ns_event
{
    macosx_buffer_mouse_event(macosx_mouse_event_button_right, false);
}

- (void) mouseMoved:(NSEvent *)ns_event
{
    // TODO: Is this affected by acceleration? Do we care?
    
    Macosx_Input_Event event = {};
    event.type = macosx_input_event_type_mouse_moved;
    
    CGFloat delta_x = [ns_event deltaX];
    CGFloat delta_y = [ns_event deltaY];
    
    if(delta_x != 0 || delta_y != 0)
    {
        event.mouse_moved = vec2(delta_x, delta_y);
        
        printf("x: %f; y%f" macosx_text_line_ending, delta_x, delta_y);
        
        // TODO: Do we need to care about thread sychonisation here?
        
        // NOTE(HACK): Seems like jumping the mouse move
        // affects more than 1 event with a large delta
        // (I don't know...) so we offset multiple events
        if(cursor_centering_jump_delta_offset_count > 0)
        {
            event.mouse_moved.x = 0;
            event.mouse_moved.y = 0;
            
            --cursor_centering_jump_delta_offset_count;
        }
    }
    
    macosx_buffer_input_event(event);
}

@end