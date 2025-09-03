#include "twosome_game_hub.h"


internal f32 hub_state_elapsed_time(Game_Hub *hub, f32 gametime)
{
    f32 result = (gametime - hub->state_change_time);
    return result;
}

internal b32 has_hub_state_time_elapsed(Game_Hub *hub, f32 gametime, f32 time)
{
    b32 result = (hub_state_elapsed_time(hub, gametime) >= time);
    return result;
}

internal b32 is_hub_state(Game_Hub *hub, s32 state)
{
    b32 result = hub->state == state;
    return result;
}

internal b32 has_song_time_elapsed(Game_Hub *hub, f32 time)
{
    b32 result = (get_sound_position(hub->boys_song) >= time);
    return result;
}

internal b32 is_boy_singing(Game_Hub *hub)
{
    b32 result = (is_hub_state(hub, game_hub_state_boy_prepping_to_sing) || is_hub_state(hub, game_hub_state_boy_singing));
    return result;
}

internal void set_hub_state(Game_Hub *hub, s32 state, f32 gametime)
{
    hub->state = state;
    hub->state_change_time = gametime;
}

internal void change_hair_strand_bob(Hair_Strand *strand, RNG *rng)
{
    strand->bob_speed = random(rng, 5, 30)/1000.0f;
    strand->bob_size = random(rng, 1, 10)/100.0f;
}

internal void init_game_hub(Game_Hub *hub, Game_Audio *audio, f32 gametime, RNG *rng)
{
    zero_buffer(hub, sizeof(Game_Hub));
    
    hub->girl_side_background_colour = girl_side_intro_background_colour;
    
    set_hub_state(hub, game_hub_state_boy_alone, gametime);
    hub->score_meter_goal_flash = make_flashing_value(0.75f, true);
    hub->score_meter_fade = make_flashing_value(0.5f, false, flashing_value_flag_expanding);
    hub->next_level_select_flash = make_flashing_value(1.5f, true, flashing_value_flag_expanding);
    
    hub->last_level_select_index_hovered_over = -1;
    
    //
    // NOTE: Level scores to get
    //
    {
        hub->level_scores_to_get[game_level_type_connectors] = 1000;
        hub->level_scores_to_get[game_level_type_spikes_barrier] = 2000;
        hub->level_scores_to_get[game_level_type_spinners] = 2000;
        hub->level_scores_to_get[game_level_type_everything] = 1800;
        hub->level_scores_to_get[game_level_type_shield_intro] = 5000;
        hub->level_scores_to_get[game_level_type_shield_everything] = 6000;
        
#if TWOSOME_INTERNAL
        // NOTE: If want easy scores to beat in debugging
#if 0
        hub->level_scores_to_get[game_level_type_connectors] = 10;
        hub->level_scores_to_get[game_level_type_spikes_barrier] = 20;
        hub->level_scores_to_get[game_level_type_spinners] = 18;
        hub->level_scores_to_get[game_level_type_everything] = 14;
        hub->level_scores_to_get[game_level_type_shield_intro] = 10;
        hub->level_scores_to_get[game_level_type_shield_everything] = 80;
#endif
#endif
    }
    
    {
        Boy_Face *face = &hub->boy_face;
        face->scale = vec2(400.0f);
        face->position = vec2(50.0f);
        init_anim(&face->anim, blank);
        set_idle_anim(&face->anim, anim_idle);
        jump_to_first_anim_frame(&hub->boy_face.anim);        
        
        Boy_Idle_Anim idle_anims[] =
        {
            { anim_clears_throat, asset_boy_clears_throat },
            { anim_sigh, asset_boy_sighs },
            { anim_yawn, asset_boy_yawn },
            { anim_noticing_girl, asset_none }
        };
        assert(array_count(idle_anims) == array_count(face->idle_anims));
        copy_memory(face->idle_anims, idle_anims, sizeof(idle_anims));
        
        face->colour_flash = make_flashing_value();
    }
    
    // NOTE: Girl Face
    {
        Girl_Face *face = &hub->girl_face;
        face->scale = vec2(200.0f, 400.0f);
        face->position = vec2(550.0f, 50.0f);
        face->original_position = face->position;
        
        init_anim(&face->anim, blank);
        set_idle_anim(&face->anim, anim_listening);
        jump_to_random_anim_frame(&hub->girl_face.anim, rng);
        
        face->colour_flash = make_flashing_value();
        
        Vec2 hair_scale = vec2(face->scale.x*0.1f, face->scale.y*0.8f);
        
        // NOTE: Setup each hair strand
        {
            struct Strand_Setup
            {
                Vec2 position;
                Vec2 scale;
                f32 start_rot;
                f32 end_rot;
            };
            
            Strand_Setup strand_setups[] =
            {
                { vec2(face->scale.x*0.5f, face->scale.y*0.6f), hair_scale, -PI*1.1f, -PI*0.8f },
                { vec2(face->scale.x*0.5f, face->scale.y*0.85f), hair_scale, -PI*1.15f, -PI*0.85f },
                { vec2(face->scale.x*0.9f, face->scale.y*0.9f), hair_scale*0.8f, -PI*1.2f, -PI*0.8f },
                { vec2(face->scale.x*0.5f, face->scale.y*0.95f), hair_scale*0.8f, -PI*1.2f, -PI*0.8f },
                { vec2(face->scale.x*0.2f, face->scale.y*1.01f), hair_scale*0.6f, -PI*1.175f, -PI*0.9f },
                { vec2(face->scale.x*0.8f, face->scale.y*1.01f), hair_scale*0.6f, -PI*1.225f, -PI*0.9f }
            };
            assert(array_count(strand_setups) == array_count(face->strands));
            
            for(u32 strand_index = 0; strand_index < array_count(face->strands); ++strand_index)
            {
                Strand_Setup *setup = &strand_setups[strand_index];
                Hair_Strand *strand = &face->strands[strand_index];
                strand->position = setup->position;
                strand->scale = setup->scale;
                strand->start_rot = setup->start_rot;
                strand->end_rot = setup->end_rot;
                
                strand->bob_velocity = (random_boolean(rng) ? 1.0f : -1.0f);
                change_hair_strand_bob(strand, rng);
            }
        }
    }
    
    {
        Hub_Progress_Button *progress_button = &hub->progress_button;
        
        progress_button->scale = vec2(hub_progress_button_radius*2.0f);
        progress_button->position.x = (virtual_screen_width*0.5f) - (progress_button->scale.x*0.5f);
        f32 top_boy_face = (hub->boy_face.position.y + hub->boy_face.scale.y);
        f32 top_boy_face_gap = (virtual_screen_height - top_boy_face);
        progress_button->position.y = top_boy_face + ((top_boy_face_gap - hub->progress_button.scale.y)/2.0f);
        progress_button->flash = make_flashing_value(1.75f, false, flashing_value_flag_expanding);
        progress_button->flash.velocity = -1;
        
        assert(array_count(progress_button->goal_positions) == number_of_levels);
        
        progress_button->position.y = (virtual_screen_height - progress_button->scale.y/1.5f);
        
        progress_button->goal_scale = progress_button->scale / 8.0f;
        // NOTE: These are offsets from bl, then we add the position after
        progress_button->goal_positions[0] = vec2(progress_button->scale.x*0.6f, progress_button->scale.y*0.15f);
        progress_button->goal_positions[1] = vec2(progress_button->scale.x*0.2f, progress_button->scale.y*0.2f);
        progress_button->goal_positions[2] = vec2(progress_button->scale.x*0.1f, progress_button->scale.y*0.5f);
        progress_button->goal_positions[3] = vec2(progress_button->scale.x*0.7f, progress_button->scale.y*0.4f);
        progress_button->goal_positions[4] = vec2(progress_button->scale.x*0.4f, progress_button->scale.y*0.55f);
        progress_button->goal_positions[5] = vec2(progress_button->scale.x*0.45f, progress_button->scale.y*0.35f);
        
        for(s32 hole_position_index = 0; hole_position_index < array_count(progress_button->goal_positions); ++hole_position_index)
        {
            progress_button->goal_positions[hole_position_index] = progress_button->position + progress_button->goal_positions[hole_position_index];
        }
    }
    
    hub->boys_song = load_sound(audio, asset_boys_song);
    hub->boys_song_whistle = load_sound(audio, asset_boys_song_whistle);
    hub->girl_saying_credits = load_sound(audio, asset_girl_saying_credits);
    hub->background_ambience = load_overlapping_sound(audio, asset_hub_ambience, 0, true);
    play_sound(hub->background_ambience, 1.0f, 0.0f);
}

internal void pick_next_time_to_play_boys_special_idle_anim(Boy_Face *face, f32 gametime, RNG *rng)
{
    face->next_time_to_play_special_idle_anim = (gametime + (random(rng, 2000, 3000) / 100.0f));
}

