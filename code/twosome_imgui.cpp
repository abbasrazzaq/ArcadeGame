
/*
  NOTE: IMGUI basic logic:
  bool do button(ui_context, id, text, ...)
  {
     if(active)
       if(mouse_went_up)
         if(hot) result = true;
         set_not_active;
     else if(hot)
       if(mouse_went_down) set_active;

     if(inside) set_hot;

     render_button
  }
*/

internal void imgui_frame_start(UI_Context *context, Cursor *cursor, Game_Input *input, f32 time)
{
    context->mouse_x = (s32)cursor->x;
    context->mouse_y = (s32)cursor->y;

    if(!context->mouse_inside_element)
    {
        context->hot = 0;
    }

    context->last_hot_or_active = context->hot;
    if(context->active)
    {
        context->last_hot_or_active = context->active;
    }
    
    context->mouse_is_down = input->ui_interaction_button.down;
    context->mouse_went_up = button_release(input->ui_interaction_button);
    context->time = time;
    
    context->mouse_inside_element = false;

#if TWOSOME_SLOW
    // NOTE: Check that there weren't any duplicate ids
    for(u32 id_record_index = 0; id_record_index < context->id_records_count; ++id_record_index)
    {
        UI_Id id = context->id_records[id_record_index];
        
        for(u32 other_id_record_index = (id_record_index + 1); other_id_record_index < context->id_records_count; ++other_id_record_index)
        {
            if(id_record_index != other_id_record_index)
            {
                UI_Id other_id = context->id_records[other_id_record_index];
                assert(id != other_id);   
            }                        
        }        
    }
    
    context->id_records_count = 0;
#endif
}

internal b32 do_button_logic(UI_Context *context, UI_Id id, b32 cursor_inside_element)
{
    b32 result = false;

    if(context->active == id)
    {
        if(context->mouse_went_up)
        {
            if(context->hot == id)
            {
                result = true;
            }

            context->active = 0;
        }
    }
    else if(context->hot == id)
    {
        if(context->mouse_is_down)
        {
            context->active = id;
        }
    }

    if(cursor_inside_element)
    {
        // NOTE: If someone else is active, then don't go hot
        if(!context->active || context->active == id)
        {
            if(context->hot != id)
            {
                context->time_entered_last_hot = context->time;
            }
            context->hot = id;
        }

        context->mouse_inside_element = true;
    }   

#if TWOSOME_SLOW
    assert(context->id_records_count < array_count(context->id_records));
    context->id_records[context->id_records_count++] = id;
#endif
    
    return result;
}

internal b32 is_cursor_inside_element(UI_Context *context, UI_Id id, Vec2 position, Vec2 scale)
{
    b32 result = (context->mouse_x >= (s32)position.x && context->mouse_x < (s32)(position.x + scale.x) && context->mouse_y >= position.y && context->mouse_y < (position.y + scale.y));

    return result;
}

internal b32 do_button_logic(UI_Context *context, UI_Id id, Vec2 position, Vec2 scale)
{
    b32 result = false;

    b32 cursor_inside_element = is_cursor_inside_element(context, id, position, scale);
    result = do_button_logic(context, id, cursor_inside_element);
    
    return result;
}

internal b32 ui_item_active(UI_Context *context, UI_Id id)
{
    b32 result = (context->active == id && context->hot == id);
    return result;
}

internal b32 ui_item_hot(UI_Context *context, UI_Id id)
{
    b32 result = (context->hot == id || context->active == id);
    return result;
}

internal b32 ui_item_hot_or_active_last_frame(UI_Context *context, UI_Id id)
{
    b32 result = (context->last_hot_or_active == id);
    return result;
}

internal Vec3 select_ui_item_colour(UI_Context *context, UI_Id id, Vec3 idle_colour, Vec3 hot_colour, Vec3 active_colour)
{
    Vec3 colour = idle_colour;
    if(ui_item_active(context, id))
    {
        colour = active_colour;
    }
    else if(ui_item_hot(context, id))
    {
        colour = hot_colour;
    }

    return colour;
}

