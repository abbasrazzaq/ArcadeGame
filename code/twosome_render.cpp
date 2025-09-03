#include "twosome_render_group.h"

struct Render_Sort_Entry
{
    s32 draw_order;
    Memory_Index header_buffer_cursor;    
    u32 render_entry_type;
    Memory_Index entry_extra_bytes;
};

internal Game_Render_Prep sort_render_entries(Game_Render_Commands *commands, Memory_Arena *temp_arena)
{
    Game_Render_Prep prep = {};
    prep.sorted_render_entry_header_cursors_count = commands->render_entries_count;
    prep.sorted_render_entry_header_cursors = push_array(temp_arena, Memory_Index, prep.sorted_render_entry_header_cursors_count);

    Temporary_Memory temp_mem = begin_temporary_memory(temp_arena);
    {    
        Render_Sort_Entry *sort_entries = push_array(temp_arena, Render_Sort_Entry, prep.sorted_render_entry_header_cursors_count);

        // NOTE: Create sort entries list
        {
            u8 *push_buffer_end = (commands->push_buffer_base + commands->push_buffer_size);
            u8 *push_buffer = push_buffer_end;
        
            for(u32 sorted_index = 0; sorted_index < commands->render_entries_count; ++sorted_index)
            {
                push_buffer -= sizeof(Render_Entry_Header);
                Render_Entry_Header *header = (Render_Entry_Header *)push_buffer;

                Render_Sort_Entry *sort_entry = &sort_entries[sorted_index];
                sort_entry->draw_order = header->draw_order;
                sort_entry->header_buffer_cursor = (push_buffer_end - push_buffer);
                sort_entry->render_entry_type = header->type;
                sort_entry->entry_extra_bytes = header->entry_extra_bytes;

                push_buffer -= header->entry_size;
            }
        }

        if(commands->render_entries_count > 0)
        {
            Render_Sort_Entry *work_buffer = push_array(temp_arena, Render_Sort_Entry, prep.sorted_render_entry_header_cursors_count);

            // NOTE: Merge sort render entries
            Render_Sort_Entry *a = sort_entries;
            Render_Sort_Entry *b = work_buffer;
            
            u32 entries_count = commands->render_entries_count;
            for(u32 sublist_width = 1; sublist_width < entries_count; sublist_width = sublist_width * 2)
            {
                for(u32 sublist_index = 0; sublist_index < entries_count; sublist_index = sublist_index + (2 * sublist_width))
                {
                    u32 left_start = sublist_index;
                    u32 right_start = min(sublist_index + sublist_width, entries_count);
                    u32 right_end = min(sublist_index + (2 * sublist_width), entries_count);

                    u32 left_index = left_start;
                    u32 right_index = right_start;
                    for(u32 i = left_start; i < right_end; ++i)
                    {
                        if(left_index < right_start && (right_index >= right_end || a[left_index].draw_order <= a[right_index].draw_order))
                        {
                            b[i] = a[left_index];
                            ++left_index;
                        }
                        else
                        {
                            b[i] = a[right_index];
                            ++right_index;
                        }
                    }
                }

                swap(Render_Sort_Entry*, a, b);
            }

            // NOTE: A is the sorted list
            sort_entries = a;            
        }
        
        // NOTE: Copy sorted indices back in to main list
        for(u32 sorted_index = 0; sorted_index < commands->render_entries_count; ++sorted_index)
        {
            u32 header_buffer_cursor = sort_entries[sorted_index].header_buffer_cursor;
            prep.sorted_render_entry_header_cursors[sorted_index] = header_buffer_cursor;
            
            // NOTE: Add vertices to vertex soup
            if(sort_entries[sorted_index].render_entry_type == render_entry_type_Render_Entry_Vertices)
            {
                prep.vertex_soup_data_size += sort_entries[sorted_index].entry_extra_bytes;
                prep.last_vertex_soup_render_entry_header_cursor = header_buffer_cursor;
            }
        }
    
    }
    end_temporary_memory(temp_mem);
    
    return prep;
}
