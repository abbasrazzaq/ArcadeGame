#include "twosome_pause_screen.h"


#define pause_screen_event_sound_id asset_level_stage_transition
#define pause_menu_screen_edge_offset menu_button_half_size/2.0f


internal Volume_Slider get_volume_slider()
{
    Volume_Slider vs;

    f32 border_offset = 35.0f;
    f32 width = 40.0f;

    vs.button_position = vec2((virtual_screen_width - width) - border_offset/2.0f - pause_menu_screen_edge_offset, pause_menu_screen_edge_offset + border_offset/2.0f);
    vs.button_scale = vec2(width);
    vs.slider_scale = vec2(width, width*6.0f);
    vs.cursor_scale = vec2(vs.slider_scale.x*0.7f);

    f32 button_slider_offset = vs.cursor_scale.y*0.75f;     

    f32 button_and_slider_height = vs.button_scale.y + vs.slider_scale.y + button_slider_offset;    
    Vec2 button_and_slider_scale = vec2(vs.slider_scale.x, button_and_slider_height);    

    vs.border_scale = button_and_slider_scale + border_offset;
    vs.border_position = vs.button_position - (vs.border_scale - button_and_slider_scale)/2.0f;

    vs.slider_bar_scale = vec2(vs.slider_scale.x * 0.25f, vs.slider_scale.y);
    vs.slider_bar_mark_max_width = vs.slider_scale.x * 0.9f;
    vs.slider_bar_mark_height = vs.slider_bar_scale.y*0.015f;
    
    vs.slider_bar_pos = vec2(vs.button_position.x + ((vs.slider_scale.x - vs.slider_bar_mark_max_width)/2.0f), vs.button_position.y + vs.button_scale.y) + vec2(0.0f, button_slider_offset);

    return vs;
};

internal f32 get_volume_slider_cursor_y(Volume_Slider vs, f32 cursor_pos)
{
    f32 result = vs.slider_bar_pos.y + vs.slider_bar_scale.y*cursor_pos - vs.cursor_scale.y*0.5f;
    return result;
}

internal f32 get_pause_screen_event_sound_volume(f32 pause_screen_master_volume_setting)
{
    f32 result = 0.5f*pause_screen_master_volume_setting;
    return result;
}

internal void play_pause_screen_event_sound(Pause_Screen *pause_screen, Game_Audio *audio)
{
    // NOTE: HACK: Can't rely on audio system to apply master volume changes in a non-discontinuous way
    // (we force master volume to be 1 in pause screen), so have to use it directly to modulate volume
    f32 hack_volume = get_pause_screen_event_sound_volume(pause_screen->master_volume_setting);
    play_sound(audio, pause_screen_event_sound_id, (playing_sound_flag_no_fade | playing_sound_flag_no_pause_capture), 4.0f, -1, hack_volume);
}

internal void initialize_pause_screen(Pause_Screen *pause_screen, Game_Audio *game_audio, Game_State *game_state)
{
    pause_and_capture_currently_playing_sounds(game_audio);

    zero_object(Pause_Screen, *pause_screen);
    
    pause_screen->active = true;
    pause_screen->confirmation_icon_flash = make_flashing_value(pause_screen_confirmation_icon_flash_time, true);
                
    pause_screen->master_volume_test_sound = load_sound(game_audio, asset_spinner_laser, playing_sound_flag_loop);
    // NOTE: After we copy master volume into pause screen, need to set master volume to full
    // so that test sound in pause screen is not affected by it.
    pause_screen->master_volume_setting = game_audio->master_volume;
    game_audio->master_volume = 1.0f;

    pause_screen->volume_slider_cursor_y = get_volume_slider_cursor_y(get_volume_slider(), pause_screen->master_volume_setting);

    pause_screen->cursor_position_before_pause = game_state->cursor.position;
    
    play_pause_screen_event_sound(pause_screen, game_audio);
}

