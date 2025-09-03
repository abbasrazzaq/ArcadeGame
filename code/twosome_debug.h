#ifndef TWOSOME_DEBUG_H
#define TWOSOME_DEBUG_H

#if TWOSOME_INTERNAL

struct Debug_Counter_Snapshot
{
    u32 hit_count;
    u32 cycle_count;
};

#define debug_snapshot_count 120
struct Debug_Counter_State
{
    char *filename;
    char *function_name;

    u32 line_number;

    Debug_Counter_Snapshot snapshots[debug_snapshot_count];
};

struct Debug_Memory_Arena_Tracker
{
    Memory_Arena *arena;
    Memory_Index max_temporary_memory_end;
    char name[64];
};

struct Debug_Buffer_Usage_Tracker
{
    char name[64];
    void *ptr;
    Memory_Index size;
    Memory_Index max_used;
};

struct Debug_Frame_Timestamp
{
    char *name;
    f32 time;
};

struct Debug_Frame_End_Info
{
    u32 timestamp_count;
    Debug_Frame_Timestamp timestamps[64];
};

struct Debug_State
{
    u32 snapshot_index;
    
    Debug_Counter_State counter_states[512];
    Debug_Frame_End_Info frame_end_infos[debug_snapshot_count];

    Debug_Memory_Arena_Tracker memory_arena_trackers[12];
    u32 memory_arena_trackers_count;

    Debug_Buffer_Usage_Tracker buffer_usage_trackers[8];
    u32 buffer_usage_trackers_count;

    b32 show_counter_states;
    b32 show_memory_trackers;
    b32 show_frame_end_infos;
    b32 showing_all_information;    
};


global_variable Debug_State *global_debug_state;

#define set_global_debug_state(game_memory) { global_debug_state = (Debug_State *)((game_memory)->debug_storage); } 

#define TIMED_BLOCK__(number, ...) Timed_Block timed_block_##number(__COUNTER__, __FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__)
#define TIMED_BLOCK_(number, ...) TIMED_BLOCK__(number, ##__VA_ARGS__)
#define TIMED_BLOCK(...) TIMED_BLOCK_(__LINE__, ##__VA_ARGS__)

internal void _track_memory_arena(Memory_Arena *arena, char *name)
{
    b32 already_tracking = false;
    for(u32 memory_arena_tracker_index = 0; memory_arena_tracker_index < global_debug_state->memory_arena_trackers_count; ++memory_arena_tracker_index)
    {
        if(global_debug_state->memory_arena_trackers[memory_arena_tracker_index].arena == arena)
        {
            already_tracking = true;
            break;
        }
    }

    if(!already_tracking)
    {
        Debug_Memory_Arena_Tracker tracker = {};
        tracker.arena = arena;
        strcpy(tracker.name, name);
        assert(global_debug_state->memory_arena_trackers_count < array_count(global_debug_state->memory_arena_trackers));
        global_debug_state->memory_arena_trackers[global_debug_state->memory_arena_trackers_count++] = tracker;
    }
}

#define TRACK_MEMORY_ARENA(arena_to_track) _track_memory_arena(&arena_to_track, #arena_to_track);

internal void _track_buffer_usage(void *buffer_ptr, char *buffer_name, Memory_Index buffer_size)
{
    b32 already_tracking = false;
    
    for(u32 buffer_usage_tracker_index = 0; buffer_usage_tracker_index < global_debug_state->buffer_usage_trackers_count; ++buffer_usage_tracker_index)
    {
        if(global_debug_state->buffer_usage_trackers[buffer_usage_tracker_index].ptr == buffer_ptr)
        {
            already_tracking = true;
        }
    }

    if(!already_tracking)
    {
        Debug_Buffer_Usage_Tracker tracker = {};
        strcpy(tracker.name, buffer_name);
    
        tracker.ptr = buffer_ptr;
        tracker.size = buffer_size;
        assert(global_debug_state->buffer_usage_trackers_count < array_count(global_debug_state->buffer_usage_trackers));
        global_debug_state->buffer_usage_trackers[global_debug_state->buffer_usage_trackers_count++] = tracker;   
    }
}

#define TRACK_BUFFER_USAGE(buffer_to_track, buffer_to_track_size) _track_buffer_usage(buffer_to_track, #buffer_to_track, buffer_to_track_size)

internal void track_temporary_memory_usage(void *arena_ptr, Memory_Index end)
{
    b32 found_arena = false;
    
    for(u32 memory_arena_tracker_index = 0; memory_arena_tracker_index < global_debug_state->memory_arena_trackers_count; ++memory_arena_tracker_index)
    {
        Debug_Memory_Arena_Tracker *tracker = global_debug_state->memory_arena_trackers + memory_arena_tracker_index;

        if(arena_ptr == tracker->arena)
        {
            if(end > tracker->max_temporary_memory_end)
            {
                tracker->max_temporary_memory_end = end;
            }
            
            found_arena = true;
            break;
        }
    }

    assert(found_arena);
}

#define TRACK_TEMPORARY_MEMORY_USAGE(temp_mem) track_temporary_memory_usage(temp_mem.arena, temp_mem.arena->used)

internal void sample_buffer_usage(void *ptr, Memory_Index used)
{
    b32 found_buffer = false;

    assert(global_debug_state->buffer_usage_trackers_count < array_count(global_debug_state->buffer_usage_trackers));
    for(u32 buffer_usage_tracker_index = 0; buffer_usage_tracker_index < global_debug_state->buffer_usage_trackers_count; ++buffer_usage_tracker_index)
    {
        Debug_Buffer_Usage_Tracker *tracker = global_debug_state->buffer_usage_trackers + buffer_usage_tracker_index;
        if(tracker->ptr == ptr)
        {
            if(used > tracker->max_used)
            {
                tracker->max_used = used;
            }
            found_buffer = true;
            break;   
        }
    }
    
    assert(found_buffer);
}

#define SAMPLE_BUFFER_USAGE(buffer, used) sample_buffer_usage(buffer, used)

struct Timed_Block
{
    Debug_Counter_Snapshot *debug_counter;
    u64 start_cycles;
    u32 hit_count;

    Timed_Block(int counter, char *filename, int line_number, char *function_name, u32 hit_count_init = 1)
    {
        hit_count = hit_count_init;
        
        Debug_Counter_State *dest = &global_debug_state->counter_states[counter];
        dest->filename = filename;
        dest->line_number = line_number;
        dest->function_name = function_name;

        debug_counter = &dest->snapshots[global_debug_state->snapshot_index];
        
        start_cycles = __rdtsc();
    }

    ~Timed_Block()
    {
        debug_counter->hit_count += hit_count;
        debug_counter->cycle_count += (u32)(__rdtsc() - start_cycles);
    }
};

internal void DEBUG_overlay(Game_Memory *, Game_Render_Commands *, struct UI_Context *);

#else

#define TIMED_BLOCK(...)
#define TRACK_MEMORY_ARENA(arena)
#define TRACK_TEMPORARY_MEMORY_USAGE(arena)
#define TRACK_BUFFER_USAGE(buffer, size)
#define set_global_debug_state(game_memory)
#define SAMPLE_BUFFER_USAGE(buffer, used)

#endif // TWOSOME_INTERNAL

#endif

