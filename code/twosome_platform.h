#ifndef TWOSOME_PLATFORM_H
#define TWOSOME_PLATFORM_H


/*
  NOTE: Preprocessor flags:
  
  TWOSOME_INTERNAL:
  0 - Build for public release.
  1 - Build for develop only.
  
  TWOSOME_SLOW:
  0 - No slow code allowed!
  1 - Slow code welcome.
 */


//
// NOTE: Compilers
//
#if !defined(COMPILER_MSVC)
#define COMPILER_MSVC 0
#endif

#if !defined(COMPILER_LLVM)
#define COMPILER_LLVM 0
#endif

// NOTE: Lets try and determine compiler ourselves
#if !COMPILER_MSVC && !COMPILER_LLVM
#if _MSC_VER
#undef COMPILER_MSVC
#define COMPILER_MSVC 1
#else
#undef COMPILER_LLVM
#define COMPILER_LLVM 1
#endif
#endif

#if COMPILER_MSVC
#include <intrin.h>
#elif COMPILER_LLVM
#include <xmmintrin.h>
#include <x86intrin.h>
#else
#error SEE/NEON are not available for this compiler yet!!!!
#endif

#include <stddef.h>
#include <stdint.h>
#include <math.h>
#include <limits.h>
#include <string.h>

#if TWOSOME_SLOW || TWOSOME_INTERNAL
#include <stdarg.h>
#include <stdio.h>
#endif

#define internal static
#if TWOSOME_INTERNAL
#define local_persist static
#endif
#define global_variable static

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;
typedef int32 bool32;

typedef int8 s8;
typedef int16 s16;
typedef int32 s32;
typedef int64 s64;
typedef bool32 b32;

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;
typedef uintptr_t uintptr;
typedef intptr_t intptr;

typedef uint8 u8;
typedef uint16 u16;
typedef uint32 u32;
typedef uint64 u64;

typedef float real32;
typedef double real64;

typedef real32 r32;
typedef real64 r64;

typedef r32 f32;
typedef r64 f64;

typedef size_t Memory_Index;
typedef ptrdiff_t ptrdiff;

#define u32_from_pointer(pointer) ((u32)(Memory_Index)(pointer))

#define min(value_1, value_2) ( ((value_1) < (value_2)) ? (value_1) : (value_2) )
#define max(value_1, value_2) ( ((value_1) > (value_2)) ? (value_1) : (value_2) )

#define array_count(array) ( sizeof((array)) / sizeof((array)[0]) )
#define clamp(value, min, max) ((value) > (max) ? (max) : (value) < (min) ? (min) : (value))
#define clamp01(value) ( clamp(value, 0, 1) )
// NOTE: Don't want to allocate a large object on stack so we assert on that
#define zero_object(Type, obj) { assert(sizeof(Type) < 1024); Type zero_obj = {}; (obj) = zero_obj; }

internal void zero_buffer(void *ptr, Memory_Index size)
{
    memset(ptr, 0, size);
}


#define get_member_offset(Type, member) ((Memory_Index)(((u8 *) (&((Type *)0)->member) ) - ((u8 *)((Type *)0))))

#define kilobytes(value) ((value) * 1024LL)
#define megabytes(value) (kilobytes(value) * 1024LL)
#define gigabytes(value) (megabytes(value) * 1024LL)
#define terabytes(value) (gigabytes(value) * 1024LL)

#define bytes_to_megabytes(value) ( (value) * (1.0f / megabytes(1)) )

#define square(value) ((value)*(value))
#define interpolate(v1, v2, t) ( ((v1) * (1.0f - (t))) + ((v2) * (t)) )

#define swap(Type, v1, v2) { \
    Type temp = v1; \
    v1 = v2; \
    v2 = temp; \
}

#define align4(v) ( ((v) + 3) & ~3 )
#define align8(v) ( ((v) + 7) & ~7 )

#define s16_max_value SHRT_MAX
#define s16_min_value SHRT_MIN

#define s32_max_value LONG_MAX
#define s32_min_value LONG_MIN

#define u32_max_value ULONG_MAX

struct Debug_Read_File_Result
{
    uint32 contents_size;
    void *contents;
};

struct Platform_File_Handle
{
    void *handle;
};

struct Platform_Date_Time
{
    u32 year;
    u32 month;
    u32 day;
    
    u32 hour;
    u32 minute;
    u32 second;
    u32 milliseconds;
};

