#include "twosome.h"
#include "twosome_asset.cpp"
#include "twosome_render_group.cpp"
#include "twosome_audio.cpp"
#include "twosome_collision.cpp"
#include "twosome_imgui.cpp"
#include "twosome_game_level.cpp"
#include "twosome_animation.cpp"
#include "twosome_game_hub.cpp"
#include "twosome_pause_screen.cpp"


internal void save_game(Game_State *game_state, b32 fill_save_file = true)
{
    Game_Save save = {};
    save.header.version = 1;
    save.header.filled = fill_save_file;
    
    // NOTE: "Gather" save data from game state
    if(fill_save_file)
    {
        // NOTE: Game Progress
        {
            Game_Progress *progress = &game_state->progress;
            for(s32 best_score_index = 0; best_score_index < array_count(progress->best_level_scores); ++best_score_index)
            {
                save.best_level_scores[best_score_index] = progress->best_level_scores[best_score_index];
            }
            save.levels_unlocked = progress->levels_unlocked;
            save.levels_completed = progress->levels_completed;
        }
        
        // NOTE: Hub
        {
            Game_Hub *hub = &game_state->hub;

            save.hub_state = hub->state;
            save.girl_side_background_colour = hub->girl_side_background_colour;

            save.boy_has_sang = hub->boy_face.has_sang;
            save.boy_face_position = hub->boy_face.position;
            save.boy_idle_anim = hub->boy_face.anim.idle_anim;
            save.boy_flashing = is_flash_active(&hub->boy_face.colour_flash);
            save.boy_flash_time_for_one_flash = hub->boy_face.colour_flash.time_for_one_flash;

            save.girl_face_position = hub->girl_face.position;
            save.girl_face_rot_t = hub->girl_face.rot_t;
            save.girl_face_movement_flags = hub->girl_face.movement_flags;
            save.girl_flashing = is_flash_active(&hub->girl_face.colour_flash);
            save.girl_flash_time_for_one_flash = hub->girl_face.colour_flash.time_for_one_flash;
        }
    }
    
    // NOTE: Write save
    {
        Platform_Game_Save platform_game_save = {};
        platform_game_save.data = &save;
        platform_game_save.data_size = sizeof(save);
   
        platform.write_game_save_file(platform_game_save);
    }
}

internal void load_save_game(Game_State *game_state)
{
    Platform_Game_Save platform_game_save = {};
    
    Game_Save save;
    platform_game_save.data = &save;
    platform_game_save.data_size = sizeof(save);

    // NOTE: If there's a save data we can use we "apply" the save data to game state
    // otherwise leave it in initial state
    if(platform.load_game_save_file(platform_game_save))
    {
        assert(save.header.version == 1);
        if(save.header.version == 1 && save.header.filled)
        {
            // NOTE: Game Progress
            {
                Game_Progress *progress = &game_state->progress;
                for(s32 best_score_index = 0; best_score_index < array_count(progress->best_level_scores); ++best_score_index)
                {
                    progress->best_level_scores[best_score_index] = save.best_level_scores[best_score_index];
                }
            
                progress->levels_unlocked = save.levels_unlocked;
                progress->levels_completed = save.levels_completed;
            }

            // NOTE: Hub
            {
                Game_Hub *hub = &game_state->hub;
            
                hub->state = save.hub_state;
                hub->girl_side_background_colour = save.girl_side_background_colour;

                hub->boy_face.has_sang = save.boy_has_sang;
                hub->boy_face.position = save.boy_face_position;
                set_idle_anim(&hub->boy_face.anim, save.boy_idle_anim);
                jump_to_random_anim_frame(&hub->boy_face.anim, &game_state->rng);
                
                if(save.boy_flashing)
                {
                    activate_flash(&hub->boy_face.colour_flash, save.boy_flash_time_for_one_flash);
                }

                hub->girl_face.position = save.girl_face_position;
                hub->girl_face.rot_t = save.girl_face_rot_t;
                hub->girl_face.movement_flags = save.girl_face_movement_flags;
                if(save.girl_flashing)
                {
                    activate_flash(&hub->girl_face.colour_flash, save.girl_flash_time_for_one_flash);
                }                
            }   
        }                        
    }    
}