internal Vec3 get_ui_item_colour(UI_Context *context, UI_Id id)
{
    Vec3 colour = select_ui_item_colour(context, id, vec3(0.0f, 1.0f, 0.0f), vec3(1.0f, 1.0f, 0.0f), vec3(0.5f, 0.5f, 0.0f));
    return colour;
}

internal UI_Id make_ui_id(void *ptr)
{
    UI_Id result = (intptr)ptr;
    return result;
}                                      

internal b32 do_checkbox(UI_Context *context, UI_Id id, Game_Render_Commands *render_commands, b32 checked, Vec2 position, Vec2 scale)
{
    b32 result = false;

    result = do_button_logic(context, id, position, scale);
    
    push_rect(render_commands, position, scale, vec4(get_ui_item_colour(context, id), 1.0f));
    
    if(checked)
    {
        Vec2 inner_scale = scale/2.0f;
        Vec2 inner_position = position + (scale - inner_scale)/2.0f;
        push_rect(render_commands, inner_position, inner_scale, vec4(1.0f, 0.0f, 0.0f, 1.0f));
    }
    
    return result;
}


struct Do_Slider_Logic_Result
{
    b32 pressed;
    f32 cursor_normalized;
    f32 cursor_y;
    f32 step_size;
};

internal Do_Slider_Logic_Result do_slider_logic(UI_Context *context, f32 max, f32 *value, Vec2 position, Vec2 scale, b32 cursor_inside_element, UI_Id id = 0)
{
    Do_Slider_Logic_Result result = {};
    
    if(!id)
    {
        id = (intptr)value;
    }

    result.pressed = do_button_logic(context, id, cursor_inside_element);

    f32 step_size = (scale.y / (max + 1.0f));

    if(ui_item_hot(context, id))
    {
        if(context->mouse_is_down)
        {
            *value = (((f32)context->mouse_y - position.y) / step_size);
            *value = clamp(*value, 0, max);
        }
    }

    result.cursor_y = position.y + (*value * step_size);
    result.cursor_normalized = (*value) / max;
    result.step_size = step_size;
    
    return result;
}

internal Do_Slider_Logic_Result do_slider_logic(UI_Context *context, f32 max, f32 *value, Vec2 position, Vec2 scale, UI_Id id = 0)
{   
    b32 cursor_inside_element = is_cursor_inside_element(context, id, position, scale);

    Do_Slider_Logic_Result result = do_slider_logic(context, max, value, position, scale, cursor_inside_element, id);
    
    return result;
}

internal b32 do_slider(UI_Context *context, r32 max, r32 *value, Vec2 position, Vec2 scale, Game_Render_Commands *render_commands, UI_Id id = 0)
{
    Do_Slider_Logic_Result result = do_slider_logic(context, max, value, position, scale, id);

    push_rect(render_commands, position, scale, vec4(1.0f));

    r32 cursor_y = result.cursor_y;
    r32 cursor_height = result.step_size;
    
    push_rect(render_commands, vec2(position.x, cursor_y), vec2(scale.x, cursor_height), vec4(1.0f, 0.0f, 1.0f, 1.0f));    
    return result.pressed;
}

internal b32 do_slider(UI_Context *context, s32 max, s32 *value, Vec2 position, Vec2 scale, Game_Render_Commands *render_commands)
{
    r32 value_r32 = (r32)(*value);
    r32 max_r32 = (r32)max;
    b32 result = do_slider(context, max_r32, &value_r32, position, scale, render_commands);

    *value = (s32)round(value_r32);    
    
    return result;                           
}

