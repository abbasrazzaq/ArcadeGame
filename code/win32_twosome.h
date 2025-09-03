#ifndef WIN32_TWOSOME_H
#define WIN32_TWOSOME_H

struct Game_Code
{
    Game_Update_And_Render *update_and_render;
    Game_Get_Sound_Samples *get_sound_samples;
    
#if TWOSOME_INTERNAL
    HMODULE game_code_dll;
    Debug_Game_Frame_End *DEBUG_frame_end;
    Platform_Date_Time dll_last_write_time;
#endif
};

#define min_sound_latency_ticks 1
#define max_sound_latency_ticks 4
struct Win32_Sound_Output
{
    int32 samples_per_second;
    int32 bytes_per_sample;
    DWORD secondary_buffer_size;
    DWORD expected_sound_bytes_per_tick;
    
    DWORD running_cursor;
    
    int latency_ticks;
};

#if TWOSOME_INTERNAL

struct Win32_DirectSound_Debug_Marker
{
    DWORD output_play_cursor;
    DWORD output_write_cursor;
    
    DWORD output_location;
    DWORD output_byte_count;
};

struct Win32_Replay
{
    HANDLE file_handle;
    b32 playing_back;
    b32 recording;
};

#define DEBUG_win32_state_filename_count MAX_PATH

#endif

struct Win32_State
{
    void *game_memory_block;
    Memory_Index game_memory_block_size;
    
    wchar_t *exe_directory_path;
    Memory_Index exe_directory_path_length;
    
    wchar_t *app_output_path;
    Memory_Index app_output_path_length;
    
    b32 window_made_active;
    b32 show_cursor;
    
    b32 recognise_window_position_dirty_hit;
    b32 window_position_dirty;
    
    RECT previous_window_clip_cursor;
    b32 stored_previous_window_clip_cursor;
    b32 manually_constraining_cursor_in_window;
    
    WINDOWPLACEMENT previous_window_placement;
    
    Memory_Arena temp_arena;
    
    b32 vsync_enabled;
    
    b32 directsound_enabled;
    
    u32 tick_count;    
    b32 failed_to_get_an_opengl_function;
    
    Platform_File_Handle startup_log_file;
    
    b32 startup_failed;
    
#if TWOSOME_INTERNAL
    Win32_Replay replay;
    
#endif
    
#if TWOSOME_SLOW
    b32 DEBUG_stop_further_opengl_function_getting;
    FILE *log_fp;
#endif
    
};

struct Win32_Platform_File_Handle
{
    HANDLE handle;
    s32 access_mode;
};

internal void win32_startup_log(char *message);

#endif