enum Platform_Game_Directory
{
    platform_directory_type_null = 0,
    platform_directory_type_output, // where the os is happy for us to write to (save, settings)
    platform_directory_type_app // where the exe is
};

struct Game_Settings
{
    f32 initial_master_volume;
    b32 fullscreen;
    b32 vsync;
};

//
// NOTE: Filenames
//
#define asset_filename "data.ta"

#if TWOSOME_INTERNAL
#define DEBUG_path_from_build_path_to_src_data "../proj/data/sounds/"
#define DEBUG_font_path "c:\\windows\\fonts\\arial.ttf"
#endif

#define save_filename "save.dat"
#define backup_save_filename "backup_save.dat"
#define settings_filename "settings.txt"


struct Platform_Game_Save
{
    void *data;
    Memory_Index data_size;
};

//
// NOTE: Platform-layer services
//
#define DEBUG_PLATFORM_LOGGING(name) void name(char *message)
typedef DEBUG_PLATFORM_LOGGING(Debug_Platform_Logging);

#define DEBUG_PLATFORM_SHOW_ASSERT_POPUP(name) void name(char *message)
typedef DEBUG_PLATFORM_SHOW_ASSERT_POPUP(Debug_Platform_Show_Assert_Popup);

#define DEBUG_PLATFORM_GET_WALL_CLOCK_SECONDS(name) float name(void)
typedef DEBUG_PLATFORM_GET_WALL_CLOCK_SECONDS(Debug_Platform_Get_Wall_Clock_Seconds);

#define DEBUG_PLATFORM_DELETE_FILE(name) void name(char *filename)
typedef DEBUG_PLATFORM_DELETE_FILE(Debug_Platform_Delete_File);

#define DEBUG_PLATFORM_COUNT_FILES_IN_DIRECTORY(name) uint32 name(char *directory_path, char *search_pattern)
typedef DEBUG_PLATFORM_COUNT_FILES_IN_DIRECTORY(Debug_Platform_Count_Files_In_Directory);

#define DEBUG_PLATFORM_GET_LAST_FILE_WRITE_TIME(name) Platform_Date_Time name(char *filepath)
typedef DEBUG_PLATFORM_GET_LAST_FILE_WRITE_TIME(Debug_Platform_Get_Last_File_Write_Time);

#define DEBUG_PLATFORM_READ_ENTIRE_FILE(name) Debug_Read_File_Result name(char *filename)
typedef DEBUG_PLATFORM_READ_ENTIRE_FILE(Debug_Platform_Read_Entire_File);
#define DEBUG_PLATFORM_WRITE_ENTIRE_FILE(name) bool32 name(char *filename, void *memory, uint32 memory_size)
typedef DEBUG_PLATFORM_WRITE_ENTIRE_FILE(Debug_Platform_Write_Entire_File);
#define DEBUG_PLATFORM_FREE_FILE_MEMORY(name) void name(void *memory)
typedef DEBUG_PLATFORM_FREE_FILE_MEMORY(Debug_Platform_Free_File_Memory);

#define PLATFORM_GET_CURRENT_TIME(name) Platform_Date_Time name(void)
typedef PLATFORM_GET_CURRENT_TIME(Platform_Get_Current_Time);

#define PLATFORM_READ_ENTIRE_FILE(name) b32 name(char *filename, s32 game_directory, void *memory, u64 memory_size)
typedef PLATFORM_READ_ENTIRE_FILE(Platform_Read_Entire_File);

#define PLATFORM_WRITE_ENTIRE_FILE_TO_APP_OUTPUT_DIRECTORY(name) void name(char *filename, void *memory, u64 memory_size)
typedef PLATFORM_WRITE_ENTIRE_FILE_TO_APP_OUTPUT_DIRECTORY(Platform_Write_Entire_File_To_App_Output_Directory);

enum Platform_File_Access_Mode
{
    platform_file_access_mode_write,
    platform_file_access_mode_read
};

#define PLATFORM_OPEN_FILE(name) Platform_File_Handle name(char *filename, s32 game_directory, s32 file_access_mode)
typedef PLATFORM_OPEN_FILE(Platform_Open_File);

#define PLATFORM_READ_DATA_FROM_FILE(name) b32 name(Platform_File_Handle *source, u8 *buffer, u64 offset, u64 size, b32 expect_exact_size)
typedef PLATFORM_READ_DATA_FROM_FILE(Platform_Read_Data_From_File);

