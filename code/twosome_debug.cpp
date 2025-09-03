#include "twosome_debug.h"

#if TWOSOME_INTERNAL

internal void DEBUG_overlay(Game_Memory *memory, Game_Render_Commands *render_commands, UI_Context *ui_context)
{
    assert(sizeof(Debug_State) <= memory->debug_storage_size);
    Debug_State *debug_state = (Debug_State *)memory->debug_storage;
    if(debug_state)
    {
        // NOTE: Do ui for switching things on
        {
            Vec2 size = vec2(10.0f);
            f32 offset = size.x + (size.x*1.5f);
            f32 start_x = virtual_screen_width - size.x;
            f32 start_y = virtual_screen_center_y;
            f32 x = start_x;
            f32 y = start_y;
            
            if(do_checkbox(ui_context, make_ui_id(&debug_state->show_counter_states), render_commands, debug_state->show_counter_states, vec2(x, y), size))
            {
                debug_state->show_counter_states = !debug_state->show_counter_states;
            }
            y += offset;
            if(do_checkbox(ui_context, make_ui_id(&debug_state->show_memory_trackers), render_commands, debug_state->show_memory_trackers, vec2(x, y), size))
            {
                debug_state->show_memory_trackers = !debug_state->show_memory_trackers;
            }
            y += offset;
            if(do_checkbox(ui_context, make_ui_id(&debug_state->show_frame_end_infos), render_commands, debug_state->show_frame_end_infos, vec2(x, y), size))
            {
                debug_state->show_frame_end_infos = !debug_state->show_frame_end_infos;
            }

            if(do_checkbox(ui_context, make_ui_id(&debug_state->showing_all_information), render_commands, debug_state->showing_all_information, vec2(x - offset, start_y), vec2(size.x, y - start_y)))
            {
                debug_state->showing_all_information = !debug_state->showing_all_information;
                
                debug_state->show_counter_states = debug_state->showing_all_information;
                debug_state->show_memory_trackers = debug_state->showing_all_information;
                debug_state->show_frame_end_infos = debug_state->showing_all_information;
            }
        }                
        
        int x = 0;
        int y = 10;
        f32 font_size = 18.0f;
        int line_spacing = (int)(font_size*1.5f);
        Vec4 colour = vec4(1.0f, 0.45f, 0.23f, 1.0f);
        
        if(debug_state->show_counter_states)
        {
            for(u32 counter_state_index = 0; counter_state_index < array_count(debug_state->counter_states); ++counter_state_index)
            {
                Debug_Counter_State *counter_state = debug_state->counter_states + counter_state_index;            

                u32 total_hit_count = 0;
                u64 total_cycle_count = 0;
                u32 snapshot_count = 0;
                for(u32 snapshot_index = 0; snapshot_index < array_count(counter_state->snapshots); ++snapshot_index)
                {
                    Debug_Counter_Snapshot *snap = counter_state->snapshots + snapshot_index;
                
                    if(snap->hit_count > 0)
                    {
                        total_hit_count += snap->hit_count;
                        total_cycle_count += snap->cycle_count;
                        ++snapshot_count;
                    }
                }
            
                if(total_hit_count > 0)
                {
                    u32 avg_hit_count = (u32)((f32)total_hit_count / (f32)snapshot_count);
                    u32 avg_cycle_count = (u32)((f64)total_cycle_count / (f64)snapshot_count);

                    char buffer[512];
                    int tx = x;

                    sprintf(buffer, "%s [%u]", counter_state->function_name, counter_state->line_number);
                    DEBUG_push_text(render_commands, buffer, font_size, tx, y, colour);

                    tx += 400;
                
                    sprintf(buffer, "%u cycles", avg_cycle_count);
                    DEBUG_push_text(render_commands, buffer, font_size, tx, y, colour);

                    tx += 150;
                
                    sprintf(buffer, "%u hits", avg_hit_count);
                    DEBUG_push_text(render_commands, buffer, font_size, tx, y, colour);

                    tx += 100;
                
                    u32 avg_cycle_per_hit = (u32)((f64)total_cycle_count / (f64)total_hit_count);
                    sprintf(buffer, "%u cycle/h", avg_cycle_per_hit);
                    DEBUG_push_text(render_commands, buffer, font_size, tx, y, colour);
                
                    y += line_spacing;
                }
            
            }   
        }       

        if(debug_state->show_frame_end_infos && debug_state->snapshot_index > 0)
        {
            Debug_Frame_End_Info *frame_end_info = &debug_state->frame_end_infos[debug_state->snapshot_index - 1];
            int y = (int)(virtual_screen_height - font_size*2);

            for(u32 timestamp_index = 0; timestamp_index < frame_end_info->timestamp_count; ++timestamp_index)
            {
                Debug_Frame_Timestamp *timestamp = &frame_end_info->timestamps[timestamp_index];

                char buffer[128];
                sprintf(buffer, "%s: %f", timestamp->name, timestamp->time);
                DEBUG_push_text(render_commands, buffer, font_size, 50, y, colour);

                y -= line_spacing;
            }
        }
        
        //
        // NOTE: Memory Arena & Buffer Usage
        //
        if(debug_state->show_memory_trackers)
        {
            int x = 400;
            int y = (int)(virtual_screen_height - font_size*2);

            char buffer[128];

            for(u32 memory_arena_tracker_index = 0; memory_arena_tracker_index < debug_state->memory_arena_trackers_count; ++memory_arena_tracker_index)
            {
                Debug_Memory_Arena_Tracker *tracker = &debug_state->memory_arena_trackers[memory_arena_tracker_index];
                
                Memory_Arena *arena = tracker->arena;
                sprintf(buffer, "%s: %.3f/%.2f MB", tracker->name, bytes_to_megabytes(arena->used), bytes_to_megabytes(arena->size) );

                DEBUG_push_text(render_commands, buffer, font_size, x, y, colour);

                y -= line_spacing;

                if(tracker->max_temporary_memory_end > 0)
                {
                    sprintf(buffer, "Temp Mem Max: %.4f MB", bytes_to_megabytes(tracker->max_temporary_memory_end));
                    DEBUG_push_text(render_commands, buffer, font_size, x + 10, y, colour);

                    y -= line_spacing;                    
                }
            }

            for(u32 buffer_usage_tracker_index = 0; buffer_usage_tracker_index < debug_state->buffer_usage_trackers_count; ++buffer_usage_tracker_index)
            {
                Debug_Buffer_Usage_Tracker *tracker = debug_state->buffer_usage_trackers + buffer_usage_tracker_index;
                sprintf(buffer, "%s: %.3f MB/%.2f MB", tracker->name, bytes_to_megabytes(tracker->max_used), bytes_to_megabytes(tracker->size));

                DEBUG_push_text(render_commands, buffer, font_size, x, y, colour);

                y -= line_spacing;
                
            }
        }
        
    }
}


extern "C" DEBUG_GAME_FRAME_END(DEBUG_game_frame_end)
{
    Debug_State *debug_state = (Debug_State *)memory->debug_storage;
    if(debug_state)
    {
        ++debug_state->snapshot_index;
        if(debug_state->snapshot_index >= debug_snapshot_count)
        {
            debug_state->snapshot_index = 0;
        }

        // NOTE: Clear out next counter for next tick
        for(u32 counter_index = 0; counter_index < array_count(debug_state->counter_states); ++counter_index)
        {
            Debug_Counter_State *counter = &debug_state->counter_states[counter_index];
            Debug_Counter_Snapshot *snapshot = &counter->snapshots[debug_state->snapshot_index];
            snapshot->hit_count = 0;
            snapshot->cycle_count = 0;
        }

        // NOTE: Clear out frame end infos
        Debug_Frame_End_Info *info = debug_state->frame_end_infos + debug_state->snapshot_index;
        zero_object(Debug_Frame_End_Info, *info);
        
    }
    
}

#endif