internal void deactivate_game_hub(Game_Hub *hub)
{
    fade_out_sound(hub->background_ambience, stage_transition_sound_fade_duration);
    fade_out_sound(hub->girl_saying_credits, stage_transition_sound_fade_duration);
    
    hub->first_active_firework = 0;
    hub->first_free_firework = 0;
    
    // NOTE: Have to do this because anim sound fades out, and can't have anim
    // continue when going back to hub when the sound is no longer active
    cancel_non_idle_anim(&hub->boy_face.anim, true);
    cancel_non_idle_anim(&hub->girl_face.anim, true);
}

internal void reset_hub_background_melody(Game_Hub *hub, f32 gametime)
{
    hub->current_background_chord_type_index = 0;
    hub->background_melody_index = 0;
    hub->background_melody_last_played_time = (gametime - background_melody_interval) + 5.0f;
    hub->background_melody_speed = 1.0f;
}

internal void set_hub_ui_fade_values(Hub_UI_Fade *ui_fade, f32 fade)
{
    fade = clamp01(fade);
    
    ui_fade->progress_button = fade;
    ui_fade->selector_buttons = fade;
    ui_fade->credits_button = fade;
}

internal void fade_hub_ui(Game_Hub *hub, f32 dt, s32 direction)
{
    Hub_UI_Fade *ui_fade = &hub->ui_fade;
    
    // NOTE: This function only makes sense after/whilst boy has started singing,
    // as that's the only time we start to fade in and out the ui elements, and
    // this logic only make sense in that context
    assert(hub->state > game_hub_state_normal);
    
    assert(direction == -1 || direction == 1);
    
    {
        f32 progress_button_fade_direction = (f32)direction;
        // NOTE: We don't allow fading out of progress button if boy hasn't already sang
        // as it doesn't fade out next time he sings
        if(hub->boy_face.has_sang)
        {
            progress_button_fade_direction = 1.0f;
        }
        ui_fade->progress_button += dt*progress_button_fade_direction*hub_ui_elements_fade_speed;
        ui_fade->progress_button = clamp01(ui_fade->progress_button);
    }
    
    {
        ui_fade->selector_buttons += dt*direction*hub_ui_elements_fade_speed;
        ui_fade->selector_buttons = clamp01(ui_fade->selector_buttons);
    }
    
    {
        f32 credits_button_fade_direction = (f32)direction;
        // NOTE: We don't allow fading in of credits button if credits
        // are currently being said
        if(hub->girl_face.saying_credits)
        {
            credits_button_fade_direction = -1.0f;
        }
        ui_fade->credits_button += dt*credits_button_fade_direction*hub_ui_elements_fade_speed;
        ui_fade->credits_button = clamp01(ui_fade->credits_button);
    }
}

// NOTE: Used when switching to game hub
internal void activate_game_hub(Game_Hub *hub, Game_Audio *audio, Game_Progress *progress, Cursor *cursor, f32 gametime, RNG *rng, Memory_Arena *mode_arena)
{
    hub->mode_arena = mode_arena;
    
    switch(hub->state)
    {
        case game_hub_state_boy_alone:
        {
            f32 boy_right = hub->boy_face.position.x + hub->boy_face.scale.x;
            // NOTE: Want to place the cursor between boy face and edge of screen for x, and centered for y
            cursor->position = vec2(boy_right + (virtual_screen_width - boy_right)/2.0f, hub->boy_face.position.y + hub->boy_face.scale.y*0.5f);
        } break;        
        
    }
    
    assert(number_of_levels == array_count(hub->level_selects));
    // NOTE: Unlock the level selects based on the game save (unless we're in the intro in which case the first unlock is scripted to happen at a certain time)
    assert(progress->levels_unlocked <= array_count(hub->level_selects));
    for(int level_select_index = 0; level_select_index < progress->levels_unlocked; ++level_select_index)
    {
        Level_Select *select = &hub->level_selects[level_select_index];
        select->unlocked = true;
    }
    
    reset_hub_background_melody(hub, gametime);
    
    restart_flashing_value(&hub->next_level_select_flash);
    
    // NOTE: Fade in background ambience
    resume_sound(hub->background_ambience);
    fade_in_sound(hub->background_ambience, /*stage_transition_sound_fade_duration*/1.0f);
    
    pick_next_time_to_play_boys_special_idle_anim(&hub->boy_face, gametime, rng);
    
    set_hub_ui_fade_values(&hub->ui_fade, 1.0f);
}

internal void draw_hair_strand(Game_Render_Commands *render_commands, Vec2 position, Vec2 scale, Vec4 colour, r32 start_rot, r32 end_rot, r32 rot_t, Hair_Strand *strand, f32 dt, RNG *rng)
{
    strand->bob += strand->bob_speed * strand->bob_velocity * dt;
    b32 velocity_changed = true;
    if(strand->bob > strand->bob_size)
    {
        strand->bob_velocity = -1.0f;        
    }
    else if(strand->bob < -strand->bob_size)
    {
        strand->bob_velocity = 1.0f;
    }
    else
    {
        velocity_changed = false;
    }
    
    if(velocity_changed)
    {
        strand->bob_speed = (random(rng, 5, 30) / 1000.0f);
        strand->bob_size = (random(rng, 1, 10) / 100.0f);
    }
    
    r32 strand_rot = (start_rot + (end_rot - start_rot)*rot_t) + strand->bob;
    push_rect(render_commands, position, scale, colour, rotation_transform(strand_rot, vec2(0.0f)));
}