internal void set_game_mode(int game_mode, Game_State *game_state, Game_Mode_Result mode_switch, f32 gametime)
{
    zero_object(Pause_Screen, game_state->pause_screen);
    
    int last_game_mode = game_state->game_mode;
    game_state->game_mode = game_mode;
    game_state->cursor.position = vec2(virtual_screen_width*0.5f, virtual_screen_height*0.5f);
    
    game_state->game_mode_transition_t = 0.0f;
    game_state->game_mode_transition_direction = game_mode_transition_fade_in_direction;
    
    // NOTE: Shutdown last game mode
    switch(last_game_mode)
    {
        case game_mode_playing_level:
        {
            shutdown_level_stage(&game_state->level);
        } break;

        case game_mode_hub:
        {
            deactivate_game_hub(&game_state->hub);
        } break;
    };

    fade_out_all_sounds_owned_by_audio_system(&game_state->audio, stage_transition_sound_fade_duration);

    // NOTE: Clear out game mode arena, as this is never carried over on game mode changes
    clear(&game_state->mode_arena);
    
    // NOTE: Init new game mode
    switch(game_state->game_mode)
    {
        case game_mode_hub:
        {
            activate_game_hub(&game_state->hub, &game_state->audio, &game_state->progress, &game_state->cursor, gametime, &game_state->rng, &game_state->mode_arena);

            // NOTE: If we're coming from level, then want cursor hovering over level select of that level
            if(last_game_mode == game_mode_playing_level)
            {
                Level_Select *level_select = &game_state->hub.level_selects[mode_switch.level_type];
                game_state->cursor.position = (level_select->position + level_select->scale/2.0f);
            }
            
        } break;

        case game_mode_playing_level:
        {
            init_game_level(&game_state->level, mode_switch.level_type, &game_state->cursor, &game_state->audio, &game_state->mode_arena, mode_switch.level_score_to_get, game_state->progress.best_level_scores[mode_switch.level_type], &game_state->rng);
        } break;

        invalid_default_case;
    };
}

internal void change_game_mode(Game_State *game_state, Game_Mode_Result game_mode_result)
{
    // NOTE: We always save when the mode switches
    save_game(game_state);
            
    int new_game_mode = game_state->game_mode;
    if(game_state->game_mode == game_mode_hub)
    {
        new_game_mode = game_mode_playing_level;
    }
    else
    {
        new_game_mode = game_mode_hub;
    }
    set_game_mode(new_game_mode, game_state, game_mode_result, game_state->gametime);
}

internal Game_Render_View make_game_render_view(u32 window_width, u32 window_height)
{
    Game_Render_View view;
    
    view.width = window_width;
    view.height = window_height;

    // NOTE: Work out aspect ratio
    {        
        r64 target_aspect_ratio = 4.0/3.0;
        view.height = (s32)((r64)window_width / target_aspect_ratio);

        if(view.height > window_height)
        {
            view.height = window_height;
            view.width = (s32)((r64)window_height * target_aspect_ratio);
        }
                    
        view.x = (window_width - view.width) / 2;
        view.y = (window_height - view.height) / 2;
    }

    view.orthographic_matrix = orthographic_projection_matrix(0.0f, virtual_screen_width, 0.0f, virtual_screen_height, 0.0f, 1.0f);
    view.screen_orthographic_matrix = orthographic_projection_matrix(0.0f, (f32)view.width, 0.0f, (f32)view.height, 0.0f, 1.0f);
        
    return view;
}

internal Task_With_Memory *begin_task_with_memory(Transient_State *tran_state)
{
    TIMED_BLOCK();
    
    Task_With_Memory *found_task = 0;
    for(u32 task_index = 0; task_index < array_count(tran_state->tasks); ++task_index)
    {
        Task_With_Memory *task = &tran_state->tasks[task_index];
        if(!task->being_used)
        {
            found_task = task;
            task->being_used = true;
            task->memory_flush = begin_temporary_memory(&task->arena);
            break;
        }
    }

    return found_task;
}

internal void end_task_with_memory(Task_With_Memory *task)
{
    end_temporary_memory(task->memory_flush);

    complete_previous_writes_before_future_writes;
    task->being_used = true;
}