internal void draw_restart_game_icon(Game_Render_Commands *render_commands, Vec2 icon_position, Vec2 icon_scale, f32 flash_t, Vec3 icon_background_colour, Render_Transform transform = default_transform())
{
    Vec2 bin_icon_scale = vec2(icon_scale.x*0.55f, icon_scale.y*0.65f);
    Vec2 floppy_icon_scale = icon_scale*0.25f;

    // NOTE: Bin Icon
    Vec2 bin_icon_position = icon_position + vec2(icon_scale.x*0.5f - bin_icon_scale.x*0.5f, 0.0f);
    {
        push_rect(render_commands, bin_icon_position, bin_icon_scale, vec4(icon_background_colour, 1.0f), transform);

        // NOTE: Bin Lines
        {
            f32 thickness = bin_icon_scale.y*0.05f;
            f32 bottom_spacing = (1.0f / 4.0f) * bin_icon_scale.x;
            
            f32 start_x = bin_icon_position.x;
            f32 spacing = (1.0f / 4.0f) * bin_icon_scale.x;
            f32 ys[3] = { start_x + spacing, start_x + (spacing*2.0f), start_x + (spacing*3.0f) };
            
            f32 y_offset = bin_icon_scale.y*0.175f;
            
            for(int line_index = 0; line_index < 3; ++line_index)
            {
                push_line(render_commands, vec2(ys[line_index], bin_icon_position.y + y_offset), vec4(1.0f, 0.0f, 1.0f, 1.0f), vec2(ys[line_index], bin_icon_position.y + bin_icon_scale.y - y_offset), vec4(1.0f, 0.0f, 1.0f, 1.0f), thickness_transform(thickness, transform));
            }
        }
    }

    // NOTE: Floppy disk icon (on flash it drops into bin and fades out)
    {
        Render_Transform floppy_transform = corner_indent_transform(transform.corner_indent/2.0f, transform);
        
        f32 floppy_alpha = interpolate(1.0f, 0.0f, flash_t);
        Vec2 floppy_corner_icon_scale = floppy_icon_scale*0.35f;
        Vec2 floppy_icon_position = icon_position + vec2(icon_scale.x*0.5f - floppy_icon_scale.x*0.5f, icon_scale.y - floppy_icon_scale.y);
        floppy_icon_position.y -= flash_t * floppy_icon_scale.y;
                                    
        push_rect(render_commands, floppy_icon_position, floppy_icon_scale, vec4(icon_background_colour, floppy_alpha), floppy_transform);
        
        Vec2 floppy_icon_bottom_icon_scale = vec2(floppy_icon_scale.x*0.8f, floppy_icon_scale.y*0.35f);
        f32 edge_offset = ((floppy_icon_scale.y*0.5f - floppy_icon_bottom_icon_scale.y) / 2.0f);
        
        Vec2 floppy_icon_bottom_icon_pos = floppy_icon_position + vec2((floppy_icon_scale.x - floppy_icon_bottom_icon_scale.x) / 2.0f, edge_offset);

        
        Vec2 floppy_corner_icon_pos = floppy_icon_position + floppy_icon_scale - floppy_corner_icon_scale - vec2(floppy_corner_icon_scale.x*0.5f, edge_offset);
        
        push_rect(render_commands, floppy_corner_icon_pos, floppy_corner_icon_scale, vec4(1.0f, 0.0f, 1.0f, floppy_alpha), floppy_transform);
        
        push_rect(render_commands, floppy_icon_bottom_icon_pos, floppy_icon_bottom_icon_scale, vec4(1.0f, 0.0f, 1.0f, floppy_alpha), floppy_transform);   
    }
}