internal void update_and_draw_girl_face(RNG *rng, Girl_Face *girl_face, Game_Render_Commands *render_commands, f32 dt, f32 alpha, f32 gametime, UI_Context *ui_context, Game_Hub *hub)
{
    update_flashing_value(&girl_face->colour_flash, dt);
    update_animation(&girl_face->anim, dt);
    
    // NOTE: Movement
    {
        if(girl_face->movement_flags & girl_movement_flag_turn_to_boy)
        {
            f32 turn_speed = ((1.0f / time_for_girl_to_turn_to_boy) * dt);
            girl_face->rot_t += turn_speed;
            girl_face->rot_t = clamp(girl_face->rot_t, 0.0f, 1.0f);
        }
        
        if(girl_face->movement_flags & girl_movement_flag_move_to_boy)
        {
            if(abs(girl_face->original_position.x - girl_face->position.x) < 50.0f)
            {
                girl_face->position.x -= (dt * 16.0f);
            }
        }
    }
    
    Render_Transform transform = thickness_transform(hub_entity_line_thickness, draw_order_transform(entity_draw_order_face, transform_flags(render_transform_flag_half_line_corner_indent)));
    
    Vec4 border_colour = vec4( (vec3(1.0f) * (1.0f - girl_face->colour_flash.t)) + (vec3(0.0f) * girl_face->colour_flash.t), alpha);
    Vec4 colour = vec4(1.0f, 1.0f, 1.0f, alpha);
    
    Vec2 face_tr = girl_face->position + girl_face->scale;
    
    Key_Frame frame = girl_face->anim.current_frame;
    
    //
    // NOTE: Eyes
    //
    f32 eye_full_height = girl_face->scale.x*0.14f;
    Vec2 eye_scale = vec2(girl_face->scale.x*0.35f, eye_full_height*frame.eye_size + transform.line_thickness*2.0f);
    
    f32 eye_pos_y = girl_face->scale.y*0.7f - eye_scale.y/2.0f;
    
    f32 eye_face_side_offset = (girl_face->scale.x*0.05f);
    
    // NOTE: Right Eye
    {
        f32 eye_t = clamp(girl_face->rot_t*0.25f, 0.0f, 1.0f);
        
        Vec2 eye_start_pos = girl_face->position + vec2(girl_face->scale.x - eye_scale.x - eye_face_side_offset, eye_pos_y);
        Vec2 eye_end_pos = girl_face->position + vec2(face_tr.x, eye_pos_y);
        Vec2 eye_pos = eye_start_pos + (eye_end_pos - eye_start_pos)*eye_t;
        
        eye_pos.x += frame.eye_dir.x*(eye_face_side_offset - transform.line_thickness);
        eye_pos.y += frame.eye_dir.y*(eye_face_side_offset - transform.line_thickness);
        
        Vec2 eye_left = eye_pos;
        Vec2 eye_right = eye_pos + eye_scale;
        
        eye_left.x = min(eye_left.x, face_tr.x);
        eye_right.x = min(eye_right.x, face_tr.x);        
        
        push_rect_outline(render_commands, eye_left, eye_right - eye_left, colour, transform);
    }
    // NOTE: Left eye
    {
        r32 eye_t = clamp(((girl_face->rot_t - 0.75f) / 0.25f), 0.0f, 1.0f);
        
        Vec2 eye_start_pos = girl_face->position + vec2(-eye_scale.x, eye_pos_y);
        Vec2 eye_end_pos = girl_face->position + vec2(girl_face->scale.x*0.05f, eye_pos_y);
        
        Vec2 eye_pos = eye_start_pos + (eye_end_pos - eye_start_pos)*eye_t;
        eye_pos.x += frame.eye_dir.x*(eye_face_side_offset - transform.line_thickness);
        eye_pos.y += frame.eye_dir.x*(eye_face_side_offset - transform.line_thickness);
        
        Vec2 eye_left = eye_pos;
        Vec2 eye_right = eye_pos + eye_scale;
        
        eye_left.x = max(eye_left.x, girl_face->position.x);
        eye_right.x = max(eye_right.x, girl_face->position.x);
        
        push_rect_outline(render_commands, eye_left, eye_right - eye_left, colour, transform);
    }
    
    // NOTE: Nose
    {
        Vec2 initial_nose_scale = vec2((girl_face->scale.x*0.025f) + transform.line_thickness, girl_face->scale.y*0.15f);
        f32 nose_y = (girl_face->scale.y*0.525f - initial_nose_scale.y) + frame.mouth_size*girl_face->scale.y*0.015f;
        // NOTE: Right side
        {
            f32 nose_t = 1.0f - clamp((girl_face->rot_t / 0.25f), 0.0f, 1.0f);
            if(nose_t > 0.0f)
            {
                Vec2 nose_scale = vec2(initial_nose_scale.x*nose_t, initial_nose_scale.y);
                Vec2 nose_pos = girl_face->position + vec2(girl_face->scale.x - nose_scale.x, nose_y);
                
                push_line(render_commands, nose_pos, nose_pos + vec2(0.0f, nose_scale.y), colour, transform);
                push_line(render_commands, nose_pos, nose_pos + vec2(nose_scale.x, 0.0f), colour, transform);
            }                
        }
        
        // NOTE: Left side
        {
            f32 nose_t = clamp(((girl_face->rot_t - 0.75f) / 0.25f), 0.0f, 1.0f);
            if(nose_t > 0.0f)
            {
                Vec2 nose_scale = vec2(initial_nose_scale.x*nose_t, initial_nose_scale.y);
                Vec2 nose_pos = girl_face->position + vec2(0.0f, nose_y);
                
                push_line(render_commands, nose_pos + vec2(nose_scale.x, 0.0f), nose_pos + vec2(nose_scale.x, nose_scale.y), colour, transform);
                push_line(render_commands, nose_pos, nose_pos + vec2(nose_scale.x, 0.0f), colour, transform);
            }
        }
    }
    
    // NOTE: Mouth
    {
        // NOTE: Right side
        {
            r32 mouth_t = clamp(((girl_face->rot_t - 0.75f) / 0.25f), 0.0f, 1.0f);
            if(mouth_t > 0.0f)
            {
                f32 mouth_full_height = girl_face->scale.y*0.1f;
                Vec2 mouth_scale = vec2((girl_face->scale.x*0.25f)*mouth_t, mouth_full_height*frame.mouth_size);
                
                f32 mouth_center_y = (girl_face->position.y + (girl_face->scale.y*0.2f));
                f32 mouth_base_offset_y = mouth_center_y - mouth_scale.y/2.0f;
                
                f32 smile_end_pos_x = girl_face->position.x + transform.line_thickness*0.5f;
                
                f32 mouth_tip_y = (mouth_center_y + (frame.mouth_tip_dir*mouth_full_height));
                
                Vec2 verts[] =
                {
                    vec2(smile_end_pos_x, mouth_base_offset_y) - vec2(0.0f, transform.line_thickness),
                    vec2((girl_face->position.x + mouth_scale.x), mouth_tip_y),
                    vec2(smile_end_pos_x, mouth_base_offset_y + mouth_scale.y + transform.line_thickness)
                };
                push_line_points(render_commands, verts, array_count(verts), colour, transform);
            }
        }
        
        // NOTE: Left side
        {
            r32 mouth_t = clamp(1.0f - (girl_face->rot_t / 0.45f), 0.0f, 1.0f);
            if(mouth_t > 0.0f)
            {
                f32 mouth_full_height = (girl_face->scale.y * 0.1f);
                Vec2 mouth_scale = vec2(((girl_face->scale.x*0.25f) * mouth_t), mouth_full_height * frame.mouth_size);
                f32 mouth_center_y = (girl_face->position.y + (girl_face->scale.y * 0.2f));
                f32 mouth_base_offset_y = mouth_center_y - mouth_scale.y/2.0f;
                f32 smile_end_pos_x = (girl_face->position.x + girl_face->scale.x) - transform.line_thickness*0.5f;
                
                f32 mouth_tip_y = mouth_center_y + (frame.mouth_tip_dir*mouth_full_height*0.5f);
                Vec2 verts[] =
                {
                    vec2(smile_end_pos_x, mouth_base_offset_y),                    
                    vec2((girl_face->position.x + girl_face->scale.x - mouth_scale.x), mouth_tip_y),
                    vec2(smile_end_pos_x, mouth_base_offset_y + mouth_scale.y)
                };
                
                push_line_points(render_commands, verts, array_count(verts), colour, transform);
            }
        }
    }        
    
    // NOTE: Head
    push_rect_outline(render_commands, girl_face->position, girl_face->scale, border_colour, transform);
    
    // NOTE: Hair
    {
        Vec2 hair_scale = vec2(girl_face->scale.x*0.1f, girl_face->scale.y*0.8f);
        {
            f32 strand_t = clamp(girl_face->rot_t, 0.0f, 1.0f);
            
            Vec4 hair_colour = colour;
            
            for(u32 strand_index = 0; strand_index < array_count(girl_face->strands); ++strand_index)
            {
                Hair_Strand *strand = &girl_face->strands[strand_index];
                
                Render_Transform hair_transform = corner_indent_transform(/*hub_entity_line_thickness*0.5f*/strand->scale.x*0.15f, transform);
                
                f32 rot_t = strand_t;
                
                strand->bob += strand->bob_speed * strand->bob_velocity * dt;
                b32 velocity_changed = true;
                if(strand->bob > strand->bob_size)
                {
                    strand->bob_velocity = -1.0f;        
                }
                else if(strand->bob < -strand->bob_size)
                {
                    strand->bob_velocity = 1.0f;
                }
                else
                {
                    velocity_changed = false;
                }
                
                if(velocity_changed)
                {
                    change_hair_strand_bob(strand, rng);
                }
                
                f32 strand_rot = (strand->start_rot + (strand->end_rot - strand->start_rot) * rot_t) + strand->bob;
                push_rect(render_commands, girl_face->position + strand->position, strand->scale, colour, rotation_transform(strand_rot, /*vec2(0.0f)*/vec2(strand->scale.x/2.0f, 0.0f), hair_transform));
            }                
        }
    }
    
    // NOTE: Credits button, can only see it after boy has sang
    if(hub->boy_face.has_sang)
    {
        UI_Id ui_id = make_ui_id(girl_face);
        Vec2 scale = vec2(girl_face->scale.x*0.1f);
        Vec2 pos = girl_face->position + vec2(girl_face->scale.x*0.7f, girl_face->scale.y*0.1f);
        
        if(!girl_face->saying_credits)
        {                        
            // NOTE: Cant trigger credits unless boy and girl are in talking state
            if(hub->state == game_hub_state_boy_girl_talk)
            {
                if(do_button_logic(ui_context, ui_id, pos, scale))
                {
                    set_anim(&girl_face->anim, anim_girl_saying_credits);
                    play_sound(hub->girl_saying_credits);
                    girl_face->saying_credits = true;
                }
            }            
        }
        else
        {
            if(is_current_anim_idle(&girl_face->anim) && !hub->girl_saying_credits->playing)
            {
                girl_face->saying_credits = false;
            }
        }
        
        Vec3 button_colour = select_ui_item_colour(ui_context, ui_id, vec3(1.0f), vec3(0.25f), vec3(0.5f));
        
        push_triangle_outline(render_commands, pos, scale, vec4(button_colour, alpha*hub->ui_fade.credits_button), thickness_transform(hub_entity_line_thickness*0.75f, transform));
        
        Vec3 glow_colour = interpolate(danger_element_colour*0.25f, danger_element_colour, hub->score_meter_goal_flash.t);
        push_triangle_blocky_blend(render_commands, pos, scale, vec4(glow_colour, 1.0f*hub->ui_fade.credits_button), vec4(glow_colour, 0.1f*hub->ui_fade.credits_button), transform);
    }
}

internal b32 beaten_level_score(Game_Progress *progress, Game_Hub *hub, s32 level_index)
{
    b32 result = (progress->best_level_scores[level_index] >= hub->level_scores_to_get[level_index]);
    return result;
}