#define PLATFORM_WRITE_DATA_TO_FILE(name) void name(Platform_File_Handle *source, u8 *buffer, u64 offset, u64 size)
typedef PLATFORM_WRITE_DATA_TO_FILE(Platform_Write_Data_To_File);

#define PLATFORM_CLOSE_FILE(name) void name(Platform_File_Handle *file)
typedef PLATFORM_CLOSE_FILE(Platform_Close_File);

#define DEBUG_PLATFORM_GET_LATEST_FILE_WRITE_TIME_IN_DIRECTORY(name) Platform_Date_Time name(char *directory_path)
typedef DEBUG_PLATFORM_GET_LATEST_FILE_WRITE_TIME_IN_DIRECTORY(Debug_Platform_Get_Latest_File_Write_Time_In_Directory);
#define PLATFORM_LOAD_GAME_SAVE_FILE(name) b32 name(Platform_Game_Save save)
typedef PLATFORM_LOAD_GAME_SAVE_FILE(Platform_Load_Game_Save_File);

#define PLATFORM_WRITE_GAME_SAVE_FILE(name) void name(Platform_Game_Save save)
typedef PLATFORM_WRITE_GAME_SAVE_FILE(Platform_Write_Game_Save_File);

struct Platform_Work_Queue;
#define PLATFORM_WORK_QUEUE_CALLBACK(name) void name(Platform_Work_Queue *queue, void *data)
typedef PLATFORM_WORK_QUEUE_CALLBACK(Platform_Work_Queue_Callback);

#define PLATFORM_ADD_ENTRY(name) void name(Platform_Work_Queue *queue, Platform_Work_Queue_Callback *callback, void *data)
typedef PLATFORM_ADD_ENTRY(Platform_Add_Entry);

#define PLATFORM_COMPLETE_ALL_WORK(name) void name(Platform_Work_Queue *queue)
typedef PLATFORM_COMPLETE_ALL_WORK(Platform_Complete_All_Work);

struct Memory_Arena;

struct Platform_API
{
#if TWOSOME_SLOW
    Debug_Platform_Logging *DEBUG_log_message;
    Debug_Platform_Show_Assert_Popup *DEBUG_show_assert_popup;
#endif
    
#if TWOSOME_INTERNAL
    Debug_Platform_Read_Entire_File *DEBUG_read_entire_file;
    Debug_Platform_Free_File_Memory *DEBUG_free_file_memory;
    Debug_Platform_Write_Entire_File *DEBUG_write_entire_file;
    Debug_Platform_Delete_File *DEBUG_delete_file;    
    Debug_Platform_Count_Files_In_Directory *DEBUG_count_files_in_directory;
    Debug_Platform_Get_Latest_File_Write_Time_In_Directory *DEBUG_get_latest_file_write_in_directory;
    Debug_Platform_Get_Last_File_Write_Time *DEBUG_get_last_file_write_time;
#endif
    
    Platform_Open_File *open_file;
    Platform_Close_File *close_file;
    Platform_Read_Data_From_File *read_data_from_file;
    
    Platform_Write_Entire_File_To_App_Output_Directory *write_entire_file_to_app_output_directory;
    Platform_Read_Entire_File *read_entire_file;
    
    Platform_Load_Game_Save_File *load_game_save_file;
    Platform_Write_Game_Save_File *write_game_save_file;    
    
    Platform_Get_Current_Time *get_current_time;
    
    Platform_Add_Entry *add_entry;
    Platform_Complete_All_Work *complete_all_work;
    
    // NOTE: Options the platform will provide to player (can they quit, fullscreen etc)
    u32 player_platform_option_flags;
    
    char *text_line_ending;
};

#define LOGGING_ENABLED TWOSOME_SLOW

//
// NOTE: Logging
//
#if LOGGING_ENABLED
struct DEBUG_Log_State
{
    b32 initialized;
    char *filename;
    Debug_Platform_Logging *log_message;
    Debug_Platform_Show_Assert_Popup *show_assert_popup;
    Platform_Get_Current_Time *get_current_time;
};

global_variable DEBUG_Log_State DEBUG_log_state;

#define set_global_log_state(platform) {               \
    DEBUG_log_state.log_message = platform.DEBUG_log_message; \
    DEBUG_log_state.show_assert_popup = platform.DEBUG_show_assert_popup; \
    DEBUG_log_state.get_current_time = platform.get_current_time; \
    DEBUG_log_state.initialized = true; \
}