internal void shutdown_pause_screen(Pause_Screen *pause_screen, Game_Audio *game_audio, Game_Mode_Result game_mode_result, Game_State *game_state)
{
    play_pause_screen_event_sound(pause_screen, game_audio);

    
    game_audio->master_volume = pause_screen->master_volume_setting;

    release_sound(game_audio, pause_screen->master_volume_test_sound);

    // NOTE: If we're switching game mode then should be in game level
    assert(pause_screen->pre_update_toggle_action != pause_screen_pre_update_toggle_action_quit_level || game_mode_result.action == game_mode_result_action_switch_mode);
    
    if(game_mode_result.action == game_mode_result_action_switch_mode)
    {
        // NOTE: Don't want to resume sounds if changing game mode, just release
        // them from capture and let game mode switch clean up the sounds
        release_captured_sounds(&game_audio->pause_captured_sounds);
    }
    else
    {
        // NOTE: Don't want to deactivate pause if changing mode, otherwise we'll
        // do a tick of current game mode, instead let game mode zero out pause object
        pause_screen->active = false;
        resume_pause_game_captured_sounds(game_audio);   
    } 

    // NOTE: If playing level, then put cursor back to where it was
    if(game_state->game_mode == game_mode_playing_level
       && game_state->level.stage.mode == level_mode_playing)
    {
        game_state->cursor.position = pause_screen->cursor_position_before_pause;
    }

    pause_screen->pre_update_toggle_action = pause_screen_pre_update_toggle_action_null;
}

internal void change_pause_screen_mode(Pause_Screen *pause_screen, s32 mode, Game_Audio *audio)
{
    pause_screen->mode = mode;
    set_time_for_one_flash(&pause_screen->confirmation_icon_flash, pause_screen_confirmation_icon_flash_time);
    restart_flashing_value(&pause_screen->confirmation_icon_flash);

    play_pause_screen_event_sound(pause_screen, audio);
}

internal b32 on_main_pause_screen(Pause_Screen *pause_screen)
{
    b32 result = (pause_screen->mode == pause_mode_main);
    return result;
}