internal void end_boy_singing(Game_Hub *hub, f32 gametime)
{
    set_hub_state(hub, game_hub_state_boy_girl_talk, gametime);
    
    hub->boy_face.colour_flash.time_for_one_flash = face_slow_flash_value_time;
    set_idle_anim(&hub->boy_face.anim, anim_listening);
    hub->boy_face.has_sang = true;
    hub->boy_face.whistling = false;
    
    set_volume(hub->boys_song, 0.0f);
    set_volume(hub->boys_song_whistle, 0.0f);
    //stop_sound(hub->boys_song);
    //stop_sound(hub->boys_song_whistle);
    
    fade_in_sound(hub->background_ambience, stage_transition_sound_fade_duration);
    
    reset_hub_background_melody(hub, gametime);
}

internal void spawn_firework(Game_Hub *hub, RNG *rng, f32 gametime, f32 explode_time = -1.0f, f32 normalized_scale = -1.0f)
{
    if(!hub->first_free_firework)
    {
        hub->first_free_firework = push_struct(hub->mode_arena, Firework);
        hub->first_free_firework->next = 0;
    }
    
    Firework *firework = hub->first_free_firework;
    
    hub->first_free_firework = hub->first_free_firework->next;
    zero_object(Firework, *firework);    
    firework->next = hub->first_active_firework;
    hub->first_active_firework = firework;
    
    f32 side = 1.0f;
    firework->position.x = virtual_screen_width*0.5f;
    if(random_boolean(rng))
    {
        side = -1.0f;
    }
    
    firework->position.x += random(rng, 0, virtual_screen_width/2) * side;
    firework->radius = 18.0f;
    firework->shape = random(rng, 0, 4);
    
    f32 max_radius_t;
    if(normalized_scale >= 0.0f)
    {
        max_radius_t = normalized_scale;
        // NOTE: Add a little random variation +/-0.1
        max_radius_t += (random(rng, -10, 10)/100.0f);
    }
    else
    {
        max_radius_t = random(rng, 25, 100)/100.0f;
    }
    max_radius_t = clamp01(max_radius_t);
    
    firework->max_radius = interpolate(firework_smallest_max_scale, firework_largest_max_scale, max_radius_t);
    firework->velocity = vec2(2.5f + random(rng, -100, 100)/100.0f, 6.0f + random(rng, -100, 100)/100.0f);
    firework->velocity.x *= (side * -1.0f);
    
    {
        s32 col = random(rng, 0, 2);
        switch(col)
        {
            case 0:
            {
                firework->colour = vec3(1.0f, 0.0f, 0.0f);
            } break;
            
            case 1:
            {
                firework->colour = vec3(0.0f, 1.0f, 0.0f);
            } break;
            
            case 2:
            {
                firework->colour = vec3(0.0f, 0.0f, 1.0f);
            } break;
            
            invalid_default_case;
        };
    }
    
    if(explode_time > -1.0f)
    {
        firework->explode_time = gametime + explode_time;
    }
    else
    {
        firework->explode_time = gametime + firework_fade_out_time;
        firework->fade_out = true;
    }
}