internal void log_message(DEBUG_Log_State *log_state, char *severity, char *src_file, char *function, s32 line, char *message, ...);

#define log_error(message, ...) log_message(&DEBUG_log_state, "ERROR", __FILE__, __FUNCTION__, __LINE__, message, __VA_ARGS__);
#define log_warning(message, ...) log_message(&DEBUG_log_state, "WARNING", __FILE__, __FUNCTION__, __LINE__, message, __VA_ARGS__);
#define log_information(message, ...) log_message(&DEBUG_log_state, "INFORMATION", __FILE__, __FUNCTION__, __LINE__, message, __VA_ARGS__);

#define assert(expression) if(!(expression)) { \
    log_message(&DEBUG_log_state, "ASSERT", __FILE__, __FUNCTION__, __LINE__, "Expression: " #expression); \
    DEBUG_log_state.show_assert_popup("Expression: " #expression); \
}

#else
#define set_global_log_state(platform)
#define log_error(message, ...)
#define log_warning(message, ...)
#define log_information(message, ...)
#define assert(expression)
#endif

#define invalid_code_path assert(!"invalid_code_path")
#define invalid_default_case default: { invalid_code_path; } break;

struct Game_Sound_Output_Buffer
{
    int samples_per_second;
    int sample_count;
    int16 *samples;
};

struct Game_Memory
{
    // NOTE: Required to be cleared to zero at startup
    u32 permanent_storage_size;
    void *permanent_storage;
    // NOTE: Required to be cleared to zero at startup
    u32 transient_storage_size;
    void *transient_storage;
#if TWOSOME_INTERNAL
    // NOTE: Required to be cleared to zero at startup
    u32 debug_storage_size;
    void *debug_storage;
#endif
    
    Platform_Work_Queue *work_queue;
    
    Platform_API platform_api;
};

#include "twosome_math.h"

struct Game_Render_Prep
{
    u32 sorted_render_entry_header_cursors_count;
    Memory_Index *sorted_render_entry_header_cursors;
    
    Memory_Index vertex_soup_data_size;
    Memory_Index last_vertex_soup_render_entry_header_cursor;
};

struct Game_Render_View
{
    s32 x;
    s32 y;
    
    u32 width;
    u32 height;
    
    Mat4 orthographic_matrix;
    Mat4 screen_orthographic_matrix;
};

struct Game_Render_Commands
{
    struct Game_Assets *assets;
    
    u8 *push_buffer_base;
    Memory_Index push_buffer_size;
    
    u8 *push_buffer_at;
    
    u32 render_entries_count;
    
    u32 window_width;
    u32 window_height;
    
    Game_Render_View view;
    Vec3 clear_colour;
    
    Memory_Arena *temp_arena;
};

struct Game_Button_State
{
    int half_transition_count;
    bool32 down;
};

enum Player_Platform_Option_Flags
{
    player_platform_option_fullscreen_flag  = 1 << 0,
    player_platform_option_quit_flag        = 1 << 1
};

struct Game_Input
{
    // NOTE: Buttons
    union
    {
        Game_Button_State buttons[7];
        
        struct
        {    
            union
            {
                struct
                {
                    Game_Button_State change_colour_button;
                    Game_Button_State activate_shield_button;
                };
                struct
                {
                    Game_Button_State ui_interaction_button;
                    Game_Button_State _dud_interaction_button;
                };
            };
            
            Game_Button_State pause_button;
            
#if TWOSOME_INTERNAL
            Game_Button_State DEBUG_restart_stage_button;
            Game_Button_State DEBUG_prev_stage_button;
            Game_Button_State DEBUG_next_stage_button;
            Game_Button_State DEBUG_toggle_mute_button;
#endif
            
            // IMPORTANT: Buttons must all be above this line
            Game_Button_State terminator;            
        };
        
    };
    
    // NOTE: Right and Up are the positive directions
    s32 mouse_x, mouse_y;
    
    f32 dt;
    Game_Settings settings;
};

struct Game_Update_And_Render_Result
{
    b32 quit_game;
    b32 game_restarted;
    b32 toggle_fullscreen;
    u32 game_assets_load_result;
};

#define GAME_UPDATE_AND_RENDER(name) Game_Update_And_Render_Result name(Game_Memory *memory, Game_Input *input, Game_Render_Commands *render_commands)
typedef GAME_UPDATE_AND_RENDER(Game_Update_And_Render);

#define GAME_GET_SOUND_SAMPLES(name) void name(Game_Memory *memory, Game_Sound_Output_Buffer *sound_buffer)
typedef GAME_GET_SOUND_SAMPLES(Game_Get_Sound_Samples);

#if TWOSOME_INTERNAL
#define DEBUG_GAME_FRAME_END(name) void name(Game_Memory *memory)
typedef DEBUG_GAME_FRAME_END(Debug_Game_Frame_End);
#endif

#include "twosome_debug.h"

#if COMPILER_MSVC
#define complete_previous_writes_before_future_writes _WriteBarrier()
#elif COMPILER_LLVM
#define complete_previous_writes_before_future_writes asm volatile("" ::: "memory")
#else
#endif

internal b32 first_button_press(Game_Button_State button)
{
    b32 result = (button.down && button.half_transition_count);
    return result;
}

internal b32 button_release(Game_Button_State button)
{
    b32 result = (!button.down && button.half_transition_count);
    return result;
}

enum Platform_Date_Time_Compare
{
    platform_date_time_compare_first_earlier = -1,
    platform_date_time_compare_equal = 0,
    platform_date_time_compare_first_later = 1
};
internal Platform_Date_Time_Compare compare_platform_date_times(Platform_Date_Time a, Platform_Date_Time b)
{
    Platform_Date_Time_Compare result = platform_date_time_compare_equal;
    
    if(a.year < b.year)
    {
        result = platform_date_time_compare_first_earlier;
    }
    else if(a.year > b.year)
    {
        result = platform_date_time_compare_first_later;
    }
    else
    {
        if(a.month < b.month)
        {
            result = platform_date_time_compare_first_earlier;
        }
        else if(a.month > b.month)
        {
            result = platform_date_time_compare_first_later;
        }
        else
        {
            if(a.day < b.day)
            {
                result = platform_date_time_compare_first_earlier;
            }
            else if(a.day > b.day)
            {
                result = platform_date_time_compare_first_later;
            }
            else
            {
                if(a.hour < b.hour)
                {
                    result = platform_date_time_compare_first_earlier;
                }
                else if(a.hour > b.hour)
                {
                    result = platform_date_time_compare_first_later;
                }
                else
                {
                    if(a.minute < b.minute)
                    {
                        result = platform_date_time_compare_first_earlier;
                    }
                    else if(a.minute > b.minute)
                    {
                        result = platform_date_time_compare_first_later;
                    }
                    else
                    {
                        if(a.second < b.second)
                        {
                            result = platform_date_time_compare_first_earlier;
                        }
                        else if(a.second > b.second)
                        {
                            result = platform_date_time_compare_first_later;
                        }
                        else
                        {
                            if(a.milliseconds < b.milliseconds)
                            {
                                result = platform_date_time_compare_first_earlier;
                            }
                            else if(a.milliseconds > b.milliseconds)
                            {
                                result = platform_date_time_compare_first_later;
                            }
                            else
                            {
                                result = platform_date_time_compare_equal;
                            }
                        }
                    }
                }
            }
        }
    }
    
    return result;
}

enum Flashing_Value_Flags
{
    // NOTE: If don't want to use sin for t
    flashing_value_flag_linear = 1 << 0,
    // NOTE: If don't want to see the flash contracting
    flashing_value_flag_expanding = 1 << 1,
    // NOTE: There's no gap between flashes, jumps back back to 0 when hits 1
    flashing_value_flag_constant_expand = 1 << 2
};

struct Flashing_Value
{
    // IMPORTANT: Use this t variable - which will always be between 0 and 1 - for interpolation.
    f32 t;
    
    f32 _virtual_t;
    b32 _slowed;
    f32 velocity;
    
    // NOTE: This is the time it takes to go 0 to 1 and then back to 0
    f32 time_for_one_flash;
    u32 flags;
    
    b32 _active;
    
    b32 stop_after_current_flash_ends;
};

// NOTE: This doesn't activate the flash from the beginning, it just continues from wherever
// it is (if that be at the start, or from some other point)
internal void activate_flash(Flashing_Value *fv, f32 time_for_one_flash)
{
    fv->time_for_one_flash = time_for_one_flash;
    fv->_active = true;
    
    fv->stop_after_current_flash_ends = false;
}

internal void activate_flash(Flashing_Value *fv)
{
    activate_flash(fv, fv->time_for_one_flash);
}

internal b32 is_flash_active(Flashing_Value *fv)
{
    return fv->_active;
}

internal Flashing_Value make_flashing_value(void)
{
    Flashing_Value result = {};
    result.velocity = 1.0f;
    
    return result;
}

internal Flashing_Value make_flashing_value(f32 time_for_one_flash, b32 active, u32 flags = 0)
{
    Flashing_Value result = make_flashing_value();
    result.time_for_one_flash = time_for_one_flash;
    result.flags = flags;
    
    if(active)
    {
        activate_flash(&result);
    }    
    
    return result;
}

internal void set_time_for_one_flash(Flashing_Value *fv, f32 time_for_one_flash)
{
    fv->time_for_one_flash = time_for_one_flash;
}

internal void stop_flash(Flashing_Value *fv)
{
    fv->t = 0.0f;
    fv->_virtual_t = 0.0f;
    fv->velocity = 1.0f;
    fv->_active = false;
}

internal void restart_flashing_value(Flashing_Value *fv, b32 active = true)
{
    stop_flash(fv);
    activate_flash(fv);
}

internal void sync_flashing_value(Flashing_Value *slave_fv, Flashing_Value *master_fv)
{
    assert(is_flash_active(slave_fv) && is_flash_active(master_fv));    
    
    if( slave_fv->_virtual_t >= (master_fv->_virtual_t - 0.05f) && slave_fv->_virtual_t <= (master_fv->_virtual_t + 0.05f))
    {
        slave_fv->_slowed = false;
    }
    else
    {
        slave_fv->_slowed = true;
    }
}

internal b32 update_flashing_value(Flashing_Value *fv, f32 dt)
{
    b32 flash_start = false;
    
    if(fv->_active)
    {
        f32 time_for_one_flash_k = 0.5f;
        if(fv->flags & flashing_value_flag_expanding)
        {
            // NOTE: If we're only expanding then a total flash is small to big, rather than small to big back to smal, so going small to big should now take the same time as going from small to big, then back to small
            time_for_one_flash_k = 1.0f;
        }
        f32 t_increment = dt * (1.0f / (fv->time_for_one_flash * time_for_one_flash_k));
        // NOTE: We go a little faster if wanting to end flash when flash is fading out
        if(fv->stop_after_current_flash_ends && fv->velocity < 0.0f)
        {
            assert(fv->stop_after_current_flash_ends);
            t_increment *= 1.25f;
        }
        else if(fv->_slowed)
        {
            t_increment *= 0.5f;
        }
        
        // NOTE: If these things are zero then we know we're at the starting point of flash
        if(fv->_virtual_t == 0.0f && fv->velocity >= 0)
        {
            flash_start = true;
        }
        
        fv->_virtual_t += t_increment * fv->velocity;
        if(fv->velocity > 0.0f && fv->_virtual_t >= 1.0f)
        {
            if(fv->flags & flashing_value_flag_constant_expand)
            {
                // NOTE: Skip the moving back to 0 part, just jump back to 0
                fv->_virtual_t = 0.0f;
                fv->velocity = 1.0f;
            }
            else
            {
                fv->_virtual_t = (1.0f - (fv->_virtual_t - 1.0f));
                fv->velocity = -1.0f;       
            }
        }
        else if(fv->velocity < 0.0f && fv->_virtual_t <= 0.0f)
        {
            fv->_virtual_t = (fv->_virtual_t * -1.0f);
            fv->velocity = 1.0f;
            
            if(fv->stop_after_current_flash_ends)
            {
                stop_flash(fv);
            }
            else
            {
                flash_start = true;
            }
        }
        
        if(fv->flags & flashing_value_flag_linear)
        {
            fv->t = fv->_virtual_t;
        }
        else
        {
            // NOTE: We just move back and forth between 0 and 90 degrees
            f32 virtual_t_in_radians = (fv->_virtual_t * (PI*0.5f));
            fv->t = sinf(virtual_t_in_radians);
        }
        
        if((fv->flags & flashing_value_flag_expanding) && fv->velocity < 0.0f)
        {
            fv->t = 1.0f;
        }
    }
    
    assert(fv->t >= 0.0f && fv->t <= 1.0f);
    
    return flash_start;
}

internal void stop_after_current_flash_ends(Flashing_Value *fv)
{
    fv->stop_after_current_flash_ends = true;
}

#endif