internal void draw_quit_icon(Game_Render_Commands *render_commands, Vec2 position, Vec2 scale, Vec3 colour, f32 arrow_move_t, Vec3 icon_background_colour, Render_Transform transform = default_transform())
{
    Vec2 exit_scale = vec2(scale.x*0.65f, scale.y);
    Vec2 exit_position = vec2(position.x + scale.x - exit_scale.x, position.y);
    
    push_rect(render_commands, exit_position, exit_scale, vec4(icon_background_colour, 1.0f), transform);
    
    Vec2 base_scale = vec2(scale.x*0.5f, scale.y*0.275f);
    f32 icon_x_offset = interpolate(position.x, position.x + exit_scale.x*0.1f, arrow_move_t);
    f32 y_center = position.y + (scale.y/2.0f) - (base_scale.y)/2.0f;
    push_rect(render_commands, vec2(icon_x_offset + transform.corner_indent, y_center), base_scale, vec4(1.0f, 0.0f, 1.0f, 1.0f), transform);
    
#if 0
    Vec2 tip_scale = vec2(scale.x*0.3f, scale.y*0.6f);
    Vec2 tip_verts[] =
    {
        vec2(0.0f),
        vec2(1.0f, 0.5f),
        vec2(0.0f, 1.0f)
    };
    push_vertices(render_commands, (f32 *)tip_verts, array_count(tip_verts), vertex_flag_xy, vec2(icon_x_offset + base_scale.x, position.y + (scale.y/2.0f) - (tip_scale.y)/2.0f), tip_scale, vec4(1.0f, 0.0f, 1.0f, 1.0f), transform);
#else
    Vec2 tip_scale = vec2(scale.y*0.6f, scale.x*0.3f);
    push_triangle(render_commands, vec2(icon_x_offset + base_scale.x, position.y + (scale.y/2.0f)) - vec2(tip_scale.x/4.0f, tip_scale.y/2.0f), tip_scale, vec4(1.0f, 0.0f, 1.0f, 1.0f), rotation_transform(-PI/2.0f, tip_scale/2.0f, transform));
    
#endif

}