internal Game_Mode_Result update_game_hub(Game_Hub *hub, Transient_State *tran_state, Game_Input *input, Game_Render_Commands *render_commands, Game_Audio *audio, Cursor *cursor, Game_Progress *game_progress, UI_Context *ui_context, RNG *rng, f32 gametime)
{
    Game_Mode_Result result = {};
    
    update_flashing_value(&hub->score_meter_goal_flash, input->dt);
    
    Girl_Face *girl_face = &hub->girl_face;
    Boy_Face *boy_face = &hub->boy_face;    
    
    //
    // NOTE: Update Fireworks
    //
    {
        //
        // NOTE: Do firework spawn behaviour
        // 
        if(hub->state < game_hub_state_normal)
        {            
            if(gametime >= hub->next_firework_spawn_time)
            {
                f32 explode_time = -1.0f;
                // NOTE: When boy girl are talking, occasionally we get an exploding firework
                if(hub->state == game_hub_state_boy_girl_talk)
                {
                    if(random(rng, 1, 3) != 3)
                    {
                        explode_time = (random(rng, 25, 75) / 100.0f);
                    }
                }
                
                spawn_firework(hub, rng, gametime, explode_time);
                hub->next_firework_spawn_time = gametime + (random(rng, 350, 650) / 100.0f);
            }   
        }
        else if(hub->state == game_hub_state_normal || hub->state == game_hub_state_boy_girl_talk)
        {                        
            //
            // NOTE: Background chords
            //
            f32 sound_speed = 1.0f;
            if(hub->state == game_hub_state_boy_girl_talk)
            {
                sound_speed = 1.25f;
            }
            
            if((gametime - hub->background_melody_last_played_time) >= (background_melody_interval * (1.0f / hub->background_melody_speed)))
            {
                // NOTE: The possible versions we can play
                u32 chord_assets[] =
                {
                    asset_hub_firework_unexplode,
                    asset_hub_firework_explodes_v1,
                    asset_hub_firework_explodes_v2
                };
                
                // NOTE: The greater the interval between notes, the quieter the sound
                f32 volume = (hub->background_melody_speed / background_melody_interval_variation_end);
                assert(hub->current_background_chord_type_index < array_count(chord_assets));
                
                u32 chord_sound_asset = chord_assets[hub->current_background_chord_type_index];
                play_sound(audio, chord_sound_asset, 0 & playing_sound_flag_reverse, sound_speed, hub->background_melody_index, volume);
                hub->background_melody_last_played_time = gametime;
                
                f32 explode_time = -1.0f;
                // NOTE: Only have the firework explode if we chose one of the exploding sounds
                if(chord_sound_asset != asset_hub_firework_unexplode)
                {
                    f32 one_speed_chord_explode_time = 0.75f;
                    explode_time = one_speed_chord_explode_time * (1.0f / sound_speed);
                }
                
                // NOTE: The louder the sound, the bigger the firework
                f32 normalized_firework_scale = clamp01(volume);
                spawn_firework(hub, rng, gametime, explode_time, normalized_firework_scale);
                
                ++hub->background_melody_index;
                
                Asset_Header *chord_asset_header = get_asset_header(tran_state->assets, chord_sound_asset);
                if(hub->background_melody_index >= chord_asset_header->sound.variations_count)
                {
                    hub->background_melody_index = 0;
                    hub->current_background_chord_type_index = random_exclusive(rng, 0, array_count(chord_assets) );
                    hub->background_melody_speed = ((f32)random(rng, (s32)(background_melody_interval_variation_start*100), (s32)(background_melody_interval_variation_end*100)) / 100.0f);
                }
            }
        }
        else if(hub->state >= game_hub_state_boy_prepping_to_sing && hub->state <= game_hub_state_boy_singing)
        {
            // NOTE: Firework display for during song
            if(hub->boys_song->playing)
            {
                if(gametime >= hub->next_firework_spawn_time)
                {
                    f32 explode_time = -1.0f;
                    f32 normalized_scale = -1.0f;
                    if(has_song_time_elapsed(hub, song_script_point_fireworks_explode))
                    {
                        explode_time = random(rng, 25, 75)/100.0f;
                        normalized_scale = random(rng, 5, 10)/100.0f;
                    }
                    
                    spawn_firework(hub, rng, gametime, explode_time, normalized_scale);
                    
                    // NOTE: Interval of fireworks launching increases as get to to exploding time
                    f32 explosion_interval_t = clamp01(get_sound_position(hub->boys_song) / song_script_point_girl_flashes);
                    
                    u32 start_interval = 50;
                    u32 end_interval = 125;
                    
                    start_interval = (u32)interpolate(50.0f, 5.0f, explosion_interval_t);
                    end_interval = (u32)interpolate(125.0f, 25.0f, explosion_interval_t);
                    
                    hub->next_firework_spawn_time = gametime + (random(rng, start_interval, end_interval) / 100.0f);
                }   
            }
        }        
        
        //
        // NOTE: Move and Draw Fireworks
        //
        for(Firework **firework_ptr = &hub->first_active_firework; *firework_ptr; )
        {
            Firework *f = *firework_ptr;
            b32 free_firework = false;
            
            if(!f->exploded)
            {
                f->velocity.y -= 9.8f * input->dt * 0.25f;
                f->position += f->velocity * input->dt * 50.0f;
                f32 alpha = 0.35f;
                
                if(f->fade_out)
                {
                    alpha = clamp01(f->explode_time - gametime) / firework_fade_out_time;
                }
                else
                {
                    if(gametime >= f->explode_time)
                    {
                        f->exploded = true;
                    }
                }
                
                Vec2 dir = normalize(f->velocity);
                f32 rot = atan2(dir.y, dir.x) - PI/2.0f;
                
                push_triangle_outline(render_commands, f->position, vec2(f->radius), vec4(safe_element_colour*0.5f, alpha), thickness_transform(8.0f, draw_order_transform(entity_draw_order_background + 1, rotation_transform(rot, vec2(f->radius*0.5f)))));
            }
            else
            {
                Temporary_Memory temp_mem = begin_temporary_memory(&tran_state->tran_arena);
                
                f->radius += input->dt * 75.0f;
                
                f32 extra_thickness = clamp01(f->radius / (f->max_radius * 0.5f)) * 16.0f;
                
                f32 alpha = 1.0f;
                f32 life_fade_threshold = 0.6f;
                f32 life_t = (clamp01(f->radius / f->max_radius));
                if(life_t > life_fade_threshold)
                {
                    alpha = (1.0f - (clamp01((life_t - life_fade_threshold) / (1.0f - life_fade_threshold))) );
                }
                
                
                Render_Transform transform = thickness_transform(16.0f + extra_thickness, draw_order_transform(entity_draw_order_background + 1));
                
                Vec4 colour = vec4(f->colour, alpha);
                Vec2 position = f->position - (f->radius - 32.0f)/2.0f;
                Vec2 scale = vec2(f->radius);
                
                Vec2 *points = 0;
                u32 points_count = 0;
                
                switch(f->shape)
                {
                    case 0:
                    {
                        Star_Points star = get_star_points();
                        points_count = array_count(star.points);
                        
                        points = push_array(&tran_state->tran_arena, Vec2, points_count);
                        copy_memory(points, star.points, sizeof(Vec2)*points_count);
                    } break;
                    
                    case 1:
                    {
                        Hexagon_Points hexagon = get_hexagon_points();
                        points_count = array_count(hexagon.points);
                        
                        points = push_array(&tran_state->tran_arena, Vec2, points_count);
                        copy_memory(points, hexagon.points, sizeof(Vec2)*points_count);
                    } break;
                    
                    case 2:
                    {
                        Dodecagon_Points dodecagon = get_dodecagon_points();
                        points_count = array_count(dodecagon.points);
                        
                        points = push_array(&tran_state->tran_arena, Vec2, points_count);
                        copy_memory(points, dodecagon.points, sizeof(Vec2)*points_count);
                    } break;
                    
                    case 3:
                    {
                        Vec2 triangle[] =
                        {
                            vec2(0.0f, 0.0f),
                            vec2(1.0f, 0.0f),
                            vec2(0.5f, 1.0f)
                        };
                        points_count = array_count(triangle);
                        
                        points = push_array(&tran_state->tran_arena, Vec2, points_count);
                        copy_memory(points, triangle, sizeof(Vec2)*points_count);
                    } break;
                    
                    case 4:
                    {
                        Vec2 rect[] =
                        {
                            vec2(0.0f, 0.0f),
                            vec2(1.0f, 0.0f),
                            vec2(1.0f, 1.0f),
                            vec2(0.0f, 1.0f)
                        };
                        points_count = array_count(rect);
                        
                        points = push_array(&tran_state->tran_arena, Vec2, points_count);
                        copy_memory(points, rect, sizeof(Vec2)*points_count);
                    } break;
                    
                    invalid_default_case;
                };
                
                // NOTE: Glow
                {
                    Vec2 glow_scale = scale*2.0f;
                    Vec2 glow_position = position - (glow_scale - scale)/2.0f;
                    
                    push_shape_blocky_blend(render_commands, glow_position, glow_scale, colour, vec4(colour.rgb, 0.1f*colour.a), points, points_count, transform);
                }
                
                transform_vertices(points, points, points_count, position, scale);
                
                push_line_points(render_commands, points, points_count, colour, transform_flags(render_transform_flag_wrap_line_points | render_transform_flag_joined_outline, transform));
                
                if(f->radius > f->max_radius)
                {
                    free_firework = true;
                }
                
                end_temporary_memory(temp_mem);
            }
            
            if(free_firework)
            {
                // NOTE: Free it
                *firework_ptr = f->next;
                f->next = hub->first_free_firework;
                hub->first_free_firework = f;
            }
            else
            {
                firework_ptr = &f->next;                
            }
        }
        
    }
    
    //
    // NOTE: Game Hub Update
    //
    {        
        switch(hub->state)
        {            
            case game_hub_state_girl_fades_in:
            {
                f32 state_t = clamp01(hub_state_elapsed_time(hub, gametime) / time_for_girl_to_fade_in);
                hub->girl_side_background_colour = interpolate(girl_side_intro_background_colour, girl_side_normal_background_colour, state_t);
                
                if(has_hub_state_time_elapsed(hub, gametime, girl_fades_in_state_time))
                {
                    set_anim(&hub->boy_face.anim, anim_noticing_girl);
                    set_hub_state(hub, game_hub_state_boy_notices_girl, gametime);                    
                }
                
            } break;
            
            case game_hub_state_boy_notices_girl:
            {
                // NOTE: Wait until last anim finished being doing this one
                if(is_current_anim_idle(&hub->boy_face.anim))
                {
                    set_anim(&hub->boy_face.anim, anim_about_to_speak);
                    play_sound(audio, asset_boy_about_to_speak);
                    
                    set_hub_state(hub, game_hub_state_boy_attempts_chat, gametime);    
                }
            } break;
            
            case game_hub_state_boy_attempts_chat:
            {                
                //if(hub->boy_face.anim.current == face_anim_sad_idle)
                if(has_hub_state_time_elapsed(hub, gametime, 8.0f))
                {
                    // NOTE: Unlock first level
                    set_hub_state(hub, game_hub_state_normal, gametime);
                    hub->level_selects[0].unlocked = true;
                    pick_next_time_to_play_boys_special_idle_anim(&hub->boy_face, gametime, rng);
                    
                    game_progress->levels_unlocked = 1;
                    
                    hub->background_melody_last_played_time = (gametime - background_melody_interval) + 3.0f;
                    
                    result.action = game_mode_result_action_save;
                }
                
            } break;
            
            case game_hub_state_normal:
            {
                
            } break;
            
            case game_hub_state_boy_prepping_to_sing:
            {
                activate_flash(&hub->boy_face.colour_flash, face_fast_flash_value_time);
                
                // NOTE: Change background colour
                hub->girl_side_background_colour = move_from_initial_to_target(girl_side_normal_background_colour, girl_side_boy_singing_background_colour, hub->girl_side_background_colour, input->dt, prepping_to_sing_state_time);
                
                fade_hub_ui(hub, input->dt, -1);
                
                // NOTE: First play whistle, then when done whistling, start main song, and move to boy singing state
                if(boy_face->whistling)
                {
                    if(hub->boys_song->playing)
                    {
                        if(has_song_time_elapsed(hub, song_script_point_start_singing))
                        {
                            set_hub_state(hub, game_hub_state_boy_singing, gametime);
                            boy_face->whistling = false;
                        }                    
                    }
                    else
                    {
                        if(has_hub_state_time_elapsed(hub, gametime, 7.0f))
                        {
                            set_anim(&hub->boy_face.anim, anim_song);
                            play_sound(hub->boys_song);
                        }   
                    }
                }
                else
                {
                    if(has_hub_state_time_elapsed(hub, gametime, 3.0f))
                    {
                        play_sound(hub->boys_song_whistle);
                        set_anim(&hub->boy_face.anim, anim_song_whistle);
                        
                        boy_face->whistling = true;
                    }
                }
                
            } break;
            
            case game_hub_state_boy_singing:
            {
                // NOTE: Just making sure none of our script points are outside of
                // the song length
                {
                    f32 song_length = get_sound_length(hub->boys_song);
                    assert(song_script_point_girl_turns_around < song_length
                           && song_script_point_girl_flashes < song_length);
                }
                
                hub->girl_side_background_colour = girl_side_boy_singing_background_colour;
                fade_hub_ui(hub, input->dt, -1);
                
                if(has_song_time_elapsed(hub, song_script_point_girl_turns_around))
                {
                    girl_face->movement_flags |= girl_movement_flag_turn_to_boy;
                }
                
                if(has_song_time_elapsed(hub, song_script_point_girl_flashes))
                {
                    activate_flash(&girl_face->colour_flash, face_slow_flash_value_time);
                }
                
                if(sound_stopped(hub->boys_song))
                {
                    end_boy_singing(hub, gametime);
                    result.action = game_mode_result_action_save;
                }
                
            } break;
            
            case game_hub_state_boy_girl_talk:
            {
                // NOTE: Background going back to normal colour
                hub->girl_side_background_colour = move_from_initial_to_target(girl_side_boy_singing_background_colour, girl_side_normal_background_colour, hub->girl_side_background_colour, input->dt, prepping_to_sing_state_time);
                
                // NOTE: UI elements fade back in
                fade_hub_ui(hub, input->dt, 1);
                
                girl_face->movement_flags |= (girl_movement_flag_move_to_boy | girl_movement_flag_turn_to_boy);
                
            } break;
        };
    }
    
    // NOTE: Background: Two sides, white and black that fade into each other
    {
        Temporary_Memory temp_mem = begin_temporary_memory(&tran_state->tran_arena);
        
        Render_Transform bkg_render_transform = draw_order_transform(entity_draw_order_background);
        
        Vec4 col_left_side = vec4(1.0f);
        Vec4 col_right_side = vec4(hub->girl_side_background_colour, 1.0f);
        
        f32 center_left = 0.5525f;
        f32 center_right = 0.695f;
        
        s32 blending_pieces = 6;
        // 6 blend_pieces + 1 left_quad + 1 right_quad
        s32 total_quads = blending_pieces + 2;
        s32 total_verts = total_quads*6;
        Vertex_XY_RGBA *verts = push_array(&tran_state->tran_arena, Vertex_XY_RGBA, total_verts);
        Vertex_XY_RGBA *v = verts;
        
        f32 fade_width = virtual_screen_width*(center_right-center_left);
        
        f32 block_width = (virtual_screen_width / 2.0f) - fade_width;
        f32 top = virtual_screen_height;
        f32 left_max_x = virtual_screen_width*center_left;
        f32 center_min_x = left_max_x;
        f32 center_max_x = virtual_screen_width*center_right;
        f32 right_min_x = center_max_x;
        f32 right_max_x = virtual_screen_width;
        
        // NOTE: Left Side
        {
            *v++ = vertex_xy_rgba(vec2(0.0f), col_left_side);
            *v++ = vertex_xy_rgba(vec2(left_max_x, 0.0f), col_left_side);
            *v++ = vertex_xy_rgba(vec2(0.0f, top), col_left_side);
            *v++ = vertex_xy_rgba(vec2(0.0f, top), col_left_side);
            *v++ = vertex_xy_rgba(vec2(left_max_x, 0.0f), col_left_side);
            *v++ = vertex_xy_rgba(vec2(left_max_x, top), col_left_side);
        }
        
        // NOTE: Center Blending
        {
            
            v =
                add_blocky_blend_verts(v, col_left_side, col_right_side, vec2(center_min_x, 0.0f), vec2(center_max_x, 0.0f), vec2(center_min_x, top), vec2(center_max_x, top), transform_flags(render_transform_flag_blend_exclude_first_colour | render_transform_flag_blend_exclude_second_colour, bkg_render_transform));
            
        }
        
        // NOTE: Right Side
        {
            *v++ = vertex_xy_rgba(vec2(right_min_x, 0.0f), col_right_side);
            *v++ = vertex_xy_rgba(vec2(right_max_x, 0.0f), col_right_side);
            *v++ = vertex_xy_rgba(vec2(right_min_x, top), col_right_side);
            *v++ = vertex_xy_rgba(vec2(right_min_x, top), col_right_side);
            *v++ = vertex_xy_rgba(vec2(right_max_x, 0.0f), col_right_side);
            *v++ = vertex_xy_rgba(vec2(right_max_x, top), col_right_side);            
        }
        assert((v - verts) == total_verts);
        
        push_vertices(render_commands, (f32 *)verts, total_verts, vertex_flag_xy | vertex_flag_rgba, vec4(1.0f), bkg_render_transform);
        
        end_temporary_memory(temp_mem);
    }
    
    //
    // NOTE: Girl Face
    //
    update_and_draw_girl_face(rng, girl_face, render_commands, input->dt, 1.0f, gametime, ui_context, hub);
    
    //
    // NOTE: Hub Progress Button
    //
    {
        Vec3 button_base_colour = safe_element_colour;
        
        Hub_Progress_Button *progress_button = &hub->progress_button;
        b32 beaten_all_scores = true;
        for(int score_index = 0; score_index < number_of_levels; ++score_index)
        {
            if(game_progress->best_level_scores[score_index] < hub->level_scores_to_get[score_index])
            {                
                beaten_all_scores = false;
                break;
            }
        }
        
        b32 progress_button_enabled = false;
        if(is_boy_singing(hub))
        {
            // NOTE: Only allow button click during singing if we've already sang (so can skip singing)
            if(hub->boy_face.has_sang)
            {
                progress_button_enabled = true;
                button_base_colour = danger_element_colour*0.75f;
            }
            
            stop_after_current_flash_ends(&progress_button->flash);
        }
        else
        {
            if(beaten_all_scores)
            {
                progress_button_enabled = true;
                
                if(hub->boy_face.has_sang)
                {
                    stop_after_current_flash_ends(&progress_button->flash);
                }
                else
                {
                    activate_flash(&progress_button->flash);
                }                
            }
            
            if(is_hub_state(hub, game_hub_state_boy_alone))
            {
                progress_button_enabled = true;
                activate_flash(&progress_button->flash);
            }
        }
        
        Dodecagon_Points progress_button_shape = get_dodecagon_points();
        transform_vertices(progress_button_shape.points, progress_button_shape.points, array_count(progress_button_shape.points), progress_button->position, progress_button->scale);
        
        UI_Id progress_button_ui_id = make_ui_id(&progress_button->position);
        if(progress_button_enabled)
        {
            b32 cursor_inside_button = point_vs_polygon(cursor->position, progress_button_shape.points, array_count(progress_button_shape.points), progress_button->position, progress_button->scale);
            
            if(do_button_logic(ui_context, progress_button_ui_id, cursor_inside_button))
            {
                //
                // NOTE: Click behaviour
                //
                if(is_boy_singing(hub))
                {
                    end_boy_singing(hub, gametime);
                }
                else if(beaten_all_scores)
                {
                    // NOTE: Boy sings bit
                    set_hub_state(hub, game_hub_state_boy_prepping_to_sing, gametime);
                    set_idle_anim(&hub->boy_face.anim, anim_closing_eyes_and_mouth);
                    
                    fade_out_sound(hub->background_ambience, stage_transition_sound_fade_duration);
                    
                    // NOTE: Stop girl saying credits if she is
                    set_volume(hub->girl_saying_credits, 0.0f);
                    hub->girl_face.saying_credits = false;
                    
                    stop_sound(hub->boys_song);
                    stop_sound(hub->boys_song_whistle);
                    
                    cancel_non_idle_anim(&hub->girl_face.anim, false);
                }
                else
                {
                    // NOTE: Girl fades in bit
                    set_hub_state(hub, game_hub_state_girl_fades_in, gametime);
                    play_sound(audio, asset_girl_fades_in);
                }
                
                stop_after_current_flash_ends(&progress_button->flash);
            }
        }
        
        b32 intro = is_hub_state(hub, game_hub_state_boy_alone);                       
        
        if(update_flashing_value(&progress_button->flash, input->dt))
        {
            // NOTE: We play a different sound for when it's flashing from
            // having beaten scores or when we're at beginning
            if(beaten_all_scores)
            {
                play_sound(audio, asset_level_score_beaten, 0, 0.5f, 0, 0.05f);
            }
            else
            {
                play_sound(audio, asset_level_select_hover, 0, 0.5f);
            }
        }
        
        f32 progress_button_fade = hub->ui_fade.progress_button;
        Render_Transform transform = thickness_transform(hub_entity_line_thickness*0.75f);
        Vec3 button_colour = select_ui_item_colour(ui_context, progress_button_ui_id, button_base_colour*0.7f, vec3(0.5f), vec3(0.35f));
        Vec3 button_bright_spot_colour = button_colour*1.5f;
        // NOTE: Draw "button"
        {
            if(hub->state == game_hub_state_boy_alone || beaten_all_scores)            
            {
                push_shape_points_hollow(render_commands, progress_button_shape.points, array_count(progress_button_shape.points), vec4(button_colour, 1.0f*hub->ui_fade.progress_button), vec4(button_bright_spot_colour, 1.0f*progress_button_fade), vec2(-0.25f, 0.25f));
            }
            else
            {
                push_shape_points_outline(render_commands, progress_button_shape.points, array_count(progress_button_shape.points), vec4(button_colour, 1.0f*progress_button_fade), transform_flags(render_transform_flag_joined_outline, transform));
            }
        }
        
        // NOTE: Flash
        if(is_flash_active(&progress_button->flash))
        {
            f32 flash_alpha = interpolate(1.0f, 0.0f, progress_button->flash.t)*hub->ui_fade.progress_button;
            Vec2 flash_large_scale = progress_button->scale*4.0f;
            Vec2 flash_scale = interpolate(progress_button->scale, flash_large_scale, progress_button->flash.t);
            Vec2 flash_position = progress_button->position + progress_button->scale/2.0f - flash_scale/2.0f;
            
            // NOTE: We draw a whole circle or outline based on if flashing at start or when beaten all scores
            if(beaten_all_scores)
            {
                push_dodecagon(render_commands, flash_position, flash_scale, vec4(button_colour, flash_alpha), transform);   
            }
            else
            {
                push_dodecagon_outline(render_commands, flash_position, flash_scale, vec4(button_colour, flash_alpha), transform_flags(render_transform_flag_joined_outline, transform));
            }
        }
        
        // NOTE: Draw glow
        {
            Vec2 glow_scale = progress_button->scale*1.75f;
            Vec2 glow_position = progress_button->position - (glow_scale - progress_button->scale)/2.0f;
            
            push_dodecagon_blocky_blend(render_commands, glow_position, glow_scale, vec4(button_base_colour*0.25f*progress_button_fade, 0.0f), vec4(button_base_colour*0.05f*progress_button_fade, 0.0f), transform_flags(render_transform_flag_additive_blend));
        }
        
        // NOTE: Draw goals
        if(!intro)
        {
            Vec3 flash_colour = interpolate(safe_element_colour, vec3(0.5f), hub->score_meter_goal_flash.t);
            
            // NOTE: One "goal" for each level
            for(int level_index = 0; level_index < number_of_levels; ++level_index)
            {
                Vec2 position = progress_button->goal_positions[level_index];
                
                transform = rotation_transform((PI*2.0f) * ((f32)level_index / (f32)number_of_levels), progress_button->goal_scale/2.0f, transform);
                
                if(beaten_level_score(game_progress, hub, level_index))
                {
                    push_triangle(render_commands, position, progress_button->goal_scale, vec4(flash_colour, 0.75f*progress_button_fade), transform);
                }
                
                push_triangle_outline(render_commands, position, progress_button->goal_scale, vec4(vec3(0.75f), 1.0f*progress_button_fade), transform);
                
                if(beaten_level_score(game_progress, hub, level_index))
                {                    
                    Vec4 center_colour = vec4(flash_colour*0.5f*progress_button_fade, 1.0f);
                    Vec4 edge_colour = vec4(0.0f*progress_button_fade, 0.0f, 0.0f, 0.0f);
                    
                    Vec2 glow_scale = progress_button->goal_scale*1.5f;
                    Vec2 glow_position = position - (glow_scale - progress_button->goal_scale)/2.0f;
                    
                    Render_Transform glow_transform = transform;
                    glow_transform.rotation_pt = glow_scale/2.0f;
                    
                    push_triangle_blocky_blend(render_commands, glow_position, glow_scale, center_colour, edge_colour, draw_order_transform(entity_draw_order_floating_score_meter, transform_flags(render_transform_flag_additive_blend, glow_transform)));
                }
            }
        }
        
    }
    
    // NOTE: Boy Face
    {        
        Render_Transform transform = thickness_transform(hub_entity_line_thickness, transform_flags(render_transform_flag_half_line_corner_indent, draw_order_transform(entity_draw_order_face)));
        
        Vec4 colour = vec4(0.0f, 0.0f, 0.0f, 1.0f);
        
        Boy_Face *face = &hub->boy_face;
        
        // NOTE: Boy and girl colour flash syncs when their both active and when their flash is at the
        // same speed
        if(is_flash_active(&face->colour_flash) && is_flash_active(&hub->girl_face.colour_flash)
           && face->colour_flash.time_for_one_flash == hub->girl_face.colour_flash.time_for_one_flash)
        {
            sync_flashing_value(&face->colour_flash, &hub->girl_face.colour_flash);
        }
        
        update_flashing_value(&face->colour_flash, input->dt);
        update_animation(&face->anim, input->dt);
        
        {
            // NOTE: Occasionally we want to play "special" idle animation, and we
            // play a vocal sound with it
            if(hub->state == game_hub_state_normal)
            {
                if(is_current_anim_idle(&face->anim) && gametime >= face->next_time_to_play_special_idle_anim)
                {
                    if(face->idle_anims_played >= array_count(face->idle_anims))
                    {
                        face->idle_anims_played = 0;
                    }
                    
                    u32 anims_available = (array_count(face->idle_anims) - face->idle_anims_played);
                    u32 anim_index = random_exclusive(rng, 0, anims_available);
                    
#if 1
                    Boy_Idle_Anim anim_selected = face->idle_anims[anim_index];
                    // NOTE: Swap the anim positions out so it's not in selectable range next time
                    {
                        s32 swap_index = ((array_count(face->idle_anims) - face->idle_anims_played) - 1);
                        assert(swap_index >= 0);
                        
                        Boy_Idle_Anim temp = face->idle_anims[swap_index];
                        face->idle_anims[swap_index] = face->idle_anims[anim_index];
                        face->idle_anims[anim_index] = temp;
                    }
                    ++face->idle_anims_played;
#else
                    Boy_Idle_Anim anim_selected = { anim_clears_throat, asset_boy_clears_throat };
#endif
                    
                    set_anim(&face->anim, anim_selected.anim);
                    if(anim_selected.sound)
                    {
                        play_sound(audio, anim_selected.sound);
                    }
                    
                    pick_next_time_to_play_boys_special_idle_anim(face, gametime, rng);
                }
            }
        }
        
        Key_Frame *frame = &face->anim.current_frame;
        
#if 0
        Key_Frame dud =
        {
            vec2(0.0f),
            1.0f,
            
            1.0f,
            0.0f
        };
        
        frame = dud;
#endif
        
        // NOTE: Head
        {            
            Render_Transform border_transform = transform;
            //border_transform.line_thickness += border_transform.line_thickness * face->colour_flash.t;
            Vec4 border_colour = vec4((vec3(0.0f) * (1.0f - face->colour_flash.t)) + (face->colour_flash.t * vec3(1.0f)), 1.0f);                
            
            push_rect_outline(render_commands, face->position, face->scale, border_colour, thickness_transform(hub_entity_line_thickness + 2.0f, transform));
        }
        
        // NOTE: Eyes
        {
            Vec2 direction = frame->eye_dir;
            f32 eye_move_radius = face->scale.y*0.075f;
            f32 size = frame->eye_size;
            
            f32 eye_full_height = face->scale.y*0.1f;
            
            // NOTE: Eyes
            Vec2 eyes_scale = vec2(face->scale.x*0.2f + transform.line_thickness*2.0f, eye_full_height*size + transform.line_thickness*2.0f);
            
            Vec2 left_eye_pos = face->position + (vec2(face->scale.x*0.3f, face->scale.y*0.75f) - eyes_scale*0.5f) + (direction*eye_move_radius);
            Vec2 right_eye_pos = face->position + vec2(face->scale.x*0.7f, face->scale.y*0.75f) - (eyes_scale*0.5f) + (direction*eye_move_radius);
            
            push_rect_outline(render_commands, left_eye_pos, eyes_scale, colour, transform);
            push_rect_outline(render_commands, right_eye_pos, eyes_scale, colour, transform);
            
        }
        
        // NOTE: Nose
        {
            // NOTE: This is how much the mouth affects the nose movement
            // (the bigger the mouth, the higher the nose goes)
            f32 nose_full_height = face->scale.y*0.225f;
            Vec2 nose_scale = vec2(face->scale.x*0.2f, nose_full_height - (nose_full_height*0.05f*frame->mouth_size));
            
            Vec2 nose_pos = face->position + vec2(face->scale.x*0.5f, face->scale.y*0.475f) - nose_scale/2.0f;
            nose_pos.y += (nose_full_height - nose_scale.y);
            
            Vec2 verts[] =
            {
                vec2(nose_pos.x, nose_pos.y + nose_scale.y),
                nose_pos,
                vec2(nose_pos.x + nose_scale.x, nose_pos.y),
                nose_pos + nose_scale,
            };
            
            push_line_points(render_commands, verts, array_count(verts), colour, transform);
        }
        
        // NOTE: Mouth
        {
            f32 size = frame->mouth_size;
            
            Vec2 mouth_scale = {};
            mouth_scale.x = face->scale.x*0.3f;
            mouth_scale.y = (mouth_scale.x * 0.4f * size) + transform.line_thickness;
            
            Vec2 mouth_pos = face->position + vec2(face->scale.x*0.5f, face->scale.y*0.2f) - mouth_scale/2.0f;
            
            f32 tip_offset = mouth_scale.y*0.5f + (frame->mouth_tip_dir * mouth_scale.y);
            f32 mouth_offset = face->scale.x*0.05f + (face->scale.x*0.025f /* ((tip_offset + 1.0f) / 2.0f)*/);
            
            Vec2 verts[] =
            {
                vec2(mouth_pos.x - mouth_offset, mouth_pos.y + tip_offset),
                // NOTE: Offseting here  gives him a slightly crooked mouth
                mouth_pos + transform.line_thickness*0.5f,
                vec2(mouth_pos.x + mouth_scale.x, mouth_pos.y),                
                vec2(mouth_pos.x + mouth_scale.x + mouth_offset, mouth_pos.y + tip_offset),                
                mouth_pos + mouth_scale,                
                vec2(mouth_pos.x, mouth_pos.y + mouth_scale.y),                
                vec2(mouth_pos.x - mouth_offset, mouth_pos.y + tip_offset),
            };
            
            push_line_points(render_commands, verts, array_count(verts), colour, transform);
            
        }
        
    }
    
    //
    // NOTE: Level Selects
    //
    assert(array_count(hub->level_scores_to_get) == array_count(hub->level_selects));
    Vec2 select_bl = vec2(hub->boy_face.position.x + hub->boy_face.scale.x/2.5f, hub->boy_face.position.y + hub->boy_face.scale.y/2.75f);
    Vec2 select_scale = hub->boy_face.scale*0.2f;
    {
        f32 below_point_y = select_bl.y + select_scale.y*0.55f;
        
        {
            Level_Select *select = &hub->level_selects[0];
            select->position = hub->boy_face.position + hub->boy_face.scale/10.0f; vec2(select_bl.x, below_point_y);
            select->level_type = game_level_type_connectors;
        }
        {
            Level_Select *select = &hub->level_selects[1];
            select->position = vec2(hub->boy_face.position.x + hub->boy_face.scale.x*0.825f, hub->boy_face.position.y + hub->boy_face.scale.y*0.2f);
            select->level_type = game_level_type_spikes_barrier;
        }
        {
            Level_Select *select = &hub->level_selects[2];
            select->position = hub->boy_face.position + vec2(hub->boy_face.scale.x/5.0f, hub->boy_face.scale.y*0.37f);
            select->level_type = game_level_type_spinners;
        }
        {
            Level_Select *select = &hub->level_selects[3];
            select->position = vec2(hub->boy_face.position.x + hub->boy_face.scale.x*0.75f, hub->boy_face.position.y + hub->boy_face.scale.y*0.495f);
            select->level_type = game_level_type_everything;
        }
        {
            Level_Select *select = &hub->level_selects[4];
            select->position = vec2(hub->boy_face.position.x + hub->boy_face.scale.x/3.9f, hub->boy_face.position.y + hub->boy_face.scale.y*0.835f);
            select->level_type = game_level_type_shield_intro;
        }
        {
            Level_Select *select = &hub->level_selects[5];
            select->position = vec2(hub->boy_face.position.x + hub->boy_face.scale.x/1.725f, hub->boy_face.position.y + hub->boy_face.scale.y*0.86f);
            select->level_type = game_level_type_shield_everything;
        }
    }
    
    //
    // NOTE: Level select
    //
    Temporary_Memory temp_mem = begin_temporary_memory(&tran_state->tran_arena);
    {
        assert((prepping_to_sing_level_select_ray_direction_change_start_time + prepping_to_sing_level_select_ray_direction_time) <= prepping_to_sing_state_time);
        
        if(game_progress->levels_unlocked > 0)
        {
            update_flashing_value(&hub->next_level_select_flash, input->dt);
        }
        
        f32 level_select_radius = hub->boy_face.scale.x*0.0375f;
        b32 level_select_enabled = (hub->state == game_hub_state_normal || hub->state == game_hub_state_boy_girl_talk);
        
        update_flashing_value(&hub->score_meter_fade, input->dt);
        
        b32 hovered_over_any_level_selects = false;
        for(int level_select_index = 0; level_select_index < array_count(hub->level_selects); ++level_select_index)
        {
            Level_Select *select = &hub->level_selects[level_select_index];
            select->scale = vec2(level_select_radius*2.0f);
            
            UI_Id ui_id = make_ui_id(select);
            
            Hexagon_Points level_select_shape = get_hexagon_points();
            transform_vertices(level_select_shape.points, level_select_shape.points, array_count(level_select_shape.points), select->position, select->scale);
            
            if(select->unlocked)
            {
                if(level_select_enabled)
                {
                    b32 cursor_inside_element = point_vs_polygon(cursor->position, level_select_shape.points, array_count(level_select_shape.points), select->position, select->scale);
                    if(do_button_logic(ui_context, ui_id, cursor_inside_element))
                    {
                        // NOTE: Switch levels
                        result.action = game_mode_result_action_switch_mode;
                        result.level_type = select->level_type;
                        result.level_score_to_get = hub->level_scores_to_get[select->level_type];                
                    }
                    
                    if(ui_item_hot(ui_context, ui_id))
                    {
                        hovered_over_any_level_selects = true;
                        hub->last_level_select_index_hovered_over = level_select_index;
                        restart_flashing_value(&hub->score_meter_fade);
                        
                        if(!ui_item_hot_or_active_last_frame(ui_context, ui_id))
                        {
                            play_sound(audio, asset_level_select_hover);
                        }
                    }
                    
                }
                
                Vec3 button_colour = select_ui_item_colour(ui_context, ui_id, vec3(0.0f), vec3(0.5f), vec3(0.75f));
                
                // Draw selector
                //if(level_select_enabled)
                {
                    // NOTE: Hair strands
                    {
                        //Vec2 hair_scale = vec2(select->scale.x/7.0f, select->scale.y*0.85f);
                        Vec2 hair_scale = vec2(select->scale.x*0.25f, select->scale.y*0.75f);
                        
                        //Vec2 hair_scale = vec2(select->scale.x*0.7f, select->scale.y*0.7f);
                        f32 start_rotation = -PI;
                        
                        // NOTE: Each strand points in the direction of each of the 6 hexagon edges
                        f32 rotations[] =
                        {
                            start_rotation,
                            start_rotation + 1.0f*hexagon_triangle_angle_radians,
                            start_rotation + 2.0f*hexagon_triangle_angle_radians,
                            start_rotation + 3.0f*hexagon_triangle_angle_radians,
                            start_rotation + 4.0f*hexagon_triangle_angle_radians,
                            start_rotation + 5.0f*hexagon_triangle_angle_radians,
                        };
                        
                        for(int hair_index = 0; hair_index < (level_select_index + 1); ++hair_index)
                        {
                            f32 rotation = rotations[hair_index];
                            Render_Transform transform = rotation_transform(rotation, vec2(hair_scale.x/2.0f, 0.0f), corner_indent_transform(2.0f));
                            
                            push_rect(render_commands, select->position + select->scale/2.0f - vec2(hair_scale.x/2.0f, 0.0f), hair_scale, vec4(button_colour, 0.8f*hub->ui_fade.selector_buttons), transform);
                            
                        }
                    }
                    
                    // NOTE: Draw Main Button
                    push_shape_points(render_commands, level_select_shape.points, array_count(level_select_shape.points), vec4(button_colour, 1.0f*hub->ui_fade.selector_buttons));
                    
                    // NOTE: Draw flash as outline of level select for next one to play
                    if(level_select_index == game_progress->levels_completed)
                    {
                        f32 t = hub->next_level_select_flash.t;
                        Vec2 large_scale = select->scale * 10.0f;
                        Vec2 scale = select->scale + ((large_scale - select->scale) * t);
                        Vec2 position = select->position - ((scale - select->scale)/2.0f);
                        
                        push_hexagon_outline(render_commands, position, scale, vec4(vec3(0.25f), (1.0f - t)*hub->ui_fade.selector_buttons), thickness_transform(2.0f));
                    }
                    
                    if(beaten_level_score(game_progress, hub, level_select_index))
                    {
                        Vec2 scale = select->scale*0.35f;
                        Vec2 position = select->position + (select->scale*0.5f) - equilateral_centroid(scale);
                        
                        Vec3 colour = interpolate(vec3(0.25f), safe_element_colour, hub->score_meter_goal_flash.t*hub->ui_fade.selector_buttons);
                        
                        {
                            Vec2 glow_scale = scale*2.0f;
                            f32 offset = (1.0f - (2.0f / 3.0f));
                            
                            Vec2 glow_position = vec2(position.x - ((glow_scale.x - scale.x)/2.0f),
                                                      position.y - ((glow_scale.y - scale.y)*offset) );
                            
                            
                            push_triangle_blocky_blend(render_commands, glow_position, glow_scale, vec4(colour*hub->ui_fade.selector_buttons, 0.0f), vec4(0.0f), transform_flags(render_transform_flag_additive_blend));
                        }
                        
                        push_triangle(render_commands, position, scale, vec4(colour, 1.0f*hub->ui_fade.selector_buttons));
                    }
                    
                }                
            }
            else
            {            
                push_hexagon_outline(render_commands, select->position, select->scale, vec4(0.0f, 0.0f, 0.0f, 1.0f), thickness_transform(hub_entity_line_thickness*0.75f));
            }                        
        }
        
        
        // NOTE: If not over any level selects, then fade out meter
        if(!hovered_over_any_level_selects)
        {
            stop_after_current_flash_ends(&hub->score_meter_fade);
        }
        
        // NOTE: Draw score meter
        if(is_flash_active(&hub->score_meter_fade))
        {
            int level_select_index = hub->last_level_select_index_hovered_over;
            assert(level_select_index >= 0 && level_select_index < array_count(hub->level_selects));
            
            Level_Select *select = &hub->level_selects[level_select_index];
            u32 best_score = game_progress->best_level_scores[level_select_index];
            
            f32 alpha = (1.0f - hub->score_meter_fade.t);
            draw_score_meter_floating(render_commands, select->position + select->scale, 25.0f, best_score, hub->level_scores_to_get[level_select_index], best_score, &hub->score_meter_goal_flash, 0.0f, alpha);
            
        }
    }
    end_temporary_memory(temp_mem);
    
    return result;
}