internal Pause_Screen_Update_Action update_pause_screen(Pause_Screen *pause_screen, Game_State *game_state, Game_Input *input, UI_Context *ui_context, Game_Render_Commands *render_commands, Game_Audio *game_audio, Cursor *cursor, Game_Mode_Result *game_mode_result)
{
    Pause_Screen_Update_Action update_action = pause_screen_update_action_null;
    
    if(pause_screen->active)
    {
        UI_Id slider_id = make_ui_id(&pause_screen->master_volume_setting);
        UI_Id volume_button_id = make_ui_id(pause_screen->master_volume_test_sound);
        UI_Id restart_game_button_id = make_ui_id(pause_screen);
        UI_Id quit_no_confirm_button_id = make_ui_id(&pause_screen->active);
        UI_Id quit_yes_confirm_button_id = make_ui_id(&pause_screen->mode);
        UI_Id restart_game_no_confirm_button_id = make_ui_id(&pause_screen->confirmation_icon_flash);
        UI_Id restart_game_yes_confirm_button_id = make_ui_id(&pause_screen->cursor_position_before_pause);

        // NOTE: Black background
        push_rect(render_commands, vec2(0.0f), vec2(virtual_screen_width, virtual_screen_height), vec4(0.0f, 0.0f, 0.0f, 1.0f), draw_order_transform(entity_draw_order_background));
        
        update_flashing_value(&pause_screen->confirmation_icon_flash, input->dt);

        Vec2 confirmation_icon_scale = vec2(menu_button_size * 1.25f);
        Vec2 confirmation_icon_position = vec2(virtual_screen_center_x - confirmation_icon_scale.x/2.0f, virtual_screen_height*0.75f - confirmation_icon_scale.y/2.0f);

        f32 confirm_button_y = virtual_screen_height*0.4125f - menu_button_size/2.0f;
        Vec2 yes_confirm_button_position = vec2(virtual_screen_center_x - menu_button_size - menu_button_spacing, confirm_button_y);
        Vec2 no_confirm_button_position = vec2(virtual_screen_center_x + menu_button_spacing, confirm_button_y);

        if(first_button_press(input->pause_button) && !on_main_pause_screen(pause_screen))
        {
            // NOTE: Go back to main screen
            change_pause_screen_mode(pause_screen, pause_mode_main, game_audio);
        }        
        
        // NOTE: Quit button goes to different confirmation screens depending on game mode
        s32 quit_button_click_mode_change = pause_mode_quit_game_confirmation;
        s32 quit_button_icon = menu_button_icon_quit_game;
        if(game_state->game_mode == game_mode_hub)
        {
            quit_button_click_mode_change = pause_mode_quit_game_confirmation;
            quit_button_icon = menu_button_icon_quit_game;
        }
        else if(game_state->game_mode == game_mode_playing_level)
        {
            quit_button_click_mode_change = pause_mode_quit_level_confirmation;
            quit_button_icon = menu_button_icon_quit_level;
        }

        f32 screen_edge_offset = menu_button_half_size/2.0f;
        
        switch(pause_screen->mode)
        {
            case pause_mode_main:
            {
                // NOTE: Buttons in the center
                {
                    u32 center_buttons_count = 0;
                    // NOTE: Always have a resume button, but fullscreen and quit buttons
                    // availability is decided by platform layer
                    pause_screen->center_buttons[center_buttons_count++] = pause_center_button_resume;
                    
                    if(platform.player_platform_option_flags & player_platform_option_fullscreen_flag)
                    {
                        pause_screen->center_buttons[center_buttons_count++] = pause_center_button_toggle_fullscreen;
                    }
                    if(platform.player_platform_option_flags & player_platform_option_quit_flag)
                    {
                        pause_screen->center_buttons[center_buttons_count++] = pause_center_button_quit;
                    }
                    
                    assert(center_buttons_count <= pause_center_buttons_count);
                    f32 center_buttons_total_height = ((center_buttons_count*menu_button_size) + ((center_buttons_count - 1)*menu_button_half_size));
                    // NOTE: We want them the first button in list to be at the top, then we move down from here for the rest of the buttons
                    f32 center_buttons_first_button_y = ((virtual_screen_height/2.0f) + (center_buttons_total_height / 2.0f) - menu_button_size);
                    // NOTE: Do the buttons
                    for(u32 center_button_index = 0; center_button_index < center_buttons_count; ++center_button_index)
                    {                        
                        s32 center_button_type = pause_screen->center_buttons[center_button_index];

                        f32 center_button_y = center_buttons_first_button_y - (center_button_index * (menu_button_size + menu_button_spacing));
                        
                        Vec2 button_position = vec2((virtual_screen_width/2.0f) - menu_button_half_size, center_button_y);

                        UI_Id button_id = make_ui_id(&pause_screen->center_buttons[center_button_index]);
                        
                        switch(center_button_type)
                        {
                            case pause_center_button_resume:
                            {
                                if(do_menu_button(menu_button_icon_resume_game, ui_context, button_id, render_commands, button_position, menu_button_scale))
                                {
                                    pause_screen->pre_update_toggle_action = pause_screen_pre_update_toggle_action_resume;
                                }                            
                            } break;

                            case pause_center_button_toggle_fullscreen:
                            {
                                if(do_menu_button(input->settings.fullscreen ? menu_button_icon_go_windowed : menu_button_icon_go_fullscreen, ui_context, button_id, render_commands, button_position, menu_button_scale)) 
                                {
                                    update_action = pause_screen_update_action_toggle_fullscreen;
                                }            
                            } break;

                            case pause_center_button_quit:
                            {
                                if(do_menu_button(quit_button_icon, ui_context, button_id, render_commands, button_position, menu_button_scale))
                                {
                                    change_pause_screen_mode(pause_screen, quit_button_click_mode_change, game_audio);
                                }
                            } break;

                            invalid_default_case;
                        };
                    }   
                }
                               
                //
                // NOTE: Volume
                //
                {
                    Volume_Slider vs = get_volume_slider();
                    
                    b32 cursor_in_volume_area = point_vs_rect(cursor->position, vs.border_position, vs.border_scale);

                    f32 volume = pause_screen->master_volume_setting * 100.0f;

                    Render_Transform transform = corner_indent_transform(1.0f, draw_order_transform(entity_draw_order_volume_ui));
                    
                    // NOTE: Do Slider Part (based on master volume)
                    {
                        {
                            Vec3 slider_background_colour = vec3(1.0f, 0.0f, 0.0f);
                            
                            push_blocky_blend(render_commands, vs.slider_bar_pos, vec2(vs.slider_bar_mark_max_width, vs.slider_bar_scale.y), vec4(slider_background_colour, 0.01f), vec4(slider_background_colour, 0.25f), vec2(0.0f, 0.0f), vec2(0.0f, 1.0f), vec2(1.0f, 0.0f), vec2(1.0f, 1.0f), transform_flags(render_transform_flags_blend_exclude_colours));                            
                        }
                        
                        Vec4 slider_colour = vec4(get_ui_item_colour(ui_context, slider_id), 1.0f);

                        Vec2 slider_cursor_position = vec2(vs.slider_bar_pos.x - vs.cursor_scale.x/8.0f, pause_screen->volume_slider_cursor_y);

                        Indented_Triangle_World_Points slider_cursor_shape = get_indented_triangle_world_points(slider_cursor_position, vs.cursor_scale, rotation_transform(-PI/2.0f, vs.cursor_scale/2.0f, corner_indent_transform(4.0f, transform)));
                        
                        // NOTE: Collision detection
                        b32 cursor_inside_element = false;
                        {
                            if(is_cursor_inside_element(ui_context, slider_id, vs.slider_bar_pos, vs.slider_scale))
                            {
                                cursor_inside_element = true;
                            }
                            else if(point_vs_polygon(cursor->position, slider_cursor_shape.points, array_count(slider_cursor_shape.points)))
                            {
                                cursor_inside_element = true;
                            }
                        }
                        
                        Do_Slider_Logic_Result slider_result = do_slider_logic(ui_context, 100.0f, &volume, vs.slider_bar_pos, vs.slider_scale, cursor_inside_element, slider_id);

                        pause_screen->volume_slider_cursor_y = get_volume_slider_cursor_y(vs, slider_result.cursor_normalized);
                        // NOTE: Volume Bar
                        push_rect(render_commands, vs.slider_bar_pos, vec2(vs.slider_bar_scale.x, vs.slider_bar_scale.y), slider_colour, transform);

                        push_rect(render_commands, vec2(vs.slider_bar_pos.x, vs.slider_bar_pos.y), vec2(vs.slider_bar_mark_max_width*0.5f, vs.slider_bar_mark_height), slider_colour, transform);
                        push_rect(render_commands, vec2(vs.slider_bar_pos.x, vs.slider_bar_pos.y + vs.slider_bar_scale.y*0.5f - 1.25f), vec2(vs.slider_bar_mark_max_width*0.75f, vs.slider_bar_mark_height), slider_colour, transform);
                        push_rect(render_commands, vec2(vs.slider_bar_pos.x, vs.slider_bar_pos.y + vs.slider_bar_scale.y - vs.slider_bar_mark_height), vec2(vs.slider_bar_mark_max_width, vs.slider_bar_mark_height), slider_colour, transform);

                        // NOTE: Slider cursor
                        {
                            Vec3 cursor_colour = select_ui_item_colour(ui_context, slider_id, vec3(1.0f, 0.0f, 1.0f), vec3(0.75f, 0.0f, 0.0f), vec3(0.5f, 0.0f, 0.0f));
                            
                            push_shape_points(render_commands, slider_cursor_shape.points, array_count(slider_cursor_shape.points), vec4(cursor_colour, 1.0f), slider_cursor_shape.transform);
                        }
                    }

                    b32 slider_hot = (ui_item_hot(ui_context, slider_id));
                    // NOTE: Volume Speaker Button
                    if(do_menu_button((volume == 0.0f ? menu_button_icon_volume_speaker_muted : menu_button_icon_volume_speaker), ui_context, volume_button_id, render_commands, vs.button_position, vs.button_scale, slider_hot))
                    {
                        volume = (volume == 0.0f) ? 100.0f : 0.0f;
                    }

                    Playing_Sound *master_volume_test_sound = pause_screen->master_volume_test_sound;
                    if(ui_item_hot(ui_context, slider_id) || ui_item_hot(ui_context, menu_button_icon_volume_speaker) || cursor_in_volume_area)
                    {
                        master_volume_test_sound->playing_flags |= playing_sound_flag_loop;
                        if(!master_volume_test_sound->playing)
                        {
                            play_sound(master_volume_test_sound, master_volume_test_sound->speed, pause_screen->master_volume_setting);
                        }
                        else
                        {
                            set_volume(master_volume_test_sound, pause_screen->master_volume_setting);
                        }

                        // NOTE: Might as well say cursor is in volume area too volume area is "activated"
                        cursor_in_volume_area = true;
                    }
                    else
                    {
                        pause_screen->master_volume_test_sound->playing_flags &= ~playing_sound_flag_loop;
                        set_volume(master_volume_test_sound, 0.0f);
                    }

                    // NOTE: Volume area
                    {                        
                        Vec3 colour = vec3(0.15f);
                        if(cursor_in_volume_area)
                        {
                            colour = vec3(0.4f);
                        }
                        
                        push_rect_outline(render_commands, vs.border_position, vs.border_scale, vec4(colour, 1.0f), thickness_transform(3.0f, transform_flags(render_transform_flag_half_line_corner_indent)));
                    }
                    
                    pause_screen->master_volume_setting = clamp01(volume / 100.0f);

                    // NOTE: HACK: change volume of all currently playing sounds, which sould be pause screen event sounds ONLY 
                    for(Playing_Sound *s = game_audio->first_playing_sound; s; s = s->next)
                    {
                        if(s->playing && s->_owned_by_audio_system)
                        {
                            assert(s->id == pause_screen_event_sound_id);
                            set_volume(s, get_pause_screen_event_sound_volume(pause_screen->master_volume_setting));
                        }
                    }                        
                }                                        

                if(do_menu_button(menu_button_icon_restart_game, ui_context, restart_game_button_id, render_commands, vec2(screen_edge_offset), vec2(75.0f)))
                {
                    change_pause_screen_mode(pause_screen, pause_mode_restart_game_confirmation, game_audio);
                }
            } break;

            case pause_mode_quit_game_confirmation:
            case pause_mode_quit_level_confirmation:
            {
                draw_quit_icon(render_commands, confirmation_icon_position, confirmation_icon_scale, vec3(1.0f), pause_screen->confirmation_icon_flash.t, safe_element_colour, corner_indent_transform(4.0f));
                    
                // NOTE: Buttons for no, yes confirmation
                if(do_menu_button(menu_button_icon_no_confirm, ui_context, quit_no_confirm_button_id, render_commands, no_confirm_button_position, menu_button_scale))
                {
                    change_pause_screen_mode(pause_screen, pause_mode_main, game_audio);
                }
                    
                if(do_menu_button(menu_button_icon_yes_confirm, ui_context, quit_yes_confirm_button_id, render_commands, yes_confirm_button_position, menu_button_scale))
                {
                    if(pause_screen->mode == pause_mode_quit_game_confirmation)
                    {
                        update_action = pause_screen_update_action_quit_game;
                    }
                    else if(pause_screen->mode == pause_mode_quit_level_confirmation)
                    {
                        pause_screen->pre_update_toggle_action = pause_screen_pre_update_toggle_action_quit_level;
                    }
                    else
                    {
                        invalid_code_path;
                    }
                }
                    
            } break;
                
            // NOTE: Restart Game Confirmation
            case pause_mode_restart_game_confirmation:
            case pause_mode_restart_game_second_confirmation:
            {
                draw_restart_game_icon(render_commands, confirmation_icon_position, confirmation_icon_scale, pause_screen->confirmation_icon_flash.t, safe_element_colour, corner_indent_transform(4.0f));

                Vec2 actual_no_button_position = no_confirm_button_position;
                Vec2 actual_yes_button_position = yes_confirm_button_position;
                // NOTE: Swap confirmation buttons around if on second confirmation
                if(pause_screen->mode == pause_mode_restart_game_second_confirmation)
                {
                    Vec2 temp = actual_no_button_position;
                    actual_no_button_position = actual_yes_button_position;
                    actual_yes_button_position = temp;
                }
                    
                // NOTE: Buttons for no, yes confirmation
                if(do_menu_button(menu_button_icon_no_confirm, ui_context, restart_game_no_confirm_button_id, render_commands, actual_no_button_position, menu_button_scale))
                {
                    change_pause_screen_mode(pause_screen, pause_mode_main, game_audio);
                }
                if(do_menu_button(menu_button_icon_yes_confirm, ui_context, restart_game_yes_confirm_button_id, render_commands, actual_yes_button_position, menu_button_scale))
                {
                    if(pause_screen->mode == pause_mode_restart_game_second_confirmation)
                    {
                        update_action = pause_screen_update_action_restart_game;
                    }
                    else
                    {
                        change_pause_screen_mode(pause_screen, pause_mode_restart_game_second_confirmation, game_audio);
                        // NOTE: Want the icon to flash faster on second confirmation screen
                        set_time_for_one_flash(&pause_screen->confirmation_icon_flash, pause_screen_second_confirmation_icon_flash_time);
                    }
                }
 
            } break;

            invalid_default_case;                
        }
    }
    
    if(update_action)
    {
        play_pause_screen_event_sound(pause_screen, game_audio);
    }

    return update_action;
}