extern "C" GAME_UPDATE_AND_RENDER(game_update_and_render)
{    
    platform = memory->platform_api;
    set_global_log_state(platform);
    set_global_debug_state(memory);
    
    TIMED_BLOCK();
    
    Game_Update_And_Render_Result game_update_and_render_result = {};
    
    assert((&input->terminator - &input->buttons[0]) <= array_count(input->buttons));

    assert(sizeof(Game_State) <= memory->permanent_storage_size);
    Game_State *game_state = (Game_State *)memory->permanent_storage;
    assert(sizeof(Transient_State) <= memory->transient_storage_size);
    Transient_State *tran_state = (Transient_State *)memory->transient_storage;
    
    Game_Audio *game_audio = &game_state->audio;
    Cursor *cursor = &game_state->cursor;
    UI_Context *ui_context = &game_state->ui_context;

    // NOTE: If we moving to another game mode we're going to disable click so that
    // we can't interrupt it with another action (since clicking is how you make stuff happen
    // in this game)
    {
        if(game_state->game_mode_transition_direction == game_mode_transition_fade_out_direction)
        {
            for(u32 button_index = 0; button_index < array_count(input->buttons); ++button_index)
            {
                zero_object(Game_Button_State, input->buttons[button_index]);
            }
        }
    }
    
    //
    // NOTE: Transient initialization
    //
    if(!tran_state->is_initialized)
    {
        initialize_arena(&tran_state->tran_arena, memory->transient_storage_size - sizeof(Transient_State),
                         (uint8 *)memory->transient_storage + sizeof(Transient_State));
        TRACK_MEMORY_ARENA(tran_state->tran_arena);
        
        tran_state->work_queue = memory->work_queue;
        
        tran_state->assets = push_struct(&tran_state->tran_arena, Game_Assets);

        for(u32 task_index = 0; task_index < array_count(tran_state->tasks); ++task_index)
        {
            Task_With_Memory *task = &tran_state->tasks[task_index];

            task->being_used = false;
            sub_arena(&task->arena, &tran_state->tran_arena, megabytes(1));
            TRACK_MEMORY_ARENA(task->arena);
        }

        sub_arena(&tran_state->assets->perm_arena, &tran_state->tran_arena, megabytes(128));
        // NOTE: Check that we've got plenty left for temporary memory allocations
        assert(get_arena_size_remaining(&tran_state->tran_arena) >= megabytes(50));
        TRACK_MEMORY_ARENA(tran_state->assets->perm_arena);

        initialize_game_assets(tran_state);

        // NOTE: Check our animation store is up-to-date
        assert(array_count(animation_store) == anim_count);
        
        tran_state->is_initialized = true;
    }
    
#if TWOSOME_INTERNAL
    if(first_button_press(input->DEBUG_toggle_mute_button))
    {
        if(game_audio->master_volume == 0.0f)
        {
            game_audio->master_volume = 1.0f;            
        }
        else
        {
            game_audio->master_volume = 0.0f;
        }
    }
    
#endif          
    
    render_commands->view = make_game_render_view(render_commands->window_width, render_commands->window_height);
    render_commands->clear_colour = game_clear_colour;
    render_commands->assets = tran_state->assets;
    render_commands->temp_arena = &tran_state->tran_arena;
    
    //
    // NOTE: Game State initialization
    //
    if(!game_state->is_initialized)
    {
        Memory_Arena total_arena;
        initialize_arena(&total_arena, memory->permanent_storage_size - sizeof(Game_State), (uint8 *)memory->permanent_storage + sizeof(Game_State));
 
        sub_arena(&game_state->audio_arena, &total_arena, megabytes(2));
        TRACK_MEMORY_ARENA(game_state->audio_arena);
                
        sub_arena(&game_state->mode_arena, &total_arena, get_arena_size_remaining(&total_arena));
        TRACK_MEMORY_ARENA(game_state->mode_arena);
        
        // NOTE: Seed random number generator
        {
            Platform_Date_Time time = platform.get_current_time();
            u64 first_seed = (u64)(time.milliseconds + time.second + time.second + time.minute + time.hour + time.day + time.month + time.year) ^ (intptr)&seed_rng;
            u64 second_seed = (uintptr)platform.get_current_time;
            seed_rng(&game_state->rng, first_seed, second_seed);
        }
        
        init_game_audio(game_audio, &game_state->audio_arena, tran_state->assets, input->settings.initial_master_volume, &game_state->rng);        

        game_state->game_mode = game_mode_loading_assets;

        game_state->is_initialized = true;
    }
    
    // NOTE: Calculate Time
    ++game_state->total_frame_count;
    f32 realtime = (f32)((f64)game_state->total_frame_count * (f64)input->dt);    
        
    b32 restart_game = false;

    // NOTE: Move Mouse
    {
        cursor->x += input->mouse_x;
        cursor->y += input->mouse_y;
        cursor->position = clamp_v2(cursor->position, vec2(0), vec2(virtual_screen_width, virtual_screen_height));
    }

    imgui_frame_start(ui_context, cursor, input, realtime);
    
    if(game_state->game_mode == game_mode_loading_assets)
    {
        if(tran_state->assets->asset_load_result == game_assets_load_result_success)
        {
            init_game_hub(&game_state->hub, game_audio, 0.0f, &game_state->rng);
            load_save_game(game_state);            
            
#if TWOSOME_INTERNAL
            // NOTE: For jumping around game
#if 1
            game_state->progress.levels_unlocked = number_of_levels;
            game_state->progress.levels_completed = number_of_levels;
            if(game_state->hub.state < game_hub_state_normal)
            {
                set_hub_state(&game_state->hub, game_hub_state_normal, 0.0f);
                game_state->hub.girl_side_background_colour = vec3(0.0f);
            }
          
            for(int level_index = 0; level_index < array_count(game_state->progress.best_level_scores); ++level_index)
            {
                game_state->progress.best_level_scores[level_index] = 9000;
            }

            //game_state->progress.best_level_scores[number_of_levels - 1] = 0;

#endif

            //set_anim(&game_state->hub.boy_face.anim, face_anim_happy_idle);
            //set_anim(&game_state->hub.boy_face.anim, face_anim_chuckle);
        
#endif

            
            Game_Mode_Result mode_switch = {};
            set_game_mode(game_mode_hub, game_state, mode_switch, 0.0f);   
        }

        game_update_and_render_result.game_assets_load_result = tran_state->assets->asset_load_result;
    }
    else
    {        
        Pause_Screen *pause_screen = &game_state->pause_screen;
        
        //
        // NOTE: Game Mode Update
        //    
        Game_Mode_Result game_mode_result = {};        
        
        // NOTE: Game level change cursor colour so need to start with a default
        cursor->colour = vec3(0.0f, 0.5f, 0.0f);
        
#if 1 || !TWOSOME_INTERNAL
        //
        // NOTE: Pause Screen
        //
        {
            b32 toggle_pause_input_recieved = first_button_press(input->pause_button);
            // NOTE: We ignore the pause button if we're in the pause screen and not
            // in the main screen, as the pause update will use it for moving back screens
            if(pause_screen->active && !on_main_pause_screen(pause_screen))
            {
                toggle_pause_input_recieved = false;
            }
            
            if(toggle_pause_input_recieved || pause_screen->pre_update_toggle_action)
            {
                if(pause_screen->active)
                {
                    if(pause_screen->pre_update_toggle_action == pause_screen_pre_update_toggle_action_quit_level)
                    {
                        assert(game_state->game_mode == game_mode_playing_level);
                        game_mode_result.level_type = game_state->level.type;
                        game_mode_result.action = game_mode_result_action_switch_mode;   
                    }
                    
                    shutdown_pause_screen(pause_screen, game_audio, game_mode_result, game_state);
                }
                else
                {
                    assert(!pause_screen->pre_update_toggle_action);
                    initialize_pause_screen(pause_screen, game_audio, game_state);
                }
            }
            
            Pause_Screen_Update_Action pause_update_action = update_pause_screen(pause_screen, game_state, input, ui_context, render_commands, game_audio, cursor, &game_mode_result);

            // NOTE: If any kind of pause action occurs then save the settings - what could possibly go wrong!
            if(pause_update_action || pause_screen->pre_update_toggle_action || toggle_pause_input_recieved)
            {
                Game_Settings settings = game_settings(pause_screen->master_volume_setting, input->settings.fullscreen, input->settings.vsync);
                write_game_settings_to_file(settings, &platform, &tran_state->tran_arena);
            }
            
            switch(pause_update_action)
            {
                case pause_screen_update_action_null:
                {
                } break;
                
                case pause_screen_update_action_toggle_fullscreen:
                {
                    game_update_and_render_result.toggle_fullscreen = true;
                } break;
            
                case pause_screen_update_action_restart_game:
                {
                    restart_game = true;
                } break;

                case pause_screen_update_action_quit_game:
                {
                    game_update_and_render_result.quit_game = true;
                } break;

                invalid_default_case;
            };
        }

        //
        // NOTE: Game Mode Update
        //
        if(!pause_screen->active && game_mode_result.action == game_mode_result_action_null)
        {
            ++game_state->playing_frame_count;
            game_state->gametime = (f32)((f64)game_state->playing_frame_count * (f64)input->dt);

            switch(game_state->game_mode)
            {        
                case game_mode_playing_level:
                {
                    game_mode_result = update_game_level(&game_state->level, input, &tran_state->tran_arena, render_commands, &game_state->progress, ui_context, cursor, game_state->gametime);                                
                } break;

                case game_mode_hub:
                {
                    game_mode_result = update_game_hub(&game_state->hub, tran_state, input, render_commands, game_audio, cursor, &game_state->progress, ui_context, &game_state->rng, game_state->gametime);
                } break;

                invalid_default_case;
            }
        }
    
        //
        // NOTE: Game Mode Return
        //
        {
            // NOTE: Save
            if(game_mode_result.action == game_mode_result_action_save)
            {
                save_game(game_state);
            }
            // NOTE: Mode switch
            if(game_mode_result.action == game_mode_result_action_switch_mode)
            {
                play_sound(game_audio, asset_level_stage_transition, playing_sound_flag_no_fade);
                        
                // NOTE: Don't want to do stage transition if changing game mode from pause screen
                if(pause_screen->active)
                {
                    change_game_mode(game_state, game_mode_result);
                }
                else
                {
                    game_state->game_mode_transition_direction = game_mode_transition_fade_out_direction;
                    game_state->game_mode_transition_mode_result = game_mode_result;
                }
            }
        }

        //
        // NOTE: Render Cursor
        //
        {
            Vec2 scale = vec2(5.0f);
            Vec2 position = cursor->position - scale/2.0f;
            f32 axis_size = 50.0f;
        
            f32 axis_alpha = 0.5f;

            Render_Transform transform = draw_order_transform(entity_draw_order_ui, corner_indent_transform(scale.x/4.0f));

            // NOTE: Cursor Axis (Up, Down, Left, Right)
            push_rect(render_commands, position + vec2(0, scale.y), vec2(scale.x, axis_size), vec4(cursor->colour, axis_alpha), transform);
            push_rect(render_commands, position - vec2(0, axis_size), vec2(scale.x, axis_size), vec4(cursor->colour, axis_alpha), transform);
            push_rect(render_commands, position - vec2(axis_size, 0), vec2(axis_size, scale.y), vec4(cursor->colour, axis_alpha), transform);
            push_rect(render_commands, position + vec2(scale.x, 0), vec2(axis_size, scale.y), vec4(cursor->colour, axis_alpha), transform);
        
            // NOTE: Cursor
            push_rect(render_commands, position, scale, vec4(cursor->colour, 1.0f), transform);        
        }
    
#else
        //
        // NOTE: For testing out visuals & sound
        //
    
        local_persist f32 rotation = 0.0f;//-PI*0.25f;

        f32 alpha = 1.0f;//fade;

        push_rect(render_commands, vec2(100.0f, 101.0f), vec2(200.0f, 201.0f), vec4(1.0f), rotation_transform(rotation, vec2(200.0f, 201.0f)/2.0f));

        rotation += input->dt;

        
#if 0
        if(!game_state->test_overlapping_sound)
        {
            game_state->test_overlapping_sound = load_overlapping_sound(game_audio, asset_hub_ambience, 0, true);
            play_sound(game_state->test_overlapping_sound, 8.0f);
        }

        DEBUG_draw_overlapping_sound_debug_info(render_commands, game_state->test_overlapping_sound);        
        
        if(first_button_press(input->change_colour_button))
        {
            fade_out_sound(game_state->test_overlapping_sound, 1.0f);
        }
        if(first_button_press(input->activate_shield_button))
        {
            fade_in_sound(game_state->test_overlapping_sound, 1.0f);
        }
        
        
#endif
       
#if 1
        {
#if 1
            if(!game_state->test_sound)
            {
                game_state->test_sound = load_sound(game_audio, asset_spinner_laser,
                                                    playing_sound_flag_loop | playing_sound_flag_reverse, 0);
                play_sound(game_state->test_sound, /*0.56f*/1.0f);
            }

            Playing_Sound *s = game_state->test_sound;

            f32 second_change = 1.0f / 30.0f; // 0.25f;
            f32 volume = cursor->position.y / virtual_screen_width;
            push_rect(render_commands, vec2(virtual_screen_width/2.0f, cursor->position.y), vec2(10.0f), vec4(1.0f));
            volume = clamp01(volume);
            //set_volume(s, volume, second_change);
            set_play_speed(s, 0.1f + (volume*8.0f));
            
            //set_play_speed(s, 4.21f);


            DEBUG_draw_sound_debug_info(render_commands, game_state->test_sound, 100, 100);
            
#endif
            
#if 0
            if(first_button_press(input->change_colour_button))
            {
                //set_play_speed(game_state->test_sound, 8.0f);
#if 1
                if(s->playing_flags & playing_sound_flag_reverse)
                {
                    set_direction(s, 1);
                }
                else
                {
                    set_direction(s, -1);
                    //set_play_speed(s, 8);
                }
#endif
                
                //fade_out_sound(s, 1.0f);
            }
            else if(first_button_press(input->activate_shield_button))
            {
                //fade_in_sound(s, 0.25f);
                //set_volume(s, 1.0f, second_change);
                //play_sound(game_audio, asset_connected_medium, playing_sound_flag_reverse, 1.0f);

                fade_in_sound(s, 1.0f);
            }
#endif
        }
#endif
#endif    
    }            
    
    //
    // NOTE: Game Mode change transition fade
    //
#if 1
    {
        f32 game_mode_transition_change = ((1.0f / game_mode_transition_time) * input->dt);

        // NOTE: When in game loading screen the transition will appear to do a pulse
        if(game_state->game_mode == game_mode_loading_assets)
        {
            // NOTE: We start at certain point so value will start at 1
            f32 neg1_to_pos1_t = sinf( (PI + (PI*0.5f)) + (realtime*5.0f) );
            game_state->game_loading_transition_colour_t = normalize_range_neg11(neg1_to_pos1_t);
        }
        else
        {
            game_state->game_loading_transition_colour_t -= input->dt*2.5f;
        }
        game_state->game_loading_transition_colour_t = clamp01(game_state->game_loading_transition_colour_t);
        
        game_state->game_mode_transition_t += game_mode_transition_change * game_state->game_mode_transition_direction;        
        
        if(game_state->game_mode_transition_direction == game_mode_transition_fade_out_direction)
        {
            if(game_state->game_mode_transition_t <= 0.0f)
            {
                change_game_mode(game_state, game_state->game_mode_transition_mode_result);
            }
        }

        game_state->game_mode_transition_t = clamp01(game_state->game_mode_transition_t);
        f32 fade = clamp01(1.0f - game_state->game_mode_transition_t);

        Vec2 points[] =
          {
              vec2(0.0f, 0.0f),
              vec2(1.0f, 0.0f),
              vec2(1.0f, 1.0f),
              vec2(0.0f, 1.0f)
          };

        Render_Transform tint_transform = draw_order_transform(entity_draw_order_transition_tint, blocky_blend_pieces_to_draw_transform((s32)round(fade*default_transform().blocky_blend_pieces_total_count)));

        f32 alpha = sinf((PI*0.5f)*fade);
        alpha = clamp01(alpha);

        Vec4 inner_colour = vec4(vec3(0.25f), alpha);
        Vec4 outer_colour = vec4(vec3(0.0f), alpha);

        // NOTE: Draw transition squares
        push_shape_blocky_blend(render_commands, vec2(0.0f), vec2(virtual_screen_width, virtual_screen_height), inner_colour, outer_colour, points, array_count(points), tint_transform);

        // NOTE: Draw loading screen, the blocky blend pieces move in and out as time goes on
        {
            tint_transform.draw_order = entity_draw_order_loading_screen;
            f32 colour_t = game_state->game_loading_transition_colour_t;
        
            tint_transform.blocky_blend_pieces_to_draw = (s32)round(colour_t*default_transform().blocky_blend_pieces_total_count);        
        
            push_shape_blocky_blend(render_commands, vec2(0.0f), vec2(virtual_screen_width, virtual_screen_height), outer_colour, inner_colour, points, array_count(points), tint_transform);
        }    
        
    }
#endif
    
    update_game_audio(game_audio, input->dt);

#if TWOSOME_INTERNAL
    DEBUG_overlay(memory, render_commands, ui_context);
#endif
    
    check_arena(&tran_state->tran_arena);

    if(restart_game)
    {
        // NOTE: Saving a non-filled save file to blank it out
        save_game(game_state, false);
        
        // NOTE: On next frame, game_state will initialize itself again
        zero_buffer(memory->permanent_storage, memory->permanent_storage_size);

        game_update_and_render_result.game_restarted = true;
    }
    
    return game_update_and_render_result;
}

extern "C" GAME_GET_SOUND_SAMPLES(game_get_sound_samples)
{
    Game_State *game_state = (Game_State *)memory->permanent_storage;
    Transient_State *tran_state = (Transient_State *)memory->transient_storage;
    Game_Audio *audio = &game_state->audio;

    output_playing_sounds(audio, sound_buffer, &tran_state->tran_arena);
}

#include "twosome_debug.cpp"
