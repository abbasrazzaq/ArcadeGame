#ifndef TWOSOME_MEMORY_H
#define TWOSOME_MEMORY_H


struct Memory_Arena
{
    Memory_Index size;
    uint8 *base;
    Memory_Index used;

    int32 temp_count;
};

struct Temporary_Memory
{
    Memory_Arena *arena;
    Memory_Index used;
};


enum Arena_Push_Flag
{
    arena_flag_clear_to_zero = 1 << 0,
};

struct Arena_Push_Params
{
    u32 flags;
    u32 alignment;
};

internal Arena_Push_Params default_arena_params(void)
{
    Arena_Push_Params params;
    params.flags = 0;
    params.alignment = 4;

    return params;
}

internal Arena_Push_Params align_no_clear(u32 alignment)
{
    Arena_Push_Params params = default_arena_params();
    params.alignment = alignment;

    return params;
}

internal Arena_Push_Params clear_to_zero(void)
{
    Arena_Push_Params params = default_arena_params();
    params.flags |= arena_flag_clear_to_zero;

    return params;
}

#define push_struct(arena, type, ...) (type *)push_size_(arena, sizeof(type), ##__VA_ARGS__)
#define push_array(arena, type, count, ...) (type *) push_size_(arena, (count)*sizeof(type), ##__VA_ARGS__)
#define push_size(arena, size, ...) push_size_(arena, size, ##__VA_ARGS__)

internal void *push_size_(Memory_Arena *arena, Memory_Index size_requested, Arena_Push_Params params = default_arena_params())
{
    Memory_Index alignment_offset = 0;
    Memory_Index result_pointer = (Memory_Index)(arena->base + arena->used);
    Memory_Index alignment_mask = (params.alignment - 1);
    if(result_pointer & alignment_mask)
    {
        alignment_offset = params.alignment - (result_pointer & alignment_mask);
    }

    Memory_Index size = size_requested + alignment_offset;
    assert((arena->used + size) <= arena->size);
    arena->used += size;
    
    void *result = (void *)(result_pointer + alignment_offset);

    if(params.flags & arena_flag_clear_to_zero)
    {
        zero_buffer(result, size);
    }
    
    return result;
}

internal void initialize_arena(Memory_Arena *arena, Memory_Index size, void *base)
{
    arena->size = size;
    arena->base = (u8 *)base;
    arena->used = 0;
    arena->temp_count = 0;
}

internal Temporary_Memory begin_temporary_memory(Memory_Arena *arena)
{
    Temporary_Memory result;

    result.arena = arena;
    result.used = arena->used;

    ++arena->temp_count;

    return result;
}

internal void end_temporary_memory(Temporary_Memory temp_mem)
{
    Memory_Arena *arena = temp_mem.arena;
    assert(arena->used >= temp_mem.used);

    TRACK_TEMPORARY_MEMORY_USAGE(temp_mem);
    
    arena->used = temp_mem.used;
    
    assert(arena->temp_count > 0);
    --arena->temp_count;
}

internal void clear(Memory_Arena *arena)
{
    initialize_arena(arena, arena->size, arena->base);
}

internal void check_arena(Memory_Arena *arena)
{
    assert(arena->temp_count == 0);
}

internal void sub_arena(Memory_Arena *result, Memory_Arena *arena, Memory_Index size)
{
    result->size = size;
    result->base = (u8 *)push_size_(arena, size);
    result->used = 0;
    result->temp_count = 0;
}

internal Memory_Index get_arena_size_remaining(Memory_Arena *arena)
{
    Memory_Index actual_size_left = arena->size - arena->used;    
    Memory_Index aligned_size_left = (actual_size_left & ~3);
    
    return aligned_size_left;
}

#endif