internal b32 do_menu_button(s32 menu_button_icon, UI_Context *context, UI_Id id, Game_Render_Commands *render_commands, Vec2 position, Vec2 scale, b32 force_flash = false)
{
    b32 result = do_button_logic(context, id, position, scale);

    Render_Transform shared_transform = draw_order_transform(entity_draw_order_ui, corner_indent_transform(3.0f));
    Vec3 button_colour = get_ui_item_colour(context, id);
    f32 flash_t = 0.0f;
    if(ui_item_hot(context, id) || force_flash)
    {
        f32 elapsed = (context->time - context->time_entered_last_hot) / 0.75f;
        elapsed = clamp(elapsed, 0.0f, PI/4.0f);
        
        flash_t = sinf(elapsed*4.0f);
    }
    push_rect(render_commands, position, scale, vec4(button_colour, 1.0f), shared_transform);
    
    //
    // NOTE: Button Icon
    //
    Vec4 icon_colour = vec4(0.0f, 0.0f, 0.0f, 1.0f);
    Vec2 icon_full_scale = scale*0.75f;
    switch(menu_button_icon)
    {        
        case menu_button_icon_resume_game:
        {
            f32 x_offset = scale.x*0.025f;
            Vec2 icon_scale = icon_full_scale - (x_offset*2.0f);
            
            f32 x = interpolate(position.x - x_offset, position.x + x_offset, flash_t);
            Vec2 icon_position = vec2(x, position.y) + ((scale - icon_scale)/2.0f);

            push_triangle(render_commands, icon_position, icon_scale, icon_colour, rotation_transform(-PI/2.0f, icon_scale/2.0f, corner_indent_transform(shared_transform.corner_indent*2.0f, shared_transform)));
            //push_triangle(render_commands, icon_position, icon_scale, vec4(1.0f, 0.0f, 0.0f, 1.0f), rotation_transform(-PI/2.0f, icon_scale/2.0f, corner_indent_transform(4.0f, shared_transform)));
            
        } break;

        case menu_button_icon_go_fullscreen:
        case menu_button_icon_go_windowed:
        {
            Vec2 border_pos = position;
            Vec2 border_scale = scale;

            Vec2 start_flash_scale = icon_full_scale*0.9f;
            Vec2 end_flash_scale = icon_full_scale;
            // NOTE: Windowed icon goes from big to small
            if(menu_button_icon == menu_button_icon_go_windowed)
            {
                Vec2 temp = start_flash_scale;
                start_flash_scale = end_flash_scale;
                end_flash_scale = temp;
            }
            
            Vec2 target_scale = interpolate(start_flash_scale, end_flash_scale, flash_t);

            Render_Transform arrow_transform = corner_indent_transform(2.0f, shared_transform);
            
            Vec2 target_position = border_pos + ((border_scale - target_scale) / 2.0f);
            push_rect(render_commands, target_position, target_scale, vec4(0.0f, 0.0f, 0.0f, 1.0f), shared_transform);

            f32 arrow_offset = magnitude(target_scale - (icon_full_scale*0.9f)) / 2.0f;

            // NOTE: Draw arrows
            f32 rotations[] = { -PI*0.25f, PI*0.75f };
            //f32 rotations[] = { 0.2f };
            for(u32 arrow_index = 0; arrow_index < array_count(rotations); ++arrow_index)
            {
                f32 rotation = rotations[arrow_index];;

                Vec2 center = target_position + target_scale/2.0f;
                Vec2 arrow_scale = (icon_full_scale*0.9f)/2.0f;
                Vec2 arrow_position = center + vec2(0.0f, 2.5f + arrow_offset);

                if(menu_button_icon == menu_button_icon_go_fullscreen)
                {
                    // NOTE: Draw arrow pointing outwards, by rotating the whole thing around the center
                    Vec2 base_scale = vec2(arrow_scale.x*0.25f, arrow_scale.y*0.575f);
                    Vec2 base_pos = arrow_position - vec2(base_scale.x/2.0f, 0.0f);

                    Vec2 tri_scale = vec2(arrow_scale.x*0.65f, (arrow_scale.y - base_scale.y));
                    Vec2 tri_pos = base_pos + vec2(base_scale.x/2.0f, 0.0f) - vec2(tri_scale.x/2.0f, 0.0f) + vec2(0.0f, base_scale.y);

                    Vec2 rot_pt = center - base_pos;
                    push_rect(render_commands, base_pos, vec2(base_scale.x, base_scale.y + shared_transform.corner_indent), vec4(1.0f, 0.0f, 1.0f, 1.0f), rotation_transform(rotation, rot_pt, arrow_transform));

                    push_triangle(render_commands, tri_pos, tri_scale, vec4(1.0f, 0.0f, 1.0f, 1.0f), world_rotation_transform(rotation, center, arrow_transform));                    
                }
                else if(menu_button_icon == menu_button_icon_go_windowed)
                {
                    // NOTE: Draw arrow pointing inwards, , by rotating the whole thing around the center, and flip the arrow in
                    Vec2 base_scale = vec2(arrow_scale.x*0.25f, arrow_scale.y*0.575f);
                    Vec2 tri_scale = vec2(arrow_scale.x*0.65f, arrow_scale.y - base_scale.y);
        
                    Vec2 tri_pos = arrow_position - vec2(tri_scale.x/2.0f, 0.0f);
                    Vec2 base_pos = tri_pos + vec2(tri_scale.x/2.0f, 0.0f) - vec2(base_scale.x/2.0f, 0.0f) + vec2(0.0f, tri_scale.y) - vec2(0.0f, shared_transform.corner_indent);

                    Vec2 rot_pt = center - base_pos;
                    push_rect(render_commands, base_pos, vec2(base_scale.x, base_scale.y + shared_transform.corner_indent), vec4(1.0f, 0.0f, 1.0f, 1.0f), rotation_transform(rotation, rot_pt, arrow_transform));

                    // NOTE: We're going to have to flip the arrow too in local space
                    Render_Transform tri_transform = world_rotation_transform(rotation, center, rotation_transform(PI, tri_scale/2.0f, arrow_transform));

                    push_triangle(render_commands, tri_pos, tri_scale, vec4(1.0f, 0.0f, 1.0f, 1.0f), tri_transform);
                    
                }
                else
                {
                    invalid_code_path;
                }
            }            

        } break;
        case menu_button_icon_quit_game:
        case menu_button_icon_quit_level:
        {
            draw_quit_icon(render_commands, position + (scale - icon_full_scale)/2.0f, icon_full_scale, icon_colour.rgb, flash_t, vec3(0.0f), shared_transform);
        } break;
        
#if 0
        case menu_button_icon_quit_level:
        {            
            Vec2 icon_position = position + (scale - icon_full_scale)/2.0f;
            Vec2 icon_scale = icon_full_scale;
            f32 face_size_y = icon_full_scale.y*0.95f;
            Vec2 face_scale = vec2(face_size_y*0.5f, face_size_y);

            {                
                Girl_Face face = {};
                face.scale = face_scale;
                face.position = icon_position + vec2(icon_full_scale.x - face_scale.x, icon_full_scale.y- face_scale.y);
#if 0
                {
                    Vec2 exit_scale = vec2(icon_full_scale.x*0.65f, icon_full_scale.y);
                    Vec2 exit_position = vec2(icon_position.x + icon_full_scale.x - exit_scale.x, icon_position.y);
                    push_rect(render_commands, exit_position, exit_scale, vec4(0.0f, 0.0f, 0.0f, 1.0f));
                }

                face.position.x -= 5.0f;
                face.position.y -= 2.5f;
#endif
                
                draw_girl_face(&face, render_commands, 0.0f, 2.0f);
            }
            {
                Vec2 base_scale = vec2(scale.x*0.5f*0.75f, scale.y*0.275f*0.75f);
                f32 icon_x_offset = interpolate(icon_position.x, icon_position.x + face_scale.x*0.1f, flash_t);
                f32 y_center = position.y + (scale.y/2.0f) - (base_scale.y)/2.0f;
                push_rect(render_commands, vec2(icon_x_offset, y_center), base_scale, vec4(1.0f, 0.0f, 1.0f, 1.0f));
    
                Vec2 tip_verts[] =
                  {
                      vec2(0.0f),
                      vec2(1.0f, 0.5f),
                      vec2(0.0f, 1.0f)
                  };
                Vec2 tip_scale = vec2(scale.x*0.3f*0.75f, scale.y*0.6f*0.75f);
                push_vertices(render_commands, (f32 *)tip_verts, array_count(tip_verts), vertex_flag_xy, vec2(icon_x_offset + base_scale.x, position.y + (scale.y/2.0f) - (tip_scale.y)/2.0f), tip_scale, vec4(1.0f, 0.0f, 1.0f, 1.0f));
            }

        } break;
#endif
        
        case menu_button_icon_yes_confirm:
        {
            f32 thickness = icon_full_scale.x*0.2f;
            Vec2 top_scale = vec2(icon_full_scale.x*0.76f, thickness);
            Vec2 bottom_scale = vec2(icon_full_scale.x*0.44f, thickness);            
                        
            Vec2 bottom_offset = (scale * 0.25f);
            Vec2 top_offset = bottom_offset;//scale * 0.25f;
            Vec2 up_right_dir = vec2(1.0f);
            Vec2 bottom_right_dir = vec2(1.0f, -1.0f);
            
            Vec2 bottom_position = (position - vec2(bottom_scale.x/2.0f, 0.0f)) + vec2(bottom_offset.x*up_right_dir.x, bottom_offset.y*up_right_dir.y);            
            Vec2 top_position = position + vec2(top_offset.x*up_right_dir.x, top_offset.y*up_right_dir.y);
            top_position += vec2(bottom_right_dir.x*bottom_scale.x, bottom_right_dir.y*bottom_scale.y) * 0.5f;

            f32 center_y_offset = icon_full_scale.y*0.125f;
            bottom_position.y += center_y_offset;
            top_position.y += center_y_offset;
            
            f32 rotation_offset = interpolate(PI/4.0f, PI/4.5f, flash_t);
            
            push_rect(render_commands, top_position, top_scale, vec4(0.0f, 0.0f, 0.0f, 1.0f), rotation_transform(rotation_offset - PI*0.01f, vec2(0.0f), shared_transform));
            
            push_rect(render_commands, bottom_position, bottom_scale, vec4(0.0f, 0.0f, 0.0f, 1.0f), rotation_transform(-rotation_offset, vec2(bottom_scale.x/2.0f, 0.0f), shared_transform));

        } break;
        
        case menu_button_icon_no_confirm:
        {
            Vec2 cross_scale = vec2(icon_full_scale.x*1.0f, icon_full_scale.x*0.2f);
            Vec2 cross_position = position + (scale - cross_scale)/2.0f;
            f32 rotation_offset = interpolate(PI/4.0f, PI/4.75f, flash_t);
            
            push_rect(render_commands, cross_position, cross_scale, vec4(0.0f, 0.0f, 0.0f, 1.0f), rotation_transform(rotation_offset, cross_scale/2.0f, shared_transform));
            push_rect(render_commands, cross_position, cross_scale, vec4(0.0f, 0.0f, 0.0f, 1.0f), rotation_transform(-rotation_offset, cross_scale/2.0f, shared_transform));
        } break;

        case menu_button_icon_restart_game:
        {
            draw_restart_game_icon(render_commands, position + (scale - icon_full_scale)/2.0f, icon_full_scale, flash_t, vec3(0.0f), corner_indent_transform(2.0f, shared_transform));
        } break;

        case menu_button_icon_volume_speaker:
        case menu_button_icon_volume_speaker_muted:
        {
            Vec2 icon_position = position + (scale - icon_full_scale) / 2.0f;
            
            Vec2 noise_maker_scale = vec2(icon_full_scale.x*0.5f, icon_full_scale.y);
            Vec2 noise_maker_pos = icon_position;

            Vec3 noise_maker_colour = interpolate(vec3(0.0f), vec3(1.0f, 0.0f, 1.0f), flash_t);

            // NOTE: Speaker
            {
                Vec2 actual_noise_maker_scale = vec2(noise_maker_scale.y, noise_maker_scale.x);
                Vec2 actual_noise_maker_pos = noise_maker_pos + vec2(-actual_noise_maker_scale.x/4.0f, actual_noise_maker_scale.y/2.0f);
                push_triangle(render_commands, actual_noise_maker_pos, actual_noise_maker_scale, vec4(noise_maker_colour, 1.0f), rotation_transform(PI/2.0f, actual_noise_maker_scale/2.0f, shared_transform));
            }

            // NOTE: Speaker "Waves"
            {
                const int num_speaker_waves = 3;
                f32 total_speaker_waves_width = (icon_full_scale.x*0.5f);
                
                f32 small_height = icon_full_scale.y*0.5f;
                f32 large_height = icon_full_scale.y;

                f32 heights[num_speaker_waves] =
                {
                    interpolate(small_height, (small_height + (large_height - small_height)/2.0f), flash_t),
                    interpolate((small_height + (large_height - small_height)/2.0f), large_height, flash_t),
                    interpolate(large_height, (small_height + (large_height - small_height)/2.0f), flash_t),
                };                
                
                f32 width = (total_speaker_waves_width / 6.0f);
                
                Vec2 bar_pos = vec2(icon_position.x + noise_maker_scale.x + width, icon_position.y);
                Vec3 wave_colour = vec3(0.0f);
                for(int wave_bar_index = 0; wave_bar_index < num_speaker_waves; ++wave_bar_index)
                {
                    assert(wave_bar_index < array_count(heights));                    
                    f32 height = heights[wave_bar_index];
                    
                    f32 y = bar_pos.y + (large_height - height) / 2.0f;
                    push_rect(render_commands, vec2(bar_pos.x, y), vec2(width, height), vec4(0.0f, 0.0f, 0.0f, 1.0f), shared_transform);
                    bar_pos.x += width*2.0f;
                }
            }

            // NOTE: Draw slash through icon if we're muted
            if(menu_button_icon == menu_button_icon_volume_speaker_muted)
            {
                push_line(render_commands, icon_position, vec4(1.0f, 0.0f, 1.0f, 1.0f), icon_position + icon_full_scale, vec4(1.0f, 0.0f, 1.0f, 1.0f), thickness_transform(3.0f, shared_transform));
            }
                        
        } break;
        
        invalid_default_case;
    };

#if TWOSOME_INTERNAL && 0
    push_line(render_commands, position, position + scale, vec4(1.0f));
    push_line(render_commands, position + vec2(0.0f, scale.y), position + vec2(scale.x, 0.0f), vec4(1.0f));    
    push_rect_outline(render_commands, position + (scale - icon_full_scale)/2.0f, icon_full_scale, vec4(1.0f));
#endif

    return result;
}
