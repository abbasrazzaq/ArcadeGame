#include "twosome_game_level.h"


internal Vec2 get_life_display_position(s32 life)
{
    f32 spacing = 0.0f;

    // NOTE: Put it so if rendering all lives, then it's centered
    f32 x = virtual_screen_center_x - ( ((player_level_start_lives * player_life_display_glow_size) + ( (player_level_start_lives - 1) * spacing ))  / 2.0f);

    f32 y = virtual_screen_height - (player_life_display_size + (player_life_display_glow_size - player_life_display_size)/2.0f) - ui_window_edge_offset;
            
    Vec2 result = vec2(x + (player_life_display_glow_size + spacing) * life, y);
    return result;
}

internal Vec3 get_connect_colour(int connect_colour)
{
    Vec3 result = {};
    if(connect_colour == connect_colour_black)
    {
        result = vec3(0.0f);
    }
    else if(connect_colour == connect_colour_white)
    {
        result = vec3(1.0f);
    }
    else
    {
        invalid_code_path;
    }

    return result;
}

internal void spawn_spikes_barrier_death(Level_Stage *stage, Spikes_Barrier *from_spikes_barrier, b32 killed_player, b32 killed_by_shield, f32 stage_time, Memory_Arena *perm_arena)
{
    if(!stage->first_free_spikes_barrier_death)
    {
        stage->first_free_spikes_barrier_death = push_struct(perm_arena, Spikes_Barrier_Death);
        stage->first_free_spikes_barrier_death->next = 0;
    }

    Spikes_Barrier_Death *barrier_death = stage->first_free_spikes_barrier_death;
    zero_object(Spikes_Barrier_Death, *barrier_death);
    stage->first_free_spikes_barrier_death = barrier_death->next;

    barrier_death->x = from_spikes_barrier->x;
    if(from_spikes_barrier->direction > 0.0f)
    {
        barrier_death->x -= spikes_barrier_safe_width;
    }
    barrier_death->bottom_parting_y = from_spikes_barrier->safe_spikes_y;
    barrier_death->top_parting_y = from_spikes_barrier->safe_spikes_y + spikes_barrier_safe_height;
    barrier_death->spawn_time = stage_time;
    barrier_death->safe_colour = get_connect_colour(from_spikes_barrier->safe_cursor_colour);
    barrier_death->killed_player = killed_player;
    barrier_death->killed_by_shield = killed_by_shield;
    
    barrier_death->next = stage->first_active_spikes_barrier_death;
    stage->first_active_spikes_barrier_death = barrier_death;
}

internal void spawn_spikes_barrier(Level_Stage *stage, int connect_colour, b32 spawn_from_both_sides, Memory_Arena *perm_arena, RNG *rng)
{
    if(!stage->first_free_spikes_barrier)
    {
        stage->first_free_spikes_barrier = push_struct(perm_arena, Spikes_Barrier);
        stage->first_free_spikes_barrier->next = 0;
    }

    Spikes_Barrier *spikes_barrier = stage->first_free_spikes_barrier;
    zero_object(Spikes_Barrier, *spikes_barrier);
    stage->first_free_spikes_barrier = spikes_barrier->next;

    spikes_barrier->safe_cursor_colour = connect_colour;
    spikes_barrier->safe_spikes_y = (float)(random(rng, 0, (virtual_screen_height - (int)spikes_barrier_safe_height)));
    spikes_barrier->spawn_time = (stage->time + spikes_barrier_spawn_time);
 
    Spikes_Barrier **first_active_spikes_ptr = &stage->first_active_spikes_barrier;
    if(spawn_from_both_sides && stage->last_spikes_barrier_spawned_direction > 0.0f)
    {
        spikes_barrier->direction = -1.0f;
        spikes_barrier->x = (virtual_screen_width + spikes_barrier_safe_width);
    }
    else
    {
        spikes_barrier->direction = 1.0f;
        spikes_barrier->x = (0 - spikes_barrier_safe_width);
    }    

    {
        Spikes_Barrier *spikes = spikes_barrier;
        s32 random_x_offset = (s32)spikes_barrier_safe_width;
        for(s32 point_index = 0; point_index < array_count(spikes->last_hint_point_offsets); ++point_index)
        {
            spikes->last_hint_point_offsets[point_index].x = (f32)random(rng, -random_x_offset, random_x_offset);
        }

        spikes->hint_point_offsets[0].y = 0.0f;
        spikes->hint_point_offsets[1].y = (virtual_screen_height*(1.0f/3.0f)) + (virtual_screen_height * ((random(rng, 0, 25)) / 100.0f));
        spikes->hint_point_offsets[2].y = virtual_screen_height*(2.0f/3.0f) + (virtual_screen_height * ((random(rng, 0, 25)) / 100.0f));                    
        spikes->hint_point_offsets[3].y = virtual_screen_height;

        spikes->last_points_change_time = 0.0f;
    }
    
    //NOTE: Check that we're not too close to last spikes barrier
    {
        Spikes_Barrier *last_barrier = *first_active_spikes_ptr;
        while(last_barrier)
        {
            if(last_barrier->direction == spikes_barrier->direction)
            {
                if((spikes_barrier->spawn_time - last_barrier->spawn_time) < spikes_barrier_spawn_time)
                {
                    spikes_barrier->spawn_time = (last_barrier->spawn_time + spikes_barrier_spawn_time);
                }
                break;
            }

            last_barrier = last_barrier->next;
        }
    }
    
    spikes_barrier->next = *first_active_spikes_ptr;
    *first_active_spikes_ptr = spikes_barrier;
    
    stage->last_spikes_barrier_spawned_direction = spikes_barrier->direction;
}

internal b32 spawn_spikes_barrier_if_timeline_active(Level_Stage *stage, int connect_colour, Stage_Metadata *meta, Memory_Arena *perm_arena, RNG *rng)
{
    b32 result = false;
    if(meta->spikes_barrier_left.activated || meta->spikes_barrier_right.activated)
    {
        b32 spawn_from_both_sides = (meta->spikes_barrier_left.activated && meta->spikes_barrier_right.activated);
        spawn_spikes_barrier(stage, connect_colour, spawn_from_both_sides, perm_arena, rng);
        result = true;
    }

    return result;
}

internal void spawn_exploding_particles(Game_Level *level, Vec2 position, u32 amount, Vec3 colour, f32 size, s32 speed_min, s32 speed_max)
{
    Level_Stage *stage = &level->stage;
    {
        for(u32 particle_index = 0; particle_index < amount; ++particle_index)
        {        
            if(!stage->first_free_particle)
            {
                stage->first_free_particle = push_struct(level->mode_arena, Particle);
                stage->first_free_particle->next = 0;
            }

            Particle *p = stage->first_free_particle;
            stage->first_free_particle = p->next;

            p->position = (position - size/2.0f);
            f32 angle = (f32)(random(level->rng, 0, (s32)(PI*2.0f*100.0f)) / 100.0f);
            f32 speed = (f32)random(level->rng, speed_min, speed_max);
            p->velocity = vec2(sinf(angle), cosf(angle)) * speed;
        
            p->life = (speed / (f32)speed_max);//*1.5f;
            p->start_life = p->life;
            p->colour = colour;
            p->size = size;
            p->corner_offset = random(level->rng, 5, 25) / 100.0f;

            p->next = stage->first_active_particle;
            stage->first_active_particle = p;

            ++level->stage.active_particles_count;
        }   
    }
}

internal void add_level_sound_echo(Game_Level *level, Play_Sound_Result sound_info, f32 volume, s32 echo_mode = level_sound_echo_mode_default, f32 speed = 0.5f)
{
    Level_Stage *stage = &level->stage;
    if(!stage->first_free_sound_echo)
    {
        stage->first_free_sound_echo = push_struct(level->mode_arena, Level_Sound_Echo);
        zero_object(Level_Sound_Echo, *stage->first_free_sound_echo);
    }

    Level_Sound_Echo *echo = stage->first_free_sound_echo;
    stage->first_free_sound_echo = echo->next;
            
    echo->next = stage->first_active_sound_echo;
    stage->first_active_sound_echo = echo;

    echo->sound_info = sound_info;
    echo->speed = speed;
    echo->volume = volume;
    echo->mode = echo_mode;

    // NOTE: Work out when we should start this sound
    f32 time_to_start = level->gametime + 1.5f + ((random(level->rng, 0, 20) / 100.0f));
    for(Level_Sound_Echo *existing_echo = stage->first_active_sound_echo; existing_echo; existing_echo = existing_echo->next)
    {
        if(existing_echo != echo)
        {
            if(sound_info.id == existing_echo->sound_info.id && (echo->sound_info.backwards == existing_echo->sound_info.backwards))
            {
                switch(existing_echo->mode)
                {                    
                    case level_sound_echo_mode_default:
                    {
                        time_to_start = existing_echo->time_to_start + 2.0f + (random(level->rng, 10, 20) / 100.0f);
                    } break;
                    
                    case level_sound_echo_mode_regular_beat:
                    {
                        time_to_start = (existing_echo->time_to_start + 1.5f);
                    } break;

                    case level_sound_echo_mode_irregular_beat:
                    {
                        f32 basic_time_offset = 2.0f + ((existing_echo->playing_beat / 5) * 1.1f);
                        if((existing_echo->playing_beat % 5) == 2)
                        {
                            time_to_start = (existing_echo->time_to_start + (basic_time_offset * 1.5f));
                        }
                        else
                        {
                            time_to_start = (existing_echo->time_to_start + basic_time_offset);
                        }
                        echo->playing_beat = (existing_echo->playing_beat + 1);
                    } break;

                    case level_sound_echo_mode_loop:
                    {
                        // NOTE: Don't really want to do any kind of staggering of this kind of sound?
                        //time_to_start = gametime + (f32)random(level->rng, 20, 30);
                    } break;
                    
                    invalid_default_case;
                };

                break;
            }
        }
    }
    
    echo->time_to_start = time_to_start;
}

internal void spawn_connection_change(Game_Level *level, Connector *connector, b32 lost_connection, u32 connection_count_level)
{
    Level_Stage *stage = &level->stage;
    
    if(!stage->first_free_connection_change)
    {
        stage->first_free_connection_change = push_struct(level->mode_arena, Connection_Change);
        stage->first_free_connection_change->next = 0;
    }
                    
    Connection_Change *change = stage->first_free_connection_change;
    stage->first_free_connection_change = stage->first_free_connection_change->next;
                    
    zero_object(Connection_Change, *change);
    change->position = connector->position;
    change->scale = connector->scale;
    change->ray_scale = vec2(connector->scale.x, connector->scale.x*0.25f);
    // NOTE: We do longer bullet for connections that were made whilst we had more connections
    switch(connection_count_level)
    {
        case connection_count_level_low:
        {            
        } break;

        case connection_count_level_medium:
        {
            change->ray_scale.x *= 2.5f;
        } break;

        case connection_count_level_high:
        {
            change->ray_scale.x *= 5.0f;
        } break;

        invalid_default_case;
    };
    
    change->time_spawned = stage->time;

    change->lost_connection = lost_connection;
    
    {
        s32 angle_buffer = 10;
        u32 angle = 0;
        for(u32 angle_index = 0; angle_index < 4; ++angle_index)
        {
            u32 angle_start_range = angle + angle_buffer;
            angle += 90;
            u32 angle_end_range = angle - angle_buffer;

            change->last_point_angles[angle_index] = degrees_to_radians((f32)random(level->rng, angle_start_range, angle_end_range));
        }
    }            
            
    change->next = stage->first_active_connection_change;
    stage->first_active_connection_change = change;
}

internal u32 get_connection_count_level(Level_Stage *stage)
{
    u32 result = connection_count_level_low;
    
    if(stage->num_connections >= medium_connectors_count)
    {
        result = connection_count_level_high;
    }
    else if(stage->num_connections > (medium_connectors_count / 2))
    {
        result = connection_count_level_medium;
    }

    return result;
}

internal void add_connector_connection(Game_Level *level, Connector *connector, f32 stage_time)
{
    if(!connector->connected)
    {
        u32 connected_sound_id = asset_none;
        u32 connection_count_level = get_connection_count_level(&level->stage);
        
        // NOTE: Play a slightly different version depending on how many connections we have
        {
            switch(connection_count_level)
            {
                case connection_count_level_low:
                {
                    connected_sound_id = asset_connected_small;
                } break;

                case connection_count_level_medium:
                {
                    connected_sound_id = asset_connected_medium;
                } break;

                case connection_count_level_high:
                {
                    connected_sound_id = asset_connected_high;
                } break;
            };
        }       

        assert(connected_sound_id != asset_none);
        
        Play_Sound_Result play_sound_result = play_sound(level->audio, connected_sound_id);
        connector->last_connection_sound_played = play_sound_result;
        connector->connection_count_level_at_last_connection = connection_count_level;
        
        add_level_sound_echo(level, play_sound_result, 0.075f, level_sound_echo_mode_irregular_beat);
        
        connector->last_score_increase_time = stage_time;

        connector->last_connection_change = connector->position;
        if(random_boolean(level->rng))
        {
            connector->last_connection_bkg_colour = vec3(1.0f, 0.0f, 0.0f);
        }
        else
        {
            connector->last_connection_bkg_colour = vec3(0.0f, 0.0f, 1.0f);            
        }
        
        spawn_connection_change(level, connector, false, connection_count_level);
        
    }
            
    connector->connected = true;
    connector->connected_time = 0.0f;
}

internal void spawn_life_loss(Game_Level *level, Vec2 position, Vec2 scale, Vec3 colour, Vec2 large_scale)
{
    Level_Stage *stage = &level->stage;
    if(!stage->first_free_life_loss)
    {
        stage->first_free_life_loss = push_struct(level->mode_arena, Life_Loss);
        stage->first_free_life_loss->next = 0;
    }

    Life_Loss *loss = stage->first_free_life_loss;
    stage->first_free_life_loss = stage->first_free_life_loss->next;

    zero_object(Life_Loss, *loss);
    loss->position = position;
    loss->scale = scale;
    loss->large_scale = large_scale;
    loss->colour = colour;
    loss->time_life_left = life_loss_lifespan_time;

    loss->next = stage->first_active_life_loss;
    stage->first_active_life_loss = loss;    
}

internal void remove_connector_connection(Game_Level *level, Connector *connector, b32 change_direction)
{
    if(connector->connected)
    {
        Level_Stage *stage = &level->stage;
        
        connector->connected = false;
        if(change_direction)
        {
            connector->velocity.x *= -1.0f;
            connector->velocity.y *= -1.0f;
        }

        spawn_spikes_barrier_if_timeline_active(stage, connector->connect_colour, &stage->meta, level->mode_arena, level->rng);

        add_level_sound_echo(level, play_sound(level->audio, connector->last_connection_sound_played.id, playing_sound_flag_reverse, 4.0f, connector->last_connection_sound_played.variation), 0.075f, level_sound_echo_mode_irregular_beat);
 
        spawn_connection_change(level, connector, true, connector->connection_count_level_at_last_connection);
    }
}

internal Connector *spawn_connector(Level_Stage *stage, Vec2 position, Vec2 velocity, int connect_colour, RNG *rng)
{
    stage->num_connectors += 1;
    assert(stage->num_connectors <= array_count(stage->connectors));
    Connector *new_connector = &stage->connectors[stage->num_connectors - 1];
    zero_object(Connector, *new_connector);
    
    new_connector->position = position;
    new_connector->velocity = normalize_or_zero(velocity);
    new_connector->connect_colour = connect_colour;
    new_connector->initial_speed = (connector_max_speed - connector_speed_variance) + ((random(rng, 1, 100) / 100.0f) * connector_speed_variance);
    
    new_connector->speed = new_connector->initial_speed;

    return new_connector;
}

internal void spawn_connector(Level_Stage *stage, int connect_colour, int spawn_side, Vec2 velocity, RNG *rng)
{
    Vec2 position = {};
    if(spawn_side == spawn_side_left)
    {
        position.x = -stage->connectors_scale;
        position.y = (r32)random(rng, 0, (virtual_screen_height - (int)stage->connectors_scale));        
    }
    else if(spawn_side == spawn_side_right)
    {
        position.x = virtual_screen_width;
        position.y = (r32)random(rng, 0, (virtual_screen_height - (int)stage->connectors_scale));
    }
    else if(spawn_side == spawn_side_bottom)
    {
        position.x = (r32)random(rng, 0, (virtual_screen_width - (int)stage->connectors_scale));
        position.y = -stage->connectors_scale;
    }
    else if(spawn_side == spawn_side_top)
    {
        position.x = (r32)random(rng, 0, (virtual_screen_width - (int)stage->connectors_scale));
        position.y = virtual_screen_height;
    }
    else
    {
        invalid_code_path;
    }

    spawn_connector(stage, position, velocity, connect_colour, rng);
}

internal void spawn_connector(Level_Stage *stage, int connect_colour, int spawn_side, RNG *rng)
{
    Vec2 velocity;
    velocity.x = random_boolean(rng) ? 1.0f : -1.0f;
    velocity.y = random_boolean(rng) ? 1.0f : -1.0f;

    spawn_connector(stage, connect_colour, spawn_side, velocity, rng);
}

internal void spawn_connector(Level_Stage *stage, RNG *rng)
{
    // NOTE: Spawns a connector from a random side, random connector colour, and random velocity    
    int connect_colour = stage->spawn_connector_colours[stage->num_connectors];
    int spawn_side = random_exclusive(rng, 0, spawn_side_count);

    spawn_connector(stage, connect_colour, spawn_side, rng);
}

internal void draw_spikes_barrier_spawn_hints(Game_Render_Commands *render_commands, Spikes_Barrier *first_active_spikes_barrier, Game_Level *level)
{
    r32 height = virtual_screen_height;
    // NOTE: This function assumes the barrier is vertical
    for(Spikes_Barrier *barrier = first_active_spikes_barrier; barrier; barrier = barrier->next)
    {
        if(barrier && !barrier->active)
        {            
            r32 start_x, end_x;
            if(barrier->direction > 0.0f)
            {
                start_x = 0.0f;
                end_x = start_x + spikes_barrier_haze_width;
            }
            else
            {
                start_x = virtual_screen_width;
                end_x = start_x - spikes_barrier_haze_width;
            }            
            
            f32 time_to_spawn = ((barrier->spawn_time - level->stage.time) / spikes_barrier_spawn_time);
            if(time_to_spawn <= 1.0f)
            {
                if(!barrier->hint_spawned)
                {
                    add_level_sound_echo(level, play_sound(level->audio, asset_spikes_barrier_spawn), 0.2f);
                    barrier->hint_spawned = true;
                }
                
                time_to_spawn = clamp01(time_to_spawn);

                r32 x = start_x + (end_x - start_x)*(1.0f - time_to_spawn);
                
                f32 alpha = time_to_spawn*0.75f;
                //Vec3 colour = interpolate(vec3(0.0f), get_connect_colour(barrier->safe_cursor_colour), alpha);
                Vec3 colour = get_connect_colour(barrier->safe_cursor_colour);
                
                f32 triangle_size = spikes_barrier_safe_width;
                f32 triangle_offset = spikes_barrier_triangle_spacing;

                u32 triangle_count = (u32)round_up(virtual_screen_height / (triangle_size + triangle_offset));
                f32 y = 0.0f;
                f32 rotation = -PI/2.0f;
                if(barrier->direction < 0.0f)
                {
                    rotation = PI/2.0f;
                }

                Render_Transform transform = rotation_transform(rotation, vec2(triangle_size/2.0f)/*, transform_flags(render_transform_flag_additive_blend)*/);

                Vec4 center_c = vec4(colour, 1.0f);
                Vec4 edge_c = vec4(0.0f, 0.0f, 0.0f, 0.0f);
                
                Vertex_XY_RGBA verts[] =
                {
                    { vec2(0.5f, 0.5f), center_c },
                    { vec2(0.0f, 0.0f), edge_c },
                    { vec2(0.5f, 1.0f), center_c },
                    
                    { vec2(0.5f, 0.5f), center_c },
                    { vec2(0.0f, 0.0f), edge_c },
                    { vec2(1.0f, 0.0f), edge_c },

                    { vec2(0.5f, 0.5f), center_c },
                    { vec2(1.0f, 0.0f), edge_c },
                    { vec2(0.5f, 1.0f), center_c },                      
                };
                
                for(u32 triangle_index = 0; triangle_index < triangle_count; ++triangle_index)
                {
                    push_triangle(render_commands, vec2(x, y), vec2(triangle_size), vec4(colour, alpha), transform);

                    //push_vertices(render_commands, (f32 *)verts, array_count(verts), vertex_flag_xy | vertex_flag_rgba, vec2(x, y), vec2(triangle_size), vec4(colour, alpha), transform);
                    
                    y += triangle_size + triangle_offset;
                }

            }
                        
        }
    }
}

internal void update_and_draw_mouse_control_hint(Game_Render_Commands *render_commands, r32 dt, s32 control_hint_type, Tutorial *tutorial)
{
    assert(control_hint_type != control_hint_type_none);
    
    b32 left_click = (control_hint_type == control_hint_type_colour_change);

    f32 line_thickness = 6.0f;
    Vec2 scale = vec2(75.0f, 100.0f);
    Vec2 position = vec2((virtual_screen_width/2.0f) - scale.x/2.0f, (virtual_screen_height/2.0f) + scale.y);

    Render_Transform transform = thickness_transform(line_thickness, transform_flags(render_transform_flag_half_line_corner_indent));
    
    push_rect_outline(render_commands, position, scale, vec4(safe_element_colour, 1.0f), transform);
    
    Vec2 button_scale = vec2(scale.x*0.5f, scale.y*0.45f);
    f32 button_y = position.y + (scale.y - button_scale.y);
        
    tutorial->control_hint_flash_time += dt;
    if(tutorial->control_hint_flash_time > 0.25f)
    {
        tutorial->control_hint_flash_time -= 0.25f;
        tutorial->control_hint_flash_on = !tutorial->control_hint_flash_on;
    }
    
    if(tutorial->control_hint_flash_on)
    {
        Vec2 flash_position = vec2(position.x, button_y);

        if(left_click)
        {
            flash_position = vec2(position.x, button_y);
        }
        else
        {
            flash_position = vec2(position.x + button_scale.x, button_y);
        }

        flash_position += line_thickness/2.0f;
        Vec2 flash_scale = button_scale - line_thickness;
        
        {
            Vec2 points[] =
              {
                  vec2(0.0f, 0.0f),
                  vec2(1.0f, 0.0f),
                  vec2(1.0f, 1.0f),
                  vec2(0.0f, 1.0f)
              };
            push_shape_blocky_blend(render_commands, flash_position, flash_scale, vec4(0.0f, 1.0f, 0.0f, 1.0f), vec4(0.0f, 1.0f, 0.0f, 0.75f), points, array_count(points), transform);
        }
    }
        
    push_rect_outline(render_commands, vec2(position.x, button_y), button_scale, vec4(safe_element_colour, 1.0f), transform);
    push_rect_outline(render_commands, vec2(position.x + button_scale.x, button_y), button_scale, vec4(safe_element_colour, 1.0f), transform);
    
}

internal Stage_Metadata *add_stage_metadata(Game_Level *level, s32 total_connectors, f32 stage_length_time)
{
    assert(level->stage_metadatas_count < array_count(level->stage_metadatas));
    assert(total_connectors <= array_count(level->stage.connectors));
    
    Stage_Metadata *meta = &level->stage_metadatas[level->stage_metadatas_count++];
    
    zero_object(Stage_Metadata, *meta);
    meta->stage_length_time = stage_length_time;
    meta->total_connectors = total_connectors;

    return meta;
    
}

internal void setup_timeline_event(Timeline_Event *event, r32 activate_time)
{
    event->enabled = true;
    event->activate_time = activate_time;
}

internal void setup_spinner_timeline_event(Timeline_Event *event, r32 active_time, float rotation_direction, s32 max_lasers = spinner_total_laser_count)
{
    assert(fabs(rotation_direction) <= 1.0f);
    setup_timeline_event(event, active_time);
    event->rotation_direction = rotation_direction;
    event->max_lasers = max_lasers;
}

internal void spawn_spinner(Level_Stage *stage, Timeline_Event *spinner_event, RNG *rng)
{
    Spinner *last_activated_spinner = 0;
    if(stage->num_spinners == 0)
    {
        play_sound(stage->spinner_laser_sound);
    }
    else
    {
        last_activated_spinner = &stage->spinners[stage->num_spinners - 1];
    }

    s32 spawn_left = (s32)spinner_size;
    s32 spawn_bottom = (s32)spinner_size;
    s32 spawn_right = virtual_screen_width - (s32)(spinner_size*2);
    s32 spawn_top = virtual_screen_height - (s32)(spinner_size*2);
    
    
    Spinner *spinner = &stage->spinners[stage->num_spinners++];
    spinner->position.x = (r32)random(rng, spawn_left, spawn_right);
    spinner->position.y = (r32)random(rng, spawn_bottom, spawn_top);
    
    spinner->velocity.x = random_boolean(rng) ? 1.0f : -1.0f;
    spinner->velocity.y = random_boolean(rng) ? 1.0f : -1.0f;
                
    spinner->active = true;    
    spinner->active_time = stage->time;
    spinner->rotation_direction = spinner_event->rotation_direction;
    spinner->max_lasers = spinner_event->max_lasers;

    for(int laser_index = 0; laser_index < array_count(spinner->lasers); ++laser_index)
    {
        spinner->lasers[laser_index].spawn_time = -1.0f;
    }
    
    if(last_activated_spinner)
    {
        // NOTE: We're kinda assuming that there's only one other spinner to worry about
        assert(max_spinner_count == 2);
        // NOTE: Make new spinner's rotation offset from last one by 90 degrees, and moving in opposite direction
        spinner->rotation = last_activated_spinner->rotation - (PI/2.0f);
        spinner->velocity = -last_activated_spinner->velocity;
        
        // NOTE: Work out which quadrant of the screen the last spinner is, and spawn in the furthest away one, this is in case the two spinners end up near each other and following the same trajectory
        Vec2 quadrant = vec2(virtual_screen_width / 2.0f, virtual_screen_height / 2.0f);

        // NOTE: If last spinner in left half, then go in right half, and vice versa
        if(last_activated_spinner->position.x <= quadrant.x)
        {
            spinner->position.x = (r32)random(rng, (s32)quadrant.x, spawn_right);
        }
        else
        {
            spinner->position.x = (r32)random(rng, spawn_left, (s32)quadrant.x);
        }
        
        // NOTE: If last spinner in bottom half, then go in top half, and vice versa
        if(last_activated_spinner->position.y <= quadrant.y)
        {
            spinner->position.y = (r32)random(rng, (s32)quadrant.y, spawn_top);
        }
        else
        {
            spinner->position.y = (r32)random(rng, spawn_bottom, (s32)quadrant.y);
        }       
        
    }
}

internal void init_level_stage(Game_Level *level)
{
    clear(level->mode_arena);
    
    //
    // NOTE: Stage
    //
    Level_Stage *stage = &level->stage;
    zero_buffer(stage, sizeof(Level_Stage));

    stage->mode = level_mode_playing;
    stage->life_lost_flash_start = -1.0f;
    // NOTE: Assume we're in transition at start of level stage
    stage->stages_in_transition = true;
    
    stage->connector_following_player_index = -1;
    stage->connectors_scale = connector_scale_max_size;
    stage->meta = level->stage_metadatas[level->current_stage];
    stage->meta.tutorial_stage_type = stage->meta.tutorial_type;
    
    initialize_captured_sounds_arena(&stage->captured_sounds, level->mode_arena, level->audio);
    
    stage->spinner_laser_sound = add_captured_sound(&stage->captured_sounds, load_sound(level->audio, asset_spinner_laser, playing_sound_flag_loop) );

    stage->expanding_shield_flash = make_flashing_value(1.0f, false, (flashing_value_flag_expanding | flashing_value_flag_constant_expand));

    //
    // NOTE: Player
    //
    Player *player = &stage->player;
    player->connect_colour = connect_colour_black;
    player->scale = vec2(player_size);
    player->position = vec2(level->cursor->x, virtual_screen_height);
    player->aabb = make_aabb(player->position, player->scale);
    
    //
    // NOTE: Precalculate the random connector colours for when their spawned
    //
    {
        // NOTE: Generate list of alternating connector colours
        int colour = connect_colour_black;
        for(int spawn_connector_colours_index = 0; spawn_connector_colours_index < stage->meta.total_connectors; ++spawn_connector_colours_index)
        {
            stage->spawn_connector_colours[spawn_connector_colours_index] = colour;
            colour = (colour == connect_colour_black ? connect_colour_white : connect_colour_black);
        }
        
        // NOTE: Jumble the connector colours up
        for(int spawn_connector_colours_index = 0; spawn_connector_colours_index < stage->meta.total_connectors; ++spawn_connector_colours_index)
        {
            int swap_index = random_exclusive(level->rng, 0, stage->meta.total_connectors);
            int temp = stage->spawn_connector_colours[swap_index];
            stage->spawn_connector_colours[swap_index] = stage->spawn_connector_colours[spawn_connector_colours_index];
            stage->spawn_connector_colours[spawn_connector_colours_index] = temp;
        }
    }

    //
    // NOTE: Activate a background blob centered over cursor
    //
    {
        assert(player_background_blob_index < array_count(stage->background_blobs));
        Level_Background_Blob *b = &stage->background_blobs[player_background_blob_index];
        b->life = 0.1f;
        b->life_velocity = 1.0f;
        b->initial_scale = connector_scale_max_size;
        b->position = (level->cursor->position) - b->initial_scale/2.0f - (background_blob_center_square_size/2.0f);
        b->colour = vec3(0.0f, 1.0f, 0.0f);
    }
}

internal void init_game_level(Game_Level *level, int32 level_type, Cursor *cursor, Game_Audio *audio, Memory_Arena *mode_arena, u32 score_to_get, u32 last_best_score, RNG *rng)
{
    zero_buffer(level, sizeof(Game_Level));
    
    level->type = level_type;
    level->lives = player_level_start_lives;

    level->cursor = cursor;
    level->audio = audio;
    level->mode_arena = mode_arena;
    level->rng = rng;

    level->score_to_get = score_to_get;
    level->last_best_score = last_best_score;

    level->game_over_goal_flash = make_flashing_value(0.75f, false);
    level->awaiting_resume_flash = make_flashing_value(0.75f, false);

    level->last_end_of_level_score_filling = 0.0f;
    // NOTE: We start off the last fill time such that a 1 point increase would
    // trigger a sound (guranteeing that we always hear a sound even from a 1 point increase)
    level->last_end_of_level_score_filling_sound_played_time = -end_of_level_score_fill_sound_time_interval*2;
    
    //
    // NOTE: Setup all stages for level
    //
    switch(level->type)
    {
        case game_level_type_connectors:
        {
#if 1
            {
                Stage_Metadata *meta = add_stage_metadata(level, low_connectors_count, 25.0f);
                meta->tutorial_type = tutorial_type_changing_colour;                
            }
            add_stage_metadata(level, medium_connectors_count, 30.0f);
            add_stage_metadata(level, high_connectors_count, 40.0f);
#else
            add_stage_metadata(level, medium_connectors_count, 500.0f);

#endif
        } break;

        case game_level_type_spikes_barrier:
        {
#if 1
            {
                Stage_Metadata *meta = add_stage_metadata(level, medium_connectors_count, 35.0f);
                setup_timeline_event(&meta->spikes_barrier_left, 10.0f);
            }
            {
                Stage_Metadata *meta = add_stage_metadata(level, medium_connectors_count, 40.0f);
                setup_timeline_event(&meta->spikes_barrier_left, 0.0f);
                setup_timeline_event(&meta->spikes_barrier_right, 5.0f);
            }
            {
                Stage_Metadata *meta = add_stage_metadata(level, high_connectors_count, 60.0f);
                setup_timeline_event(&meta->spikes_barrier_left, 0.0f);
                setup_timeline_event(&meta->spikes_barrier_right, 5.0f);
            }
#else
            {
                Stage_Metadata *meta = add_stage_metadata(level, high_connectors_count, 60.0f);
                setup_timeline_event(&meta->spikes_barrier_left, 0.0f);
                setup_timeline_event(&meta->spikes_barrier_right, 5.0f);
            }
#endif
        } break;

        case game_level_type_spinners:
        {
#if 1
            {
                Stage_Metadata *meta = add_stage_metadata(level, medium_connectors_count, 35.0f);
                setup_spinner_timeline_event(&meta->spinners[0], 10.0f, 1.0f, 2);
            }
            {
                Stage_Metadata *meta = add_stage_metadata(level, medium_connectors_count, 45.0f);
                setup_spinner_timeline_event(&meta->spinners[0], 5.0f, 1.0f);
            }
            {
                Stage_Metadata *meta = add_stage_metadata(level, medium_connectors_count, 70.0f);
                setup_spinner_timeline_event(&meta->spinners[0], 5.0f, 1.0f);
                setup_spinner_timeline_event(&meta->spinners[1], 25.0f, -1.0f);
            }
#else
            {
                Stage_Metadata *meta = add_stage_metadata(level, medium_connectors_count, 15.0f);
                setup_spinner_timeline_event(&meta->spinners[0], 0.0f, 1.0f);
                setup_spinner_timeline_event(&meta->spinners[1], 0.0f, -1.0f);
            }
#endif
        } break;

        case game_level_type_everything:
        {
            {
                Stage_Metadata *meta = add_stage_metadata(level, medium_connectors_count, 45.0f);
                setup_timeline_event(&meta->spikes_barrier_left, 0.0f);
                setup_spinner_timeline_event(&meta->spinners[0], 10.0f, 1.0f, 2);
            }
            {
                Stage_Metadata *meta = add_stage_metadata(level, medium_connectors_count, 50.0f);
                setup_timeline_event(&meta->spikes_barrier_left, 0.0f);
                setup_spinner_timeline_event(&meta->spinners[0], 5.0f, 1.0f);
            }
            {
                Stage_Metadata *meta = add_stage_metadata(level, medium_connectors_count, 70.0f);
                setup_timeline_event(&meta->spikes_barrier_left, 0.0f);
                setup_spinner_timeline_event(&meta->spinners[0], 5.0f, 1.0f);
                setup_spinner_timeline_event(&meta->spinners[1], 25.0f, -1.0f);
            }
        } break;

        case game_level_type_shield_intro:
        {
            {
                Stage_Metadata *meta = add_stage_metadata(level, low_connectors_count, (shield_active_time + 2.0f));
                meta->tutorial_type = tutorial_type_using_shield;
            }
            add_stage_metadata(level, medium_connectors_count, 35.0f);
            add_stage_metadata(level, high_connectors_count, 40.0f);
        } break;

        case game_level_type_shield_everything:
        {
#if 1
            {
                Stage_Metadata *meta = add_stage_metadata(level, medium_connectors_count, 60.0f);
                setup_timeline_event(&meta->spikes_barrier_left, 0.0f);
                setup_spinner_timeline_event(&meta->spinners[0], 20.0f, 1.0f);
            }
            {
                Stage_Metadata *meta = add_stage_metadata(level, medium_connectors_count, 60.0f);
                setup_timeline_event(&meta->spikes_barrier_left, 0.0f);
                setup_spinner_timeline_event(&meta->spinners[0], 15.0f, 1.0f);
                f32 last_spinner_play_time = 10.0f;
                setup_spinner_timeline_event(&meta->spinners[1], (meta->spinners[0].activate_time + spinner_fully_active_time + last_spinner_play_time), -1.0f);
                
            }
            {
                Stage_Metadata *meta = add_stage_metadata(level, medium_connectors_count, 90.0f);
                setup_timeline_event(&meta->spikes_barrier_left, 0.0f);
                setup_timeline_event(&meta->spikes_barrier_right, 5.0f);
                setup_spinner_timeline_event(&meta->spinners[0], 15.0f, 1.0f);
                f32 last_spinner_play_time = 10.0f;
                setup_spinner_timeline_event(&meta->spinners[1], (meta->spinners[0].activate_time + spinner_fully_active_time + last_spinner_play_time), -1.0f);
            }
#else
            {
                Stage_Metadata *meta = add_stage_metadata(level, medium_connectors_count, 25.0f);
                setup_timeline_event(&meta->spikes_barrier_left, 0.0f);
                setup_timeline_event(&meta->spikes_barrier_right, 5.0f);
                setup_spinner_timeline_event(&meta->spinners[0], 15.0f, 1.0f);
                f32 last_spinner_play_time = 10.0f;
                setup_spinner_timeline_event(&meta->spinners[1], (meta->spinners[0].activate_time + spinner_fully_active_time + last_spinner_play_time), -1.0f);
            }
#endif
        } break;

        invalid_default_case;
    };    
    
    init_level_stage(level);
}

internal void update_wandering_velocity(Vec2 position, float scale, Vec2 *velocity)
{
    if(position.x <= 0.0f)
    {
        velocity->x = 1.0f;
    }
    else if((position.x + scale) >= virtual_screen_width)
    {
        velocity->x = -1.0f;
    }

    if(position.y <= 0.0f)
    {
        velocity->y = 1.0f;
    }
    else if((position.y + scale) >= virtual_screen_height)
    {
        velocity->y = -1.0f;
    }

    *velocity = normalize_or_zero(*velocity);
}

internal void change_game_score(Game_Level *level, Game_Render_Commands *render_commands, f32 gametime, Vec2 position, int amount)
{
    level->score += amount;

    Level_Stage *stage = &level->stage;
    
    if(!stage->first_free_score_notif)
    {
        stage->first_free_score_notif = push_struct(level->mode_arena, Score_Notification);
        stage->first_free_score_notif->next = 0;
    }
    
    Score_Notification *notif = stage->first_free_score_notif;
    stage->first_free_score_notif = notif->next;
    // NOTE: Make sure it spawns within confines of screen
    {
        Blocky_Number_Info blocky_number_info = get_blocky_number_info(render_commands->view, amount, max_score_notification_number_size);

        notif->position = clamp_v2(position, vec2(0.0f), (vec2(virtual_screen_width, virtual_screen_height) - vec2(blocky_number_info.total_size.x, blocky_number_info.total_size.y)));
    }
    
    notif->start_time = gametime;
    notif->amount = amount;

    notif->next = stage->first_visible_score_notif;    
    stage->first_visible_score_notif = notif;
}

internal b32 update_timeline_event_activeness(Timeline_Event *event, float gametime, b32 only_return_true_if_just_activated = false)
{
    b32 result = false;
    if(event->enabled && !event->activated && gametime >= event->activate_time)
    {
        result = true;
        if(only_return_true_if_just_activated && event->activated)
        {
            result = false;
        }

        event->activated = true;
    }

    return result;
}

internal void shutdown_level_stage(Game_Level *level)
{
    relinquish_sounds_to_audio_system(&level->stage.captured_sounds);
    fade_out_all_sounds_owned_by_audio_system(level->audio, stage_transition_sound_fade_duration, playing_sound_flag_game_level_stage_no_fade);
}

internal b32 in_first_level_stage(Game_Level *level)
{
    b32 result = (level->current_stage == 0);
    return result;
}

internal b32 in_last_level_stage(Game_Level *level)
{
    b32 result = (level->current_stage == (level->stage_metadatas_count - 1));
    return result;
}

internal b32 change_level_stage(Game_Level *level, Level_Stage *stage, s32 increment = 1)
{
    b32 result = false;
    
    if( (increment > 0 && !in_last_level_stage(level))
        || (increment < 0 && !in_first_level_stage(level)) )
    {
        shutdown_level_stage(level);
        
        level->current_stage += increment;
        
        init_level_stage(level);
#if !TWOSOME_INTERNAL
        // NOTE: This can be non-existant when in dev mode, because of how we can jump around levels
        assert(level->last_level_stage_transition_play_sound_result.id);
#endif
        if(level->last_level_stage_transition_play_sound_result.id)
        {
            add_level_sound_echo(level, level->last_level_stage_transition_play_sound_result, 0.75f);   
        }        
        
        result = true;
    }
    
    return result;
}

internal b32 level_stage_frozen(Level_Stage *stage)
{
    b32 result = false;

    if(stage->mode == level_mode_game_over)
    {
        result = true;
    }
    
    return result;
}

struct Shield_Collider
{
    b32 circle_collider;

    union
    {
        AABB aabb;
        
        // NOTE: Circle
        struct
        {        
            Vec2 center;
            f32 radius;
        };
    };    
};

internal b32 shields_entity_overlap(Shield_Collider collider, Level_Stage *stage)
{
    b32 result = false;

    Shield *first_active_shield = stage->first_active_shield;
    
    for(Shield *shield = first_active_shield; shield; shield = shield->next)
    {
        if(shield->active)
        {
            if((collider.circle_collider && circle_vs_circle(collider.center, collider.radius, shield->center, shield->radius))
               || (!collider.circle_collider && aabb_circle_overlap(collider.aabb, shield->radius, shield->center)))
            {
                result = true;
                break;
            } 
        }
    }

    return result;
}

internal b32 shields_entity_overlap(AABB aabb, Level_Stage *stage)
{
    Shield_Collider collider;
    collider.circle_collider = false;
    collider.aabb = aabb;

    b32 result = shields_entity_overlap(collider, stage);
    return result;
}

internal b32 shields_entity_overlap(Vec2 center, f32 radius, Level_Stage *stage)
{
    Shield_Collider collider;
    collider.circle_collider = true;
    collider.center = center;
    collider.radius = radius;

    b32 result = shields_entity_overlap(collider, stage);
    return result;
}

internal void draw_score_meter_corner(Game_Render_Commands *render_commands, u32 best_score, u32 level_score_to_get, u32 last_best_score, f32 time)
{
    // NOTE: If we've already beaten the score for the level, then the score to get is out last best score
    u32 score_to_get = level_score_to_get > last_best_score ? level_score_to_get : last_best_score;
    f32 normalized_completion = clamp01(((r32)best_score / (r32)score_to_get));
    
    Vec2 meter_scale = vec2(25.0f, 50.0f);
    
    f32 meter_position_top_y = virtual_screen_height - ui_window_edge_offset;
    r32 meter_position_y = meter_position_top_y - meter_scale.y;

    f32 meter_text_offset = 6.0f;
    f32 ui_x = ui_window_edge_offset;

    Render_Transform transform = draw_order_transform(entity_draw_order_corner_score_meter);
    
    // NOTE: Score To Get
    {
        f32 normalized_score_beaten = 0.0f;

        // NOTE: Want to push the score to get down after it's been beaten
        if(best_score > score_to_get)
        {
            u32 score_diff = best_score - score_to_get;
            normalized_score_beaten = ((f32)score_diff / (f32)score_to_get);   
        }
        
        normalized_score_beaten = clamp01(normalized_score_beaten);

        f32 text_size = 3;
        Blocky_Number_Info block_num_info = get_blocky_number_info(render_commands->view, score_to_get, text_size);
        f32 y = (((meter_position_top_y - block_num_info.total_size.y) - (meter_scale.y - block_num_info.total_size.y)*normalized_score_beaten));
        
        f32 text_right_edge_x = push_blocky_number(render_commands, score_to_get, text_size, ui_x, y, vec4(vec3(0.25f), 1.0f), transform); 

        ui_x = text_right_edge_x;
    }

    ui_x += meter_text_offset;
    f32 meter_border_thickness = 3.0f;
    Vec2 inner_meter_scale = meter_scale - vec2(meter_border_thickness*2.0f);    
    
    // NOTE: Meter
    {
        Vec2 meter_position = vec2(ui_x, meter_position_y);
        Vec2 inner_meter_position = vec2(meter_position.x + meter_border_thickness, meter_position.y + meter_border_thickness);

        f32 last_best_score_completion = clamp01((f32)last_best_score / (f32)score_to_get);
        
        // NOTE: Last Best Score Section
        push_rect(render_commands, inner_meter_position, vec2(inner_meter_scale.x, inner_meter_scale.y*last_best_score_completion), vec4((safe_element_colour*0.25f), 0.75f), transform);

        // NOTE: Best Score Section
        push_rect(render_commands, inner_meter_position, vec2(inner_meter_scale.x, inner_meter_scale.y*normalized_completion), vec4(safe_element_colour*0.75f, 1.0f), transform);
        
        push_rect_outline(render_commands, meter_position, meter_scale, vec4(vec3(0.5f), 0.75f), thickness_transform(meter_border_thickness, corner_indent_transform(meter_border_thickness, transform)));
        
        ui_x = meter_position.x + meter_scale.x;
    }

    // NOTE: Best Score
    ui_x += meter_text_offset;
    {
        f32 text_size = 4;
        if(best_score >= score_to_get)
        {
            // HACK: Because of way text scaling against resolutions works we don't know that adding
            // just 1 will actually increase the font size by 1 :(, but adding 1.5 will definately do it
            text_size += normalize_range_neg11(sinf(time*8.0f)) > 0.5f ? 1.5f : 0.0f;
        }
        
        Blocky_Number_Info block_num_info = get_blocky_number_info(render_commands->view, best_score, text_size);
        f32 y = (meter_position_y + ((meter_scale.y - block_num_info.total_size.y) * normalized_completion));
        push_blocky_number(render_commands, best_score, text_size, ui_x, y, vec4(safe_element_colour, 1.0f), transform);
    }    
}

internal void draw_score_meter_floating(Game_Render_Commands *render_commands, Vec2 position, f32 size, u32 best_score, u32 level_score_to_get, u32 last_best_score, Flashing_Value *goal_flash, f32 glow_scaling = 0.0f, f32 alpha = 1.0f, f32 time = 0.0f)
{
    u32 score_to_get = level_score_to_get > last_best_score ? level_score_to_get : last_best_score;
    f32 normalized_completion = clamp01(((f32)best_score / (f32)score_to_get));

    Vec2 scale = vec2(size, size*2.0f);
    // NOTE: We scale certain things as score meter goes bigger (border, text)
    f32 meter_scaling = size / 25.0f;
    f32 border_thickness = 3.0f * meter_scaling;

    // NOTE: Draw score meter "fill"
    {
        Vec2 fill_total_scale = vec2((scale.x - border_thickness*2.0f), (scale.y - (border_thickness*2.0f)));
        Vec2 bkg_scale = vec2(fill_total_scale.x, fill_total_scale.y*normalized_completion);
        Vec2 bkg_pos = position + border_thickness;
        push_rect(render_commands, bkg_pos, bkg_scale, vec4(safe_element_colour, alpha*0.75f), draw_order_transform(entity_draw_order_floating_score_meter));
    }
    
    push_rect_outline(render_commands, position, scale, vec4(vec3(0.5f), alpha*1.0f), thickness_transform(border_thickness, draw_order_transform(entity_draw_order_floating_score_meter, corner_indent_transform(border_thickness))));

    {
        f32 goal_icon_offset_y = 0.1f * scale.y;
        Vec2 goal_position = vec2(position.x, position.y + scale.y + goal_icon_offset_y);
        Vec2 goal_scale = vec2(scale.x);

        Vec4 fill_colour = vec4(interpolate(vec3(0.5f), safe_element_colour, goal_flash->t), alpha*1.0f);

        // NOTE: We draw glow if we achieved score, or previously have done
        b32 draw_glow = (best_score >= level_score_to_get || last_best_score >= level_score_to_get);
        
        if(draw_glow)
        {
            push_triangle(render_commands, goal_position, goal_scale, fill_colour, draw_order_transform(entity_draw_order_floating_score_meter));
        }

        push_triangle_outline(render_commands, goal_position, goal_scale, vec4(vec3(0.5f), alpha*1.0f), thickness_transform(border_thickness, draw_order_transform(entity_draw_order_floating_score_meter)));

        if(draw_glow)
        {
            Vec4 center_colour = vec4(fill_colour.rgb*fill_colour.a, alpha*1.0f);
            Vec4 edge_colour = vec4(0.1f*alpha);

            Vec2 glow_scale = goal_scale*1.5f + glow_scaling*goal_scale;
            Vec2 glow_position = goal_position - (glow_scale - goal_scale)/2.0f;

            push_triangle_blocky_blend(render_commands, glow_position, glow_scale, center_colour, edge_colour, draw_order_transform(entity_draw_order_floating_score_meter, transform_flags(render_transform_flag_additive_blend)));
        }

    }
    
    f32 meter_text_offset = 6.0f * meter_scaling;
    // NOTE: Score to Get Text
    {
        f32 score_to_get_text_y = (position.y + scale.y);

        // NOTE: We push down the score to beat the more we beat it (will hit the bottom when doubled the score)
        r32 normalized_score_beaten = 0.0f;
        if(best_score > score_to_get)
        {
            u32 score_diff = best_score - score_to_get;
            normalized_score_beaten = ((f32)score_diff / (f32)score_to_get);
        }
        normalized_score_beaten = clamp01(normalized_score_beaten);

        f32 text_size = (3.5f * meter_scaling);
        Blocky_Number_Info block_num_info = get_blocky_number_info(render_commands->view, score_to_get, text_size);

        score_to_get_text_y = (position.y + scale.y - block_num_info.total_size.y) - ((scale.y - block_num_info.total_size.y)*normalized_score_beaten);
                
        Vec2 score_to_get_text_pos = vec2((position.x - meter_text_offset), score_to_get_text_y);
        
        push_blocky_number(render_commands, score_to_get, text_size, score_to_get_text_pos.x, (score_to_get_text_pos.y), vec4(vec3(0.25f), alpha*1.0f), transform_flags(render_transform_flag_right_anchor_number, draw_order_transform(entity_draw_order_floating_score_meter)));

    }

    // NOTE: Best Score
    {
        f32 text_size = (4.5f * meter_scaling);

        if(best_score >= score_to_get)
        {
            // HACK: Because of way text scaling against resolutions works we don't know that adding
            // just 1 will actually increase the font size by 1 :(, but adding 1.5 will definately do it
            text_size += sinf(time*8.0f) > 0.0f ? 1.5f : 0.0f;
        }
        
        Blocky_Number_Info block_num_info = get_blocky_number_info(render_commands->view, score_to_get, text_size);

        f32 x = (position.x + scale.x + meter_text_offset);
        f32 y = (position.y + ((scale.y - block_num_info.total_size.y) * normalized_completion));
        
        push_blocky_number(render_commands, best_score, text_size, x, y, vec4(safe_element_colour*0.75f, alpha*1.0f), draw_order_transform(entity_draw_order_floating_score_meter));

    }
}

internal b32 is_player_protected_from_spinner_lasers(Level_Stage *stage)
{
    // NOTE: Even when player is no longer touching the connector, we want to give them a little slack
    //f32 player_touching_connector_slack = 0.125f;
    //b32 result = (stage->time - stage->player.last_time_touched_connector) <= player_touching_connector_slack;
    b32 result = stage->player.touching_connector; 
    return result;
}

internal b32 emphasize_player_colour_change(Level_Stage *stage)
{
    b32 result = (stage->meta.tutorial_stage_type == tutorial_type_changing_colour);
    return result;      
}

internal void update_player(Player *player, Game_Level *level, b32 change_colour, r32 dt)
{
    if(change_colour)
    {
        if(player->connect_colour == connect_colour_black)
        {
            player->connect_colour = connect_colour_white;
        }
        else if(player->connect_colour == connect_colour_white)
        {
            player->connect_colour = connect_colour_black;
        }
        else
        {
            invalid_code_path;
        }

        int sound_variation = player->connect_colour;
        // NOTE: Sound is louder when we're in tutorial
        f32 volume = emphasize_player_colour_change(&level->stage) ? 1.0f : 0.35f;
        
        add_level_sound_echo(level, play_sound(level->audio, asset_player_colour_change, 0, 1, sound_variation, volume), 0.75f, level_sound_echo_mode_regular_beat);

        player->last_time_changed_colour = level->stage.time;
    }
    
    Vec2 move = {};
    Cursor *cursor = level->cursor;
    Vec2 cursor_center = cursor->position - player->scale/2.0f;
    Vec2 player_to_cursor = cursor_center - player->position;
    Vec2 velocity = normalize_or_zero(player_to_cursor);
    move = velocity * dt * 1000.0f;
    float move_dist = magnitude(move);
    float dist = magnitude(player_to_cursor);
    if(move_dist > dist)
    {
        move = velocity * dist;
    }

    Vec2 last_player_pos = player->position;
    player->position += move;
    player->aabb = make_aabb(last_player_pos, player->position, player->scale);
    player->obb = make_obb(last_player_pos, player->position, 0, 0, vec2(0), player->scale);

    player->touching_spinner_laser = false;
    player->touching_connector = false;
}

internal void draw_player_shape(Game_Render_Commands *render_commands, Vec2 position, Vec2 scale, Vec2 md, Vec4 edge_c, Vec4 center_c, Level_Stage *stage, Render_Transform transform = default_transform())
{
    Vec2 bl = position;
    Vec2 tr = position + scale;

    f32 offset = magnitude(scale)*0.075f;

    Vec2 tl = vec2(bl.x, tr.y);
    Vec2 br = vec2(tr.x, bl.y);
    Vec2 center = bl + scale/2.0f;
        
    f32 left = bl.x;
    f32 right = tr.x;
    f32 top = tr.y;
    f32 bottom = bl.y;       

    bl += normalize(center - bl) * offset;
    br += normalize(center - br) * offset;
    tr += normalize(center - tr) * offset;
    tl += normalize(center - tl) * offset;
        
    Vertex_XY_RGBA verts[] =
      {
          // Bottom Triangles
          { bl, edge_c },
          { md.x, bottom, edge_c },
          { md, center_c },
              
          { md.x, bottom, edge_c },
          { br, edge_c },
          { md, center_c },
              
          // Top
          { tr, edge_c },
          { md.x, top, edge_c },
          { md, center_c },

          { md.x, top, edge_c },
          { md, center_c },
          { tl, edge_c },
              
          // Left
          { tl, edge_c },
          { left, md.y, edge_c },
          { md, center_c },
              
          { left, md.y, edge_c },
          { md, center_c },
          { bl, edge_c },
              
          // Right
          { br, edge_c },
          { right, md.y, edge_c },
          { md, center_c },

          { right, md.y, edge_c },
          { md, center_c },
          { tr, edge_c }
              
      };        
    push_vertices(render_commands, (f32 *)verts, array_count(verts), vertex_flag_xy | vertex_flag_rgba, vec4(1.0f), transform);
}

internal void draw_player(Game_Render_Commands *render_commands, Vec3 colour, Vec2 position, Vec2 scale, Level_Stage *stage, f32 last_colour_change_time, Render_Transform transform = default_transform())
{    
    Vec3 player_colour = colour;
        
    Vec4 edge_c = vec4(player_colour, 1.0f);
    f32 center_alpha = ((sinf(stage->time*3.0f) + 1.0f) / 2.0f) * 0.25f;
        
    if(is_player_protected_from_spinner_lasers(stage))
    {
        center_alpha = 0.0f;
    }
    Vec4 center_c = vec4(vec3(1.0f, 0.0f, 1.0f)*0.75f, center_alpha);

    Vec2 md = (position + scale/2.0f);

    f32 shake = 2.0f;//player->touching_connector ? 8.0f : 2.0f;
    md.x += sinf(stage->time*shake)*scale.x*0.2f;
    md.y += cosf(stage->time*shake)*scale.y*0.2f;
    
#if 1
    draw_player_shape(render_commands, position, scale, md, edge_c, center_c, stage, transform);
#else
    
    {
        Vec2 bl = position;
        Vec2 tr = position + scale;
        Vec2 md = (position + scale/2.0f);

        f32 shake = 2.0f;//player->touching_connector ? 8.0f : 2.0f;
        md.x += sinf(stage->time*shake)*scale.x*0.2f;
        md.y += cosf(stage->time*shake)*scale.y*0.2f;
        
        f32 offset = magnitude(scale)*0.075f;

        Vec2 tl = vec2(bl.x, tr.y);
        Vec2 br = vec2(tr.x, bl.y);
        Vec2 center = bl + scale/2.0f;
        
        f32 left = bl.x;
        f32 right = tr.x;
        f32 top = tr.y;
        f32 bottom = bl.y;       

        bl += normalize(center - bl) * offset;
        br += normalize(center - br) * offset;
        tr += normalize(center - tr) * offset;
        tl += normalize(center - tl) * offset;
        
        Vertex_XY_RGBA verts[] =
          {
              // Bottom Triangles
              { bl, edge_c },
              { md.x, bottom, edge_c },
              { md, center_c },
              
              { md.x, bottom, edge_c },
              { br, edge_c },
              { md, center_c },
              
              // Top
              { tr, edge_c },
              { md.x, top, edge_c },
              { md, center_c },

              { md.x, top, edge_c },
              { md, center_c },
              { tl, edge_c },
              
              // Left
              { tl, edge_c },
              { left, md.y, edge_c },
              { md, center_c },
              
              { left, md.y, edge_c },
              { md, center_c },
              { bl, edge_c },
              
              // Right
              { br, edge_c },
              { right, md.y, edge_c },
              { md, center_c },

              { right, md.y, edge_c },
              { md, center_c },
              { tr, edge_c }
              
          };        
        push_vertices(render_commands, (f32 *)verts, array_count(verts), vertex_flag_xy | vertex_flag_rgba, vec4(1.0f));
    }
#endif
    
    {
        Vec2 scale_offset = {};

        f32 expand_t = clamp01((stage->time - last_colour_change_time) / 0.5f);
        expand_t = sinf(expand_t * PI);
        
        if(emphasize_player_colour_change(stage))
        {            
            scale_offset = (scale*expand_t*6.0f);
        }
        else
        {
            scale_offset = (scale*expand_t*1.5f);    
        }
        
        if(stage->player.shake_until_time >= stage->time)
        {
            //scale_offset += (vec2(normalize_range_neg11(cosf(stage->time*13.0f)), normalize_range_neg11(sinf(stage->time*13.0f))) * 220.0f);

            //colour = vec3(0.0f, 1.0f, 0.0f);
        }
        
        Vec2 glow_scale = scale*2.0f + scale_offset;
        Vec2 glow_position = position - ((glow_scale - scale) / 2.0f);

        Vec4 corner_colour = vec4(colour, /*0.0f*/0.2f);
        Vec4 center_colour = vec4(colour, 0.75f);

        f32 corner_offset = 0.2f + (0.1f * normalize_range_neg11(sinf(stage->time)));        
        
        Bendy_Quad_Points bendy_quad = get_bendy_quad_points(corner_offset);
        push_shape_blocky_blend(render_commands, glow_position, glow_scale, center_colour, corner_colour, bendy_quad.points, array_count(bendy_quad.points));
        
    }
}

#if TWOSOME_INTERNAL
internal void DEBUG_draw_aabb(Game_Render_Commands *render_commands, AABB *a)
{
    push_rect(render_commands, a->last_center - a->extents, a->extents*2.0f, vec4(1.0f, 0.0f, 1.0f, 0.25f));
    push_rect(render_commands, a->center - a->extents, a->extents*2.0f, vec4(1.0f, 0.0f, 1.0f, 0.5f));
}
#endif

internal void draw_laser_shield(Game_Render_Commands *render_commands, Vec2 main_position, Vec2 main_scale, Vec4 colour, f32 spacing = 1.0f, b32 outer_faded = false, b32 draw_outline = false)
{
    Render_Transform transform = default_transform();
    if(draw_outline)
    {
        transform = hexagon_lattice_outline_transform(safe_element_colour, transform_flags(render_transform_flag_joined_outline, thickness_transform(2.0f)));
                                                      
                                                      //transform = transform_flags(render_transform_flag_hexagon_outline | render_transform_flag_joined_outline, thickness_transform(2.0f));
    }
    push_hexagon_lattice(render_commands, main_position - main_scale/2.0f, main_scale, colour, spacing, transform);
}

internal void draw_spikes_barrier_haze(Game_Render_Commands *render_commands, Level_Stage *stage, Timeline_Event *barrier_event, b32 right_side)
{
    // NOTE: We fade in and "flash" the haze when it starts
    f32 spawn_t = clamp01((stage->time - barrier_event->activate_time) / 1.0f);
    //spawn_t += sinf(stage->time) * 0.1f;
    
    Vec2 scale = vec2(spikes_barrier_haze_width, virtual_screen_height);
    //scale.x = spikes_barrier_haze_width + ((spikes_barrier_haze_width*2.0f) * sinf(spawn_t*PI));
    scale.x = spikes_barrier_haze_width*0.5f + ((spikes_barrier_haze_width) * sinf(spawn_t*PI));
    
    Vec4 colour = vec4(interpolate(vec3(1.0f), danger_element_colour, spawn_t), 0.75f);

    Render_Transform transform = draw_order_transform(entity_draw_order_spikes_barrier_haze,
                                                      transform_flags(render_transform_flag_additive_blend | render_transform_flags_blend_exclude_colours));
    
    if(right_side)
    {
        f32 x = (virtual_screen_width - scale.x);
        push_blocky_blend_rect(render_commands, vec2(x, 0.0f), scale, vec4(vec3(0.0f), 0.0f), colour, transform);
    }
    else
    {
        push_blocky_blend_rect(render_commands, vec2(0.0f), scale, colour, vec4(vec3(0.0f), 0.0f), transform);
    }
}

internal void set_level_game_over(Game_Level *level, Game_Progress *progress, b32 died, Game_Mode_Result *game_mode_result)
{
    // NOTE: Got to end of last stage so can record progress
    assert(level->type < number_of_levels);
    s32 level_index = level->type;
    if(level->score > progress->best_level_scores[level_index])
    {
        progress->best_level_scores[level_index] = level->score;
    }

    if((level_index + 1) == progress->levels_unlocked)
    {
        ++progress->levels_unlocked;
        progress->levels_unlocked = min(progress->levels_unlocked, number_of_levels);
        ++progress->levels_completed;
    }
    game_mode_result->action = game_mode_result_action_save;

    
    if(level->stage.mode != level_mode_game_over)
    {
        shutdown_level_stage(level);

        //f32 speed = died ? 0.5f : 2.0f;
        //play_sound(level->audio, asset_level_stage_transition, 0, speed);
    }

    level->stage.mode = level_mode_game_over;
}

internal b32 update_bpm(Level_Stage *stage, f32 t, f32 bpm, f32 *last_beat_time, f32 *bpm_picked = 0)
{
    b32 hit_beat = false;

    bpm += (t * 150.0f);

    f32 beat_interval = 60.0f / bpm;
    if((stage->time - *last_beat_time) >= beat_interval)
    {
        hit_beat = true;
        *last_beat_time = stage->time;
    }

    if(bpm_picked)
    {
        *bpm_picked = bpm;
    }
    
    return hit_beat;
}

internal void draw_shield(Game_Render_Commands *render_commands, Vec2 center, f32 radius, f32 rotation, Memory_Arena *temp_arena, Vec4 center_colour, Vec4 edge_colour, b32 alternate_colour, Render_Transform transform)
{
    Temporary_Memory temp_mem = begin_temporary_memory(temp_arena);

    Vec2 position = center - radius;
    Vec2 scale = vec2(radius*2.0f);
    
    Dodecagon_Points dodecagon_points = get_dodecagon_points();
    transform_vertices(dodecagon_points.points, dodecagon_points.points, array_count(dodecagon_points.points), position, scale, rotation, scale/2.0f);
    
    for(u32 edge_point_index = 0; edge_point_index < array_count(dodecagon_points.points); ++edge_point_index)
    {
        Vec3 center_colour_rgb = center_colour.rgb;
        Vec3 edge_colour_rgb = edge_colour.rgb;

        if(alternate_colour && (edge_point_index % 2))
        {
            Vec3 temp = center_colour_rgb;
            center_colour_rgb = edge_colour_rgb;
            edge_colour_rgb = temp;

        }

        Vec2 center_point = center;
        Vec2 edge_point = dodecagon_points.points[edge_point_index];
        Line_Quad line = line_quad_points(center_point, edge_point, transform);
        Vec4 start_colour = vec4(center_colour_rgb, center_colour.a);
        Vec4 end_colour = vec4(edge_colour_rgb, edge_colour.a);
            
        push_shape_blocky_blend(render_commands, vec2(0.0f), vec2(1.0f), start_colour, end_colour, line.points, array_count(line.points), transform);
    }

    end_temporary_memory(temp_mem);
}

internal void draw_shield_around_entity(Game_Render_Commands *render_commands, Level_Stage *stage, Vec2 position, Vec2 scale, Memory_Arena *temp_arena, f32 alpha = 0.5f)
{
    Vec2 entity_scale = scale;
    scale = entity_scale*1.75f;
    position = position - (scale - entity_scale)/2.0f;

    f32 thickness = 4.0f;
    Vec2 center = position + scale/2.0f;
    Render_Transform transform = draw_order_transform(entity_draw_order_shield_around_entity, thickness_transform(thickness));

    draw_shield(render_commands, center, scale.x/2.0f, stage->shield_cover_rotation, temp_arena, vec4(0.5f, 0.0f, 0.0f, alpha*0.5f), vec4(0.0f, 0.0f, 0.5f, alpha), true, transform);
}

internal void start_player_shake(Player *player, Level_Stage *stage)
{
    player->shake_until_time = (stage->time + player_life_lost_shake_time);
}

internal b32 is_player_shaking(Player *player, f32 stage_time)
{
    b32 result = (player->shake_until_time >= stage_time);
    return result;
}

internal void update_particles(Game_Render_Commands *render_commands, Level_Stage *stage, Memory_Arena *temp_arena, f32 level_dt)
{
    TIMED_BLOCK();
    
#if 1
    Temporary_Memory temp_mem = begin_temporary_memory(temp_arena);

#if 0
#if TWOSOME_INTERNAL
    char buffer[256];
    sprintf(buffer, "Particles: %d", stage->active_particles_count);
    DEBUG_push_text(render_commands, buffer, 24, 650, 550, vec4(1.0f));
#endif
#endif
    
    // NOTE: Need four triangles per particle, so can soften edges
    u32 verts_count = stage->active_particles_count*24;
    Vertex_XY_RGBA *verts = push_array(temp_arena, Vertex_XY_RGBA, verts_count);        

    f32 life_decrease_t = 0.15f;
    // NOTE: To help keep number of particles down we start increasing the
    // rate at which their life decays after x particles
    if(stage->active_particles_count > active_particles_soft_max)
    {
        f32 extra_t = ((f32)((stage->active_particles_count) - active_particles_soft_max) / 100.0f);
        extra_t = clamp01(extra_t);
        life_decrease_t = interpolate(0.15f, 0.5f, extra_t);
    }
    
    f32 life_decrease = level_dt*life_decrease_t;
    
    Vec4 corner_colour = vec4(0.0f);
    
    for(Particle **particle_ptr = &stage->first_active_particle; *particle_ptr; )
    {
        TIMED_BLOCK();
        
        Particle *p = *particle_ptr;

        p->position += (p->velocity * level_dt);
        Vec2 scale = vec2(p->size * p->life);
        Vec2 half_scale = vec2(scale.x*0.5f);
        
        Vec4 center_colour = vec4(p->colour, p->life);

        Vec2 bl = p->position;
        Vec2 tr = p->position + scale;
        Vec2 br = vec2(tr.x, bl.y);
        Vec2 tl = vec2(bl.x, tr.y);
        
        Vec2 center = bl + half_scale;

        f32 bottom_y = bl.y;
        f32 top_y = tr.y;
        f32 left_x = tl.x;
        f32 right_x = tr.x;

        f32 corner_offset;
        {
            f32 half_scale_mag = ( sqrtf( (half_scale.x*half_scale.x) + (half_scale.y*half_scale.y) ) );
            corner_offset = half_scale_mag * p->corner_offset;
        }

        bl += corner_offset;
        
        br.x -= corner_offset;
        br.y += corner_offset;
        
        tr.x -= corner_offset;
        tr.y -= corner_offset;
        
        tl.x += corner_offset;
        tr.y -= corner_offset;
        
        // Bottom Triangles
        *verts++ = vertex_xy_rgba(bl, corner_colour);
        *verts++ = vertex_xy_rgba(vec2(center.x, bottom_y), corner_colour);
        *verts++ = vertex_xy_rgba(center, center_colour);

        *verts++ = vertex_xy_rgba(vec2(center.x, bottom_y), corner_colour);
        *verts++ = vertex_xy_rgba(br, corner_colour);
        *verts++ = vertex_xy_rgba(center, center_colour);
          
        // Right Triangles
        *verts++ = vertex_xy_rgba(br, corner_colour);
        *verts++ = vertex_xy_rgba(vec2(right_x, center.y), corner_colour);
        *verts++ = vertex_xy_rgba(center, center_colour);

        *verts++ = vertex_xy_rgba(vec2(right_x, center.y), corner_colour);
        *verts++ = vertex_xy_rgba(tr, corner_colour);
        *verts++ = vertex_xy_rgba(center, center_colour);
          
        // Top Triangles
        *verts++ = vertex_xy_rgba(tr, corner_colour);
        *verts++ = vertex_xy_rgba(vec2(center.x, top_y), corner_colour);
        *verts++ = vertex_xy_rgba(center, center_colour);

        *verts++ = vertex_xy_rgba(vec2(center.x, top_y), corner_colour);
        *verts++ = vertex_xy_rgba(center, center_colour);
        *verts++ = vertex_xy_rgba(tl, corner_colour);
          
        // Left Triangles
        *verts++ = vertex_xy_rgba(tl, corner_colour);
        *verts++ = vertex_xy_rgba(vec2(left_x, center.y), corner_colour);
        *verts++ = vertex_xy_rgba(center, center_colour);

        *verts++ = vertex_xy_rgba(vec2(left_x, center.y), corner_colour);
        *verts++ = vertex_xy_rgba(center, center_colour);
        *verts++ = vertex_xy_rgba(bl, corner_colour);
            
        p->life -= life_decrease;
        if(p->life <= 0.0f)
        {
            *particle_ptr = p->next;
                
            p->next = stage->first_free_particle;
            stage->first_free_particle = p;

            --stage->active_particles_count;
        }
        else
        {
            particle_ptr = &p->next;   
        }
            
    }

    verts -= verts_count;
    push_vertices(render_commands, (f32 *)verts, verts_count, vertex_flag_xy | vertex_flag_rgba, vec4(1.0f), transform_flags(render_transform_flag_additive_blend, draw_order_transform(entity_draw_order_particles)));
        
    end_temporary_memory(temp_mem);
#endif
}

internal Game_Mode_Result update_game_level(Game_Level *level, Game_Input *input, Memory_Arena *temp_arena, Game_Render_Commands *render_commands, Game_Progress *progress, UI_Context *ui_context, Cursor *cursor, f32 gametime)
{
    TIMED_BLOCK();
    
    Game_Mode_Result result = {};
    result.level_type = level->type;
    
    Level_Stage *stage = &level->stage;    
    Stage_Metadata *stage_meta = &stage->meta;
    
    b32 life_lost = false;
    
    Player *player = &stage->player;   
    
    // NOTE: Level dt can be set to 0 when level is frozen
    f32 level_dt = input->dt;
    if(level_stage_frozen(stage))
    {
        level_dt = 0.0f;
    }
    stage->time += level_dt;
    level->gametime = gametime;
    if(stage_meta->tutorial_type == tutorial_type_null)
    {
        stage->active_time += level_dt;
    }

    if(level->lives == 0)
    {
        set_level_game_over(level, progress, true, &result);
    }
    
    f32 stage_time = stage->time;

    //
    // NOTE: Input
    //
    b32 change_colour_input_received = false;
    b32 activate_shield_input_received = false;
    if(!level_stage_frozen(stage))
    {
        change_colour_input_received = (first_button_press(input->change_colour_button));
        activate_shield_input_received = (first_button_press(input->activate_shield_button));        
    }

    //
    // NOTE: Player Update
    //
    update_player(player, level, change_colour_input_received, level_dt);
    
    //
    // NOTE: Update Connector
    //
    for(int connector_index = 0; connector_index < stage->num_connectors; ++connector_index)
    {     
        Connector *connector = &stage->connectors[connector_index];
        Vec2 connectors_scale = vec2(stage->connectors_scale);
        connector->scale = connectors_scale;
            
        Vec2 last_connector_position = connector->position;

        update_wandering_velocity(connector->position, stage->connectors_scale, &connector->velocity);
        
        Vec2 move = (connector->velocity * connector->speed) * level_dt;
        // NOTE: If we're the following connector then seek player
        if(connector_index == stage->connector_following_player_index)
        {
            Vec2 player_center = (player->position + player->scale/2.0f);
            Vec2 connector_center = (connector->position + (vec2(stage->connectors_scale) / 2.0f));
            Vec2 connector_to_player = player_center - connector_center;
            
            Vec2 dir = normalize_or_zero(connector_to_player);

            move = dir * (connector->speed * 2.0f) * level_dt;

            // NOTE: Don't want to overshoot player
            if(magnitude(move) > magnitude(connector_to_player))
            {
                move = connector_to_player;
            }

            // NOTE: Want to continue in the direction towards player
            if(magnitude(dir) > 0.0f)
            {
                connector->velocity = dir;
            }
        }
        
        connector->position += move;
        connector->speed = connector->initial_speed;
            
        connector->aabb = make_aabb(last_connector_position, connector->position, connectors_scale);
        connector->obb = make_obb(last_connector_position, connector->position, 0.0f, 0.0f, vec2(0.0f), connectors_scale);

        connector->touching_shield = false;
        
        connector->speed = connector->initial_speed;

        f32 border_thickness = connector_default_thickness;
        f32 last_touch_thickness_scaling = 6.0f;
        if(connector->swell_until_time >= stage->time)
        {
            f32 t = 1.0f - ((connector->swell_until_time - stage_time));
            border_thickness += sinf((t*PI)) * connector_touch_max_thickness;            
        }

        if(connector->touching_spinner_laser)
        {
            f32 scale_offset = 0.0f;
            //f32 scale_offset = normalize_range_neg11(sinf(stage_time*40.0f)) * 0.8f;
                    
            Vec2 glow_scale = vec2(stage->connectors_scale*(1.25f + scale_offset));
            Vec2 glow_position = connector->position + stage->connectors_scale/2.0f;// - ((glow_scale - vec2(stage->connectors_scale)) / 2.0f);
            //push_rect(render_commands, glow_position, glow_scale, vec4(safe_element_colour, 0.75f));
            
            draw_laser_shield(render_commands, glow_position, glow_scale, vec4(safe_element_colour, 0.75f), /*1.75f*/2.0f);

            connector->touching_spinner_laser = false;
        }

        // NOTE: Draw connector
        {
            // NOTE: Draw background
            Vec2 bkg_scale = connectors_scale - border_thickness*2.0f;
            Vec2 bkg_pos = connector->position + border_thickness;
            push_rect(render_commands, bkg_pos, bkg_scale, vec4(get_connect_colour(connector->connect_colour), 0.25f));            
            push_rect_outline(render_commands, connector->position, connectors_scale, vec4(get_connect_colour(connector->connect_colour), 1.0f), thickness_transform(border_thickness, transform_flags(render_transform_flag_half_line_corner_indent)));
        }
        
        if(connector->connected)
        {            
            if(connector->connected_time >= connector_connect_time)
            {
                remove_connector_connection(level, connector, false);
            }
            else
            {
                // NOTE: Draw dodecagon inside connector when connected 
                f32 last_touch_t = clamp01(connector->connected_time/connector_connect_time);
                
                f32 max_scale = connector->scale.x - border_thickness*2.0f;
                f32 circle_scale = (max_scale - (max_scale * last_touch_t));
                
                Vec2 circle_pos = connector->position + connectors_scale/2.0f - vec2(circle_scale/2.0f);

                Vec2 player_connector_center_diff = ((player->position + player_size/2.0f) - (connector->position + connectors_scale/2.0f));

                // NOTE: Want the circle to be in the direction of the player
                f32 circle_radius = circle_scale/2.0f;
                f32 player_connector_center_dist = magnitude(player_connector_center_diff);
                f32 max_circle_size_normalized = 0.875f;
                Vec2 circle_t = ((normalize_or_zero(player_connector_center_diff)*max_circle_size_normalized) * clamp01(player_connector_center_dist / (circle_radius)));                
                
                circle_t.x = normalize_range_neg11(circle_t.x);
                circle_t.y = normalize_range_neg11(circle_t.y);
                
                push_dodecagon_blocky_blend(render_commands, circle_pos, vec2(circle_scale), vec4(get_connect_colour(connector->connect_colour), /*0.25f*//*0.5f*/0.75f), vec4(safe_element_colour, 0.5f), blocky_blend_center_transform(circle_t));
                
                push_dodecagon_outline(render_commands, circle_pos, vec2(circle_scale), vec4(safe_element_colour, 1.0f), transform_flags(render_transform_flag_joined_outline, thickness_transform(2.5f)));
            }
        }
    }

    //
    // NOTE: Update Spinners
    //
    {
        for(int spinner_index = 0; spinner_index < stage->num_spinners; ++spinner_index)
        {
            Spinner *spinner = &stage->spinners[spinner_index];
            spinner->total_lasers = 0;
            {
                f32 active_elapsed = (stage_time - spinner->active_time);
                spinner->total_lasers = 1 + (int)(active_elapsed/spinner_laser_to_activate_time);
                spinner->total_lasers = min(spinner->total_lasers, spinner->max_lasers);
            }

            Vec2 last_position = spinner->position;
                
            update_wandering_velocity(spinner->position, spinner_size, &spinner->velocity);
            spinner->position += (spinner->velocity * spinner->speed) * level_dt;
            spinner->speed = 35.0f;

            spinner->aabb = make_aabb(last_position, spinner->position, vec2(spinner_size));

            spinner->rotation += (0.375f * spinner->rotation_direction * level_dt) * (spinner->touching_shield ? 0.25f : 1.0f);
            Vec2 spinner_scale = vec2(spinner_size);

            f32 spinner_alpha = clamp01((stage_time - spinner->active_time) / 0.25f);
            
            // NOTE: Main body
            {
                push_dodecagon(render_commands, spinner->position, spinner_scale, vec4(safe_element_colour, spinner_alpha));

                Vec2 glow_scale = spinner_scale*2.0f;
                Vec2 glow_position = spinner->position - (glow_scale - spinner_scale)/2.0f;
                
                push_dodecagon_blocky_blend(render_commands, glow_position, glow_scale, vec4(danger_element_colour*spinner_alpha, 0.0f), vec4(danger_element_colour*0.1f*spinner_alpha, 0.0f), transform_flags(render_transform_flag_additive_blend));
                
                if(spinner->touching_shield)
                {
                    f32 shield_alpha = spinner_alpha * normalize_range_neg11(sinf((stage_time - spinner->active_time)*10.0f));

                    draw_shield_around_entity(render_commands, stage, spinner->position, spinner_scale, temp_arena, (0.75f - 0.125f*shield_alpha));
                }

                spinner->touching_shield = false;

                // NOTE: Icon in the middle
                {
                    Vec2 icon_scale = spinner_scale*0.75f;
                    f32 spacing = 1.0f + normalize_range_neg11(sinf(stage_time*6.0f))*1.5f;
                    draw_laser_shield(render_commands, spinner->position + (spinner_scale/2.0f), icon_scale, vec4(safe_element_colour*0.75f, spinner_alpha), spacing);
                }

            }
            
            //
            // NOTE: Spinner Lasers 
            //
            {
                Vec2 laser_position = (spinner->position + vec2(spinner_size - spinner_laser_thickness, (spinner_size - spinner_laser_thickness)/2.0f));
                    
                Vec2 laser_scale = vec2(spinner_laser_width, spinner_laser_thickness);
                Vec2 laser_rotation_pt = (spinner->position + spinner_size/2.0f) - laser_position;

                f32 lasers_rotation[] =
                {
                    spinner->rotation + (PI/2.0f),
                    spinner->rotation + (1.5f*PI),
                    spinner->rotation + PI,
                    spinner->rotation + (2.0f*PI)
                };
                assert(spinner->total_lasers <= array_count(lasers_rotation));
                    
                Vec2 spinner_center = spinner->position + spinner_size/2.0f;
                for(int laser_index = 0; laser_index < spinner->total_lasers; ++laser_index)
                {
                    Spinner_Laser *laser = &spinner->lasers[laser_index];
                    laser->rotation = lasers_rotation[laser_index];

#if 0
                    if(laser_index > 0)
                    {
                        break;
                    }
                    
                    laser->rotation = 0.0f;
#endif
                    
                    if(laser->spawn_time < 0.0f)
                    {
                        laser->spawn_time = stage_time;
                    }

                    float active_elapsed = stage_time - laser->spawn_time;
                    float to_active_t = active_elapsed / time_for_spinner_laser_activation;
                
                    Vec3 laser_colour = danger_element_colour;
                    float laser_alpha = 1.0f;
                    if(to_active_t < 1.0f)
                    {
                        laser_colour = safe_element_colour;
                        laser_alpha *= to_active_t;
                        laser->active = false;
                    }
                    else
                    {
                        laser->active = true;
                        laser_colour = danger_element_colour;
                        laser->obb = make_obb(laser_position, laser_scale, laser->rotation, laser_rotation_pt);
                    }

                    Render_Transform laser_transform = rotation_transform(laser->rotation, laser_rotation_pt, draw_order_transform(entity_draw_order_laser_spinner));

                    push_rect(render_commands, laser_position, laser_scale, vec4(laser_colour+0.5f, laser_alpha*0.5f), laser_transform);
                    
                    push_rect_outline(render_commands, laser_position, laser_scale, vec4(laser_colour, laser_alpha), thickness_transform(4.0f, rotation_transform(laser->rotation, laser_rotation_pt, draw_order_transform(entity_draw_order_laser_spinner))));
                    
                }
            }
        }
            
    }

    //
    // NOTE: Update Spikes Barrier
    //
    for(Spikes_Barrier **spikes_barrier_ptr = &stage->first_active_spikes_barrier; *spikes_barrier_ptr;)
    {
        Spikes_Barrier *barrier = *spikes_barrier_ptr;

        b32 scrolling_finished = false;

        // NOTE: Barrier is made up of lots of spikes, the safe ones that are larger, and smaller dangerous ones
        if(barrier->spawn_time <= stage->time)
        {
            barrier->active = true;
        
            f32 safe_top_y = barrier->safe_spikes_y + spikes_barrier_safe_height;
            Vec2 safe_spikes_scale = vec2(spikes_barrier_safe_width, safe_top_y - barrier->safe_spikes_y);
            Vec2 safe_spikes_pos = vec2(0.0f, barrier->safe_spikes_y);

            f32 last_spikes_barrier_x = barrier->x;

            Vec2 last_safe_spikes_pos = vec2(0.0f, barrier->safe_spikes_y);

            barrier->x += barrier->direction * (level_dt * spikes_barrier_speed);
            if(barrier->direction > 0.0f)
            {
                last_safe_spikes_pos.x = last_spikes_barrier_x;
                safe_spikes_pos.x = barrier->x;
                    
                if(barrier->x > virtual_screen_width)
                {
                    scrolling_finished = true;
                    // NOTE: Scrolling barrier should never have got to other end of screen
                    // without hitting player, but if it does, deal with it
                    invalid_code_path;
                }
            }
            else
            {
                last_safe_spikes_pos.x = (last_spikes_barrier_x - spikes_barrier_safe_width);
                safe_spikes_pos.x = (barrier->x - spikes_barrier_safe_width);
                
                if(barrier->x < 0.0f)
                {
                    scrolling_finished = true;
                    // NOTE: Scrolling barrier should never have got to other end of screen
                    // without hitting player, but if it does, deal with it
                    invalid_code_path;
                }
            }            
            
            f32 danger_triangle_size = spikes_barrier_safe_width;
            f32 offset = (barrier->direction > 0.0f ? -1.0f : 1.0f);

            // NOTE: The barriers below and above the safe spikes
            Vec2 barrier_bottom_pos = vec2(safe_spikes_pos.x + (offset * danger_triangle_size), 0.0f);
            Vec2 last_barrier_bottom_pos = vec2(last_safe_spikes_pos.x + (offset * danger_triangle_size), 0.0f);
            
            Vec2 barrier_bottom_scale = vec2(danger_triangle_size, barrier->safe_spikes_y);

            Vec2 barrier_top_pos = vec2(safe_spikes_pos.x + (offset * danger_triangle_size), safe_top_y);
            Vec2 last_barrier_top_pos = vec2(last_safe_spikes_pos.x + (offset * danger_triangle_size), safe_top_y);
            
            Vec2 barrier_top_scale = vec2(danger_triangle_size, virtual_screen_height - safe_top_y);            
            
            barrier->danger_bottom_aabb = make_aabb(last_barrier_bottom_pos, barrier_bottom_pos, barrier_bottom_scale);
            barrier->danger_top_aabb = make_aabb(last_barrier_top_pos, barrier_top_pos, barrier_top_scale);
            barrier->safe_spikes_aabb = make_aabb(last_safe_spikes_pos, safe_spikes_pos, safe_spikes_scale);

            Vec3 safe_center_colour = get_connect_colour(barrier->safe_cursor_colour);

            // NOTE: Draw a load of triangles
            {
                f32 master_rotation = (stage_time - barrier->spawn_time) * 18.0f;
                Temporary_Memory temp_mem = begin_temporary_memory(temp_arena);

                s32 bottom_triangle_count = (s32)round_up(barrier_bottom_scale.y / danger_triangle_size);
                s32 top_triangle_count = (s32)round_up(barrier_top_scale.y / danger_triangle_size);

                s32 total_triangle_count = (bottom_triangle_count + top_triangle_count);
                
                u32 triangle_vertices_count = 3*total_triangle_count;
                Vec2 *triangles_batch = push_array(temp_arena, Vec2, triangle_vertices_count);
                Vec2 *current_triangles = triangles_batch;

                // NOTE: Batch rendering of all these triangles, Two points for each line of triangle
                u32 border_vertices_count = triangle_vertices_count*2;
                Vec2 *borders_batch = push_array(temp_arena, Vec2, border_vertices_count);
                Vec2 *current_borders = borders_batch;

                b32 spawn_particles = false;
                if((stage_time - barrier->last_particles_spawn_time) > 1.0f)
                {
                    spawn_particles = true;

                    barrier->last_particles_spawn_time = stage_time;
                }

                f32 particle_size = 10.0f;//10.0f;
                
                // NOTE: Draw the bottom "danger" set of triangles
                {
                    for(s32 bottom_triangle_index = 0; bottom_triangle_index < bottom_triangle_count; ++bottom_triangle_index)
                    {
                        f32 triangle_x = barrier_bottom_pos.x - (danger_triangle_size - barrier_bottom_scale.x);
                        f32 triangle_y = ((barrier_bottom_pos.y + barrier_bottom_scale.y) - (danger_triangle_size * (bottom_triangle_index + 1)));

                        f32 rotation = master_rotation + (f32)(bottom_triangle_index + 1);
                        
                        Vec2 md = vec2(triangle_x, triangle_y) + vec2(danger_triangle_size*0.5f);

                        Vec2 pt1, pt2, pt3;
                        f32 r = rotation + PI/2.0f;
                        pt1 = md + vec2(cosf(r), sinf(r))*(danger_triangle_size*0.5f);
                        r += (PI/2.0f + PI/4.0f);
                        pt2 = md + vec2(cosf(r), sinf(r))*(danger_triangle_size*0.5f);
                        r = rotation - PI/4.0f;
                        pt3 = md + vec2(cosf(r), sinf(r))*(danger_triangle_size*0.5f);
                        
                        *current_triangles++ = pt1;
                        *current_triangles++ = pt2;
                        *current_triangles++ = pt3;
                        
                        *current_borders++ = pt1;
                        *current_borders++ = pt2;
                        *current_borders++ = pt2;
                        *current_borders++ = pt3;                        
                        *current_borders++ = pt3;
                        *current_borders++ = pt1;

                        if(spawn_particles)
                        {
                            //f32 purp_offset = ((f32)random(level->rng, 0, 25) / 100.0f);
                            //Vec3 colour = vec3(1.0f - purp_offset, 0.0f + purp_offset, 1.0f - purp_offset);
                            
                            //colour = interpolate(danger_element_colour, danger_element_colour*0.25f, ());
                            Vec3 colour = danger_element_colour;
                            spawn_exploding_particles(level, md, 2, colour, particle_size, 10, 100);
                        }
                    }                    
                }

                // NOTE: Draw the top "danger" set of triangles
                {
                    for(s32 top_triangle_index = 0; top_triangle_index < top_triangle_count; ++top_triangle_index)
                    {
                        f32 triangle_x = barrier_top_pos.x - (danger_triangle_size - barrier_top_scale.x);
                        f32 triangle_y = barrier_top_pos.y + (danger_triangle_size * top_triangle_index);

                        Vec2 md = vec2(triangle_x, triangle_y) + vec2(danger_triangle_size*0.5f);
                        Vec2 pt1, pt2, pt3;

                        f32 rotation = master_rotation + (f32)(top_triangle_index + 1);
                        
                        f32 r = rotation + PI/2.0f;
                        pt1 = md + vec2(cosf(r), sinf(r))*(danger_triangle_size*0.5f);
                        r += (PI/2.0f + PI/4.0f);
                        pt2 = md + vec2(cosf(r), sinf(r))*(danger_triangle_size*0.5f);
                        r = rotation - PI/4.0f;
                        pt3 = md + vec2(cosf(r), sinf(r))*(danger_triangle_size*0.5f);
                        
                        *current_triangles++ = pt1;
                        *current_triangles++ = pt2;
                        *current_triangles++ = pt3;
                        
                        *current_borders++ = pt1;
                        *current_borders++ = pt2;
                        *current_borders++ = pt2;
                        *current_borders++ = pt3;
                        *current_borders++ = pt3;
                        *current_borders++ = pt1;

                        if(spawn_particles)
                        {
                            //f32 purp_offset = ((f32)random(level->rng, 0, 25) / 100.0f);
                            //Vec3 colour = vec3(1.0f - purp_offset, 0.0f + purp_offset, 1.0f - purp_offset);
                            Vec3 colour = danger_element_colour;//*0.75f;
                            spawn_exploding_particles(level, md, 2, colour, particle_size, 10, 100);
                        }
                    }
                }

                push_vertices(render_commands, (f32 *)triangles_batch, triangle_vertices_count, vertex_flag_xy, vec4(1.0f, 0.0f, 1.0f, 1.0f), draw_order_transform(entity_draw_order_spikes_barrier));
                push_line_batch(render_commands, borders_batch, border_vertices_count, vec4(danger_element_colour*0.4f, 1.0f), thickness_transform(3.0f, draw_order_transform(entity_draw_order_spikes_barrier)));
                
                end_temporary_memory(temp_mem);                
            }

            f32 thickness_t = (sinf(stage_time*10.0f) + 1.0f) / 2.0f;

            f32 thickness = 4.5f*(1.0f - thickness_t) + ((spikes_barrier_safe_width*0.45f)*thickness_t);

            /*DEBUG_draw_aabb(render_commands, &barrier->safe_center_aabb);
            DEBUG_draw_aabb(render_commands, &barrier->danger_bottom_aabb);
            DEBUG_draw_aabb(render_commands, &barrier->danger_top_aabb);*/

            // NOTE: Safe triangles
            {
                f32 triangle_size = safe_spikes_scale.y / 4.0f;

                f32 y = safe_spikes_pos.y;
                f32 line_thickness = 5.0f * (1.0f - thickness_t) + 8.0f*thickness_t;

                Vec2 triangle_scale = vec2(triangle_size);
                Vec4 glow_colour = vec4(danger_element_colour, (1.0f - thickness_t)*0.75f);
                Vec4 glow_inner_colour = glow_colour;
                Vec4 glow_outer_colour = vec4(glow_colour.rgb, glow_colour.a*0.1f);
                
                Render_Transform safe_triangle_transform = thickness_transform(line_thickness, draw_order_transform(entity_draw_order_spikes_barrier));
                if(barrier->direction > 0.0f)
                {
                    safe_triangle_transform = rotation_transform(-PI/2.0f, triangle_scale/2.0f, safe_triangle_transform);
                }
                else
                {
                    safe_triangle_transform = rotation_transform(PI/2.0f, triangle_scale/2.0f, safe_triangle_transform);
                }
                
                for(s32 triangle_index = 0; triangle_index < 4; ++triangle_index)
                {
                    Vec2 triangle_position = vec2(safe_spikes_pos.x, y);                        
                    push_triangle_outline(render_commands, triangle_position, triangle_scale, vec4(safe_center_colour, 1.0f), safe_triangle_transform);
                    push_triangle_blocky_blend(render_commands, triangle_position, triangle_scale, glow_inner_colour, glow_outer_colour, safe_triangle_transform);                    

                    y += triangle_size;    
                }
            }

        }    

        if(scrolling_finished)
        {
            *spikes_barrier_ptr = barrier->next;
            barrier->next = stage->first_free_spikes_barrier;
                    
            barrier->active = false;
            stage->first_free_spikes_barrier = barrier;
        }
        else
        {
            spikes_barrier_ptr = &barrier->next;
        }
    }
        
    //
    // NOTE: Shield Level
    //
    if(level->type >= game_level_type_shield_intro)
    {
        stage->shield_cover_rotation += level_dt;

        update_flashing_value(&stage->expanding_shield_flash, level_dt);
        
        //
        // NOTE: Shield
        //
        for(Shield **shield_ptr = &stage->first_active_shield; *shield_ptr;)
        {
            Shield *shield = *shield_ptr;

            shield->rotation += level_dt * 0.5f;
            
            b32 destroyed_shield = false;

            if(!shield->active)
            {
                if(activate_shield_input_received)
                {
                    shield->active = true;
                    shield->flash = make_flashing_value(1.0f, false);
                    shield->activated_time = stage_time;
                    if(stage_meta->tutorial_type == tutorial_type_using_shield)
                    {
                        stage->tutorial.activated_shield = true;
                        stage_meta->tutorial_type = tutorial_type_null;
                    }

                    add_level_sound_echo(level, play_sound(level->audio, asset_shield_deployed), 0.25f, level_sound_echo_mode_default, 0.375f);
                }
            }
            
            if(shield->active)
            {
                shield->time_left = shield_active_time - (stage_time - shield->activated_time);
                shield->normalized_time_left = shield->time_left/shield_active_time;
                if(shield->time_left <= 0.0f)
                {
                    *shield_ptr = shield->next;
                    shield->next = stage->first_free_shield;
                    stage->first_free_shield = shield;
                    destroyed_shield = true;
                }
            }
            else
            {                                        
                shield->radius += (level_dt * 10.0f) * shield->radius;
                //shield->radius += (level_dt * 7.5f) * shield->radius;
                shield->radius = clamp(shield->radius, 0.0f, shield_max_radius);

                shield->center = (player->position + player->scale/2.0f);

                // NOTE: Sound
                {
                    f32 t = 0.2f + (shield->radius / (shield_max_radius / 2.0f));
                    t = clamp01(t);
                    f32 bpm_picked = 0.0f;
                    if(update_bpm(stage, t, 60.0f, &shield->last_beat_time, &bpm_picked))
                    {
                        add_level_sound_echo(level, play_sound(level->audio, asset_expanding_shield, 0, 1.0f, shield->beat, t), 0.5f);
                        ++shield->beat;
                        shield->beat %= 2;
                    }
                    
                    // NOTE: Want the shield flash to be in time with the beat
                    set_time_for_one_flash(&stage->expanding_shield_flash, (60.0f / bpm_picked)*1.0f);
                }
            }

            if(!destroyed_shield)
            {                    
                shield_ptr = &shield->next;
            }
        }

        //
        // NOTE: Update Collectable
        //
        {
            Collectable *collectable = &stage->collectable;
            if(!collectable->active)
            {
                b32 activate_collectable = false;
                if(stage_meta->tutorial_type == tutorial_type_using_shield && !stage->tutorial.activated_shield)
                {
                    // NOTE: In tutorial level we make the collectable appear quicker
                    if(!stage->tutorial.got_shield && stage_time >= tutorial_time_before_collectable_appears)
                    {
                        activate_collectable = true;
                    }
                }
                else
                {
                    if((stage->active_time - collectable->deactivated_time) >= collectable_deactivated_time)
                    {
                        activate_collectable = true;
                    }
                }
                    
                if(activate_collectable)
                {
                    collectable->active = true;
                    collectable->activated_time = stage->active_time;

                    f32 collectable_size_mag = magnitude(vec2(collectable_size));
                    // NOTE: Spawn the collectable in the middle third of the screen (along the x), so they're away from the scrolling line spawning
                    collectable->position.x = (f32)((virtual_screen_width / 3) + random(level->rng, 0, virtual_screen_width / 3));

                    // NOTE: Normally spawn collectable within height of screen, but in tutorial want to avoid spawning on top player
                    s32 spawn_y_min = 0;
                    s32 spawn_y_max = virtual_screen_height;
                    
                    if(stage_meta->tutorial_type == tutorial_type_using_shield)
                    {
                        if(player->position.y > virtual_screen_height/2.0f)
                        {
                            // NOTE: Spawn below player
                            spawn_y_max = (s32)(player->position.y - player->scale.y);
                        }
                        else
                        {
                            // NOTE: Spawn above player
                            spawn_y_min = (s32)(player->position.y + player->scale.y*2.0f);
                        }
                    }

                    collectable->position.y = (f32)(random(level->rng, spawn_y_min, spawn_y_max - (s32)collectable_size));
                }
            }
            else 
            {
                
                f32 activated_elapsed_time = (stage->active_time - collectable->activated_time);
                    
                if(activated_elapsed_time >= collectable_activated_lifetime)
                {
                    collectable->active = false;
                    collectable->deactivated_time = stage->active_time;
                }
                else
                {
                    Vec3 colour = safe_element_colour;

                    // NOTE: Want it to got at full speed rotation about 3/4 way through its' life
                    f32 collectable_rotation_speed_t = 0.1f + ((activated_elapsed_time / (collectable_activated_lifetime - (collectable_activated_lifetime/4.0f)) ) * 0.9f);
                    collectable_rotation_speed_t = clamp01(collectable_rotation_speed_t);
                    // NOTE: When in tutorial the collectable never fades out, so it should always rotate at full speed
                    if(stage_meta->tutorial_type == tutorial_type_using_shield)
                    {
                        collectable_rotation_speed_t = 1.0f;
                    }

                    collectable->rotation += (level_dt * (PI*2.0f) * 1.75f * collectable_rotation_speed_t);

                    // NOTE: Sound
                    {
                        f32 t = 0.1f + collectable_rotation_speed_t;
                        t = clamp01(t);

                        if(update_bpm(stage, t, 60.0f, &collectable->last_beat_time))
                        {
                            add_level_sound_echo(level, play_sound(level->audio, asset_shield_collectable), 0.25f, level_sound_echo_mode_default, 0.375f);                            
                        }                        
                    }

                    Star_Points star = get_star_points();
                    {
                        // NOTE: Triangulate points
                        Vec2 verts[array_count(star.points)*3];
                        for(u32 point_index = 0; point_index < array_count(star.points); ++point_index)
                        {
                            verts[point_index*3 + 0] = vec2(0.5f);
                            verts[point_index*3 + 1] = star.points[point_index];
                            verts[point_index*3 + 2] = star.points[(point_index + 1) % array_count(star.points)];
                        }
                        
                        push_vertices(render_commands, (f32 *)verts, array_count(verts), vertex_flag_xy, vec2(collectable->position.x, collectable->position.y), vec2(collectable_size), vec4(colour, 0.75f), rotation_transform(collectable->rotation, vec2(collectable_size/2.0f)));

                        // NOTE: Draw border around star
                        {
                            Vec2 border_points[array_count(star.points)];                        
                            transform_vertices(star.points, border_points, array_count(border_points), collectable->position, vec2(collectable_size), collectable->rotation, vec2(collectable_size/2.0f));
                        
                            push_line_points(render_commands, border_points, array_count(border_points), vec4(0.0f, 0.5f, 0.0f, 0.75f), thickness_transform(3.0f, transform_flags(render_transform_flag_wrap_line_points/* | render_transform_flag_additive_blend*/)));
                        }
                        
                        // NOTE: Draw 4 rays coming out of star
                        {
                            Vec2 ray_scale = vec2(collectable_size*1.0f + normalize_range_neg11(sinf(stage->time*2.5f*collectable_rotation_speed_t))*collectable_size*collectable_rotation_speed_t, collectable_size);

                            Vec2 ray_position = collectable->position + vec2(collectable_size/2.0f, 0.0f);

                            for(u32 rot = 0; rot < 4; ++rot)
                            {
                                f32 rotation = (PI/4.0f) + (collectable->rotation + ((PI/2.0f) * rot));

                                push_blocky_blend(render_commands, ray_position, ray_scale, vec4(colour*0.75f, 1.0f), vec4(0.0f), vec2(0.0f, 0.5f), vec2(1.0f, 0.0f), vec2(0.0f, 0.5f), vec2(1.0f, 1.0f), transform_flags(render_transform_flag_additive_blend | render_transform_flag_blend_exclude_second_colour, rotation_transform(rotation, vec2(0.0f, ray_scale.y/2.0f))));
                                
                            }
                        }

                    }

                    
                    
                }
                
            }
        }
    }

    
    //
    // NOTE: Collision Step
    //
    {
        // NOTE: Connectors
        {
            bool32 connector_seeking_player = false;
            for(s32 connector_index = 0; connector_index < stage->num_connectors; ++connector_index)
            {
                Connector *connector = &stage->connectors[connector_index];
                b32 add_connection = false;
                b32 remove_connection = false;
                
                connector->touching_shield = shields_entity_overlap(connector->aabb, stage);

                if(connector->touching_shield && connector->connected)
                {
                    f32 shield_alpha = normalize_range_neg11(sinf(connector->connected_time*10.0f));
                    draw_shield_around_entity(render_commands, stage, connector->position, connector->scale, temp_arena, (0.75f - 0.125f*shield_alpha));
                }
                                
                // NOTE: Spinner collision
                for(int spinner_index = 0; spinner_index < stage->num_spinners; ++spinner_index)
                {
                    Spinner *spinner = &stage->spinners[spinner_index];
                    for(int laser_index = 0; laser_index < array_count(spinner->lasers); ++laser_index)
                    {
                        Spinner_Laser *laser = &spinner->lasers[laser_index];
                        if(laser->active)
                        {
                            if(obb_vs_obb(connector->obb, laser->obb))
                            {
                                connector->touching_spinner_laser = true;
                                break;
                            }
                        }
                    }
                }

                // NOTE: Player collision
                if(aabb_vs_aabb(connector->aabb, player->aabb))
                {
                    player->touching_connector = true;
                    
                    if(connector->connect_colour == player->connect_colour)
                    {                        
                        add_connection = true;

                        // NOTE: Follow cursor
                        if(stage->connector_following_player_index < 0 || stage->connector_following_player_index == connector_index)
                        {
                            stage->connector_following_player_index = connector_index;
                            if(connector->swell_until_time <= stage->time)
                            {
                                connector->swell_until_time = stage->time + connector_thickness_swell_time;
                            }
                            
                            connector_seeking_player = true;
                        }
                    }
                    else
                    {
                        remove_connection = true;
                    }
                }

                if(remove_connection && connector->connected)
                {
                    remove_connector_connection(level, connector, true);
                }
                else if(add_connection)
                {                    
                    add_connector_connection(level, connector, stage_time);
                }
                
            }
            
            if(!connector_seeking_player)
            {
                stage->connector_following_player_index = -1;
            }   
        }

        // NOTE: Spinner
        stage->nearest_player_laser_dist = player_laser_sound_cutoff_dist;
        {
            for(int spinner_index = 0; spinner_index < stage->num_spinners; ++spinner_index)
            {
                Spinner *spinner = &stage->spinners[spinner_index];
                
                // NOTE: Spinner vs Shields
                f32 spinner_radius = spinner_size/2.0f;
                if(shields_entity_overlap(spinner->position + spinner_radius, spinner_radius, stage))
                {
                    spinner->touching_shield = true;
                    
                    for(int laser_count = 0; laser_count < spinner->total_lasers; ++laser_count)
                    {
                        spinner->lasers[laser_count].spawn_time = -1.0f;
                    }
                    spinner->total_lasers = 0;
                }

                // NOTE: Laser vs Player
                for(int laser_index = 0; laser_index < spinner->total_lasers; ++laser_index)
                {
                    Spinner_Laser *laser = &spinner->lasers[laser_index];
                    if(laser->active)
                    {                                                        
                        if(obb_vs_obb(laser->obb, player->obb))
                        {
                            if(!is_player_protected_from_spinner_lasers(stage))
                            {
                                life_lost = true;

                                //laser_colour = safe_element_colour;
                                laser->spawn_time = stage_time;
                            }

                            player->touching_spinner_laser = true;
                        }

                        /// NOTE: Get the shortest distance between the laser and the player
                        { 
                            Vec2 laser_dir = vec2(cosf(laser->rotation), sinf(laser->rotation));
                            Vec2 start = (spinner->position + spinner_size/2.0f);
                            Vec2 end = start + (laser_dir * spinner_laser_width);
                                
                            Vec2 laser_nrm = vec2(-laser_dir.y, laser_dir.x);
                            Vec2 laser_to_player = ((player->position + player->scale/2.0f) - start);

                            r32 player_laser_distance = dot_product(laser_to_player, laser_nrm);
                            
                            // NOTE: Check that point lies within line
                            {
                                Vec2 point_on_line = player->position + (-laser_nrm * player_laser_distance);
                                float dot = dot_product(point_on_line - start, end - start);
                                if(dot < 0.0f)
                                {
                                    player_laser_distance = distance(player->position, start);
                                }
                            }

                            player_laser_distance = fabsf(player_laser_distance);
                            if(player_laser_distance < stage->nearest_player_laser_dist)
                            {
                                stage->nearest_player_laser_dist = player_laser_distance;
                            }
                        }
                    }                        
                }
            }

        }

        //
        // NOTE: Spikes Barrier
        //
        for(Spikes_Barrier **spikes_barrier_ptr = &stage->first_active_spikes_barrier; *spikes_barrier_ptr; )
        {
            Spikes_Barrier *spikes_barrier = *spikes_barrier_ptr;
            
            b32 scrolling_finished = false;
            b32 bad_hit = false;
            b32 hit_by_shield = false;
            if(spikes_barrier->active)
            {
                if(shields_entity_overlap(spikes_barrier->safe_spikes_aabb, stage))
                {
                    scrolling_finished = true;
                    hit_by_shield = true;
                }
                else if(aabb_vs_aabb(player->aabb, spikes_barrier->safe_spikes_aabb))
                {
                    scrolling_finished = true;
                    if(spikes_barrier->safe_cursor_colour != player->connect_colour)
                    {
                        bad_hit = true;
                    }
                }
                else if(aabb_vs_aabb(player->aabb, spikes_barrier->danger_bottom_aabb) || aabb_vs_aabb(player->aabb, spikes_barrier->danger_top_aabb))
                {
                    bad_hit = true;
                    scrolling_finished = true;
                }

                if(bad_hit)
                {
                    life_lost = true;
                }   
            }
                    
            if(scrolling_finished)
            {
                if(bad_hit)
                {
                    spawn_spikes_barrier_death(stage, spikes_barrier, true, hit_by_shield, stage_time, level->mode_arena);
                }
                else
                {
                    spawn_spikes_barrier_death(stage, spikes_barrier, false, hit_by_shield, stage_time, level->mode_arena);
                    add_level_sound_echo(level, play_sound(level->audio, asset_spikes_barrier_safe_hit), 0.25f, level_sound_echo_mode_irregular_beat);

                }
                
                *spikes_barrier_ptr = spikes_barrier->next;
                spikes_barrier->next = stage->first_free_spikes_barrier;
                    
                spikes_barrier->active = false;
                stage->first_free_spikes_barrier = spikes_barrier;

            }
            else
            {
                spikes_barrier_ptr = &spikes_barrier->next;
            }
        }

        // NOTE: Shield (shield_collision & draw_shield)
        {
            Shield *shield = stage->first_active_shield;
            while(shield)
            {
                f32 thickness = (shield->radius / shield_max_radius) * 129.0f;
                f32 flash_t = 0.0f;
                s32 shield_draw_order = entity_draw_order_shield;
                s32 shield_time_left_draw_order = entity_draw_order_deployed_shield_time_life;
                Vec3 center_colour;
                Vec3 edge_colour;                
                
                // NOTE; When expanding shield we resize it based on collision with connectors
                if(!shield->active)
                {
                    Vec2 blast_position = (player->position + player->scale/2.0f) - shield->radius;
                    Vec2 blast_center = blast_position + shield->radius;
                    for(int connector_index = 0; connector_index < stage->num_connectors; ++connector_index)
                    {
                        Connector *connector = &stage->connectors[connector_index];

                        f32 dist = 0.0f;
                        if(aabb_circle_overlap(connector->aabb, shield->radius, blast_center, &dist))
                        {
                            if(dist < shield->radius)
                            {
                                shield->radius = dist;
                            }
                        }
                    }

                    shield->radius = max(shield->radius, shield_min_radius);
                    thickness = 4.0f;
                    flash_t = stage->expanding_shield_flash.t;
                                        
                    Vec3 center_colour_original = safe_element_colour;
                    Vec3 edge_colour_original = danger_element_colour;

#if 0
                    center_colour = interpolate(center_colour_original, edge_colour_original, flash_t);
#else
                    center_colour = flash_t < 0.5f ? center_colour_original : edge_colour_original;
#endif
                    edge_colour = center_colour;

                    shield_draw_order = entity_draw_order_expanding_shield;
                    shield_time_left_draw_order = shield_draw_order;
                }
                else
                {
                    center_colour = vec3(0.0f, 1.0f, 0.0f);
                    edge_colour = safe_element_colour;
                }

                Render_Transform transform = draw_order_transform(shield_draw_order, thickness_transform(thickness));
                
                // NOTE: Flash time-left indicator when we're getting close to end of life
                f32 shield_normalized_time_left_before_flashing = 0.35f;
                if(shield->normalized_time_left <= shield_normalized_time_left_before_flashing)
                {                    
                    f32 flash_time_t = clamp01(1.0f - (shield->normalized_time_left/shield_normalized_time_left_before_flashing) );
                    f32 time_for_one_flash = interpolate(1.0f, 0.25f, flash_time_t);
                    activate_flash(&shield->flash, time_for_one_flash);
                }
                update_flashing_value(&shield->flash, level_dt);
                //f32 center_alpha = interpolate(0.75f, 0.0f, shield->flash.t);
                Vec3 outline_colour = interpolate(safe_element_colour, danger_element_colour, shield->flash.t);

                // NOTE: Draw shield
                {
                    f32 alpha_t = 1.0f;
                    // NOTE: For fading out shield moements before it's life finishes
                    if(shield->active)
                    {
                        f32 time_left_for_alpha_to_start_fading = 0.2f;
                        alpha_t = interpolate(0.0f, 1.0f, clamp01(shield->time_left / time_left_for_alpha_to_start_fading));

                        // NOTE: Play deactivate sound, we start it slightly earlier than the disappearing of the shield since it's playing the sound in reverse
                        if(!shield->played_deactivate_sound && shield->time_left <= (time_left_for_alpha_to_start_fading + 0.05f))
                        {
                            add_level_sound_echo(level, play_sound(level->audio, asset_shield_deployed, playing_sound_flag_reverse, 1.5f), 0.25f, level_sound_echo_mode_default, 0.375f);

                            shield->played_deactivate_sound = true;
                        }
                    }
                    
                    draw_shield(render_commands, shield->center, shield->radius, shield->rotation, temp_arena, vec4(center_colour, 0.85f*alpha_t), vec4(edge_colour, 0.5f*alpha_t), false, transform);
                }

                // NOTE: Draw outline circle whose size indicates time left
                {
                    f32 remaining_circle_radius = shield->radius;
                    f32 remaining_circle_thickness = interpolate(6.0f, 12.0f, shield->flash.t);
                    if(shield->active)
                    {
                        // NOTE: Take into account thickness so circle doesn't disappear completely
                        remaining_circle_radius = shield->radius - ((shield->radius - remaining_circle_thickness) * (1.0f - shield->normalized_time_left));
                    }
                    Vec2 remaining_circle_scale = vec2(remaining_circle_radius * 2.0f);
                    Render_Transform remaining_circle_transform = transform_flags(render_transform_flag_outline_gaps | render_transform_flag_joined_outline, thickness_transform(remaining_circle_thickness, draw_order_transform(shield_time_left_draw_order, rotation_transform(shield->rotation, remaining_circle_scale/2.0f))));
                
                    push_dodecagon_outline(render_commands, shield->center - remaining_circle_radius, remaining_circle_scale, vec4(/*danger_element_colour*/outline_colour, 0.75f/*center_alpha*/), remaining_circle_transform);
                }

                shield = shield->next;
            }
        }

        // NOTE: Collectable
        {
            Collectable *collectable = &stage->collectable;
            if(collectable->active)
            {
                AABB collectable_aabb = make_aabb(collectable->position, vec2(collectable_size));
                if(aabb_vs_aabb(collectable_aabb, player->aabb))
                {
                    if(stage_meta->tutorial_type == tutorial_type_using_shield)
                    {
                        stage->tutorial.got_shield = true;
                        stage->tutorial.got_shield_time = stage_time;
                    }
                            
                    collectable->active = false;
                    collectable->deactivated_time = stage->active_time;

                    // NOTE: Add an inactive shield, IF there isn't one already
                    Shield *existing_inactive_shield = 0;                               
                    for(Shield *shield = stage->first_active_shield; shield; shield = shield->next)
                    {
                        if(!shield->active)
                        {
                            existing_inactive_shield = shield;
                        }
                    }

                    // NOTE: Restart the shield flash, so can have any chance of being in time with beat
                    restart_flashing_value(&stage->expanding_shield_flash, true);
                    
                    if(existing_inactive_shield)
                    {
                        existing_inactive_shield->radius = 0.0f;
                    }
                    else
                    {
                        if(!stage->first_free_shield)
                        {
                            stage->first_free_shield = push_struct(level->mode_arena, Shield);
                            stage->first_free_shield->next = 0;
                        }

                        Shield *shield = stage->first_free_shield;
                        stage->first_free_shield = stage->first_free_shield->next;
                        zero_object(Shield, *shield);

                        shield->next = stage->first_active_shield;
                        stage->first_active_shield = shield;
                    }
                }
            }
        }
    }

    //
    // NOTE: Level Background Blobs
    //
    {
        // NOTE: This should happen after connector update because the blobs look at
        // the state of the connector right now.
        assert(array_count(stage->background_blobs) == array_count(stage->connectors) + 1);
        for(u32 blob_index = 0; blob_index < array_count(stage->background_blobs); ++blob_index)
        {
            Level_Background_Blob *b = &stage->background_blobs[blob_index];

            f32 last_life_velocity = b->life_velocity;

            b32 blob_active;
            if(blob_index == player_background_blob_index)
            {
                // NOTE: Players background blob lasts
                blob_active = stage->time < background_blob_active_time;

                b->life_velocity = blob_active ? 1.0f : -1.0f;
            }
            else
            {
                assert(blob_index < player_background_blob_index);
                Connector *our_connector = &stage->connectors[blob_index];
                // NOTE: Blob stays active from last time connector was connected with
                blob_active = (our_connector->connected && our_connector->connected_time < background_blob_active_time);

                b->life_velocity = blob_active ? 1.0f : -1.0f;

                // NOTE: Reposition blob over our connector when we've faded out
                if(b->life_velocity > 0.0f && last_life_velocity < 0.0f && b->life == 0.0f)
                {
                    b->position = our_connector->position;
                    b->initial_scale = our_connector->scale.x;
                    if(random_boolean(level->rng))
                    {
                        b->colour = vec3(1.0f, 0.0f, 0.0f);
                    }
                    else
                    {
                        b->colour = vec3(0.0f, 0.0f, 1.0f);
                    }
                }
            }
#if 0
            f32 k = 0.05f;
            if(b->life_velocity < 0.0f)
            {
                // NOTE; Goes faster when dying down
                k *= 2.0f;                
            }
#else
            f32 k = 0.15f;
            if(b->life_velocity < 0.0f)
            {
                // NOTE; Goes faster when dying down
                k *= 1.25f;
            }
#endif
            f32 last_life = b->life;
            b->life += level_dt * k * b->life_velocity;
            b->life = clamp01(b->life);

            //f32 alpha = 0.625f*b->life;
            f32 alpha = 0.95f*b->life;

            f32 size = background_blob_center_square_size;

            if(last_life > 0.0f)
            {
                // NOTE: Draw several squares of increasing size (the center one square being just an outline)
                for(u32 i = 0; i < 8; ++i)
                {
                    f32 a = (alpha / (i + 1));
                    f32 s = size*(i + 1);//size - (i*(size));
                    Vec2 p = (b->position + b->initial_scale/2.0f) - ((vec2(s) - size)/2.0f);

                    if(i == 0)
                    {
                        push_rect_outline(render_commands, p, vec2(s), vec4(b->colour, a), thickness_transform(2.0f, transform_flags(render_transform_flag_half_line_corner_indent/* | render_transform_flag_additive_blend*/, draw_order_transform(entity_draw_order_background_blob))));    
                    }
                    else
                    {
                        push_rect(render_commands, p, vec2(s), vec4(b->colour, a), thickness_transform(2.0f, transform_flags(render_transform_flag_half_line_corner_indent/* | render_transform_flag_additive_blend*/, draw_order_transform(entity_draw_order_background_blob))));
                    }
                    
                }
            }
        }
    }

    // NOTE: Shake player during stage transition (which also doesn't allow player to lose
    // life during stage transition)
    if(stage->stages_in_transition)
    {
        start_player_shake(player, stage);        
    }

    //
    // NOTE: Things to happen when life lost, we don't allow life to be lost when player is shaking
    // because either they've already just lost it or there's a stage transition happening
    //
    if(life_lost && !is_player_shaking(player, stage_time))
    {        
        // NOTE: Spinner lasers become de-activated
        for(int spinner_index = 0; spinner_index < stage->num_spinners; ++spinner_index)
        {
            for(int laser_index = 0; laser_index < array_count(stage->spinners[spinner_index].lasers); ++laser_index)
            {
                stage->spinners[spinner_index].lasers[laser_index].spawn_time = -1.0f;
            }
        }
        // NOTE: Spike barriers destroyed
        free_link_list(Spikes_Barrier, stage->first_active_spikes_barrier, stage->first_free_spikes_barrier);        

        // NOTE: Spawn life loss from player and from lives counter ui
        spawn_life_loss(level, player->position, player->scale, get_connect_colour(player->connect_colour), vec2(max(virtual_screen_width, virtual_screen_height)));

        Vec2 life_display_position = get_life_display_position((level->lives - 1));
        spawn_life_loss(level, life_display_position, vec2(player_life_display_size), safe_element_colour, vec2(player_life_display_size*10.0f));

        start_player_shake(player, stage);

        // NOTE: Do life lost flash
        stage->life_lost_flash_start = gametime;

        add_level_sound_echo(level, play_sound(level->audio, asset_life_lost), 0.5f);

#if !TWOSOME_INTERNAL || 1
        --level->lives;
        level->lives = max(0, level->lives);
#endif
    }

    //
    // NOTE: Timeline Events
    //
    {
        {
            b32 spikes_barrier_event_activated = false;
            if(update_timeline_event_activeness(&stage_meta->spikes_barrier_left, stage_time, true))
            {
                spikes_barrier_event_activated = true;
            }
            if(update_timeline_event_activeness(&stage_meta->spikes_barrier_right, stage_time, true))
            {
                spikes_barrier_event_activated = true;
            }
 
            if(spikes_barrier_event_activated)
            {
                add_level_sound_echo(level, play_sound(level->audio, asset_spikes_barrier_haze), 0.25f, level_sound_echo_mode_loop);   
            }
        }
                    
        assert(array_count(stage_meta->spinners) == array_count(stage->spinners));
        for(int spinner_element_index = 0; spinner_element_index < array_count(stage_meta->spinners); ++spinner_element_index)
        {
            Timeline_Event *spinner_event = &stage_meta->spinners[spinner_element_index];
            if(update_timeline_event_activeness(spinner_event, stage_time))
            {
                spawn_spinner(stage, spinner_event, level->rng);
            }
        }   
    }

    //
    // NOTE: Spikes Barrier Haze
    {
        draw_spikes_barrier_spawn_hints(render_commands, stage->first_active_spikes_barrier, level);
        
        if(stage_meta->spikes_barrier_left.activated)
        {
            draw_spikes_barrier_haze(render_commands, stage, &stage_meta->spikes_barrier_left, false);
        }
        
        if(stage_meta->spikes_barrier_right.activated)
        {
            draw_spikes_barrier_haze(render_commands, stage, &stage_meta->spikes_barrier_right, true);
        }
    }
            
    //
    // NOTE: Render connection lines, do score change, add connectors and change their scale based on stage elapsed time
    //
    {
        f32 time_to_small_size = 7.5f;                
        f32 time_to_small_size_t = clamp01(stage->active_time / time_to_small_size);

        stage->num_connections = 0;

        f32 max_connector_line_thickness = 2.5f;
        
        f32 target_connector_line_thickness = 1.25f;
        if(stage_meta->total_connectors == medium_connectors_count)
        {
            target_connector_line_thickness = 1.5f;
        }
        else if(stage_meta->total_connectors == low_connectors_count)
        {
            target_connector_line_thickness = max_connector_line_thickness;
        }

        f32 connector_line_thickness = interpolate(max_connector_line_thickness, target_connector_line_thickness, time_to_small_size_t);
        connector_line_thickness = max_connector_line_thickness;
        
        for(int connector_index = 0; connector_index < stage->num_connectors; ++connector_index)
        {
            Connector *connector = &stage->connectors[connector_index];
            if(connector->connected)
            {
                ++stage->num_connections;
                if(stage_meta->tutorial_type != tutorial_type_null)
                {
                    connector->last_score_increase_time = stage_time;
                }
                if((stage_time - connector->last_score_increase_time) >= connector_connect_score_increase_time)
                {
                    //
                    // NOTE: Do connector score change
                    //
                    {
                        int score_increase = connector_connected_score_increase;
                        if(connector->touching_shield)
                        {
                            score_increase = connector_connected_in_shield_score_increase;
                        }

                        Vec2 position = connector->position + connector->scale/2.0f;
                        if(connector->velocity.x > 0.0f)
                        {
                            position.x = connector->position.x + connector->scale.x;
                        }
                        else if(connector->velocity.x < 0.0f)
                        {
                            position.x = connector->position.x;
                        }
                        if(connector->velocity.y > 0.0f)
                        {
                            position.y = connector->position.y + connector->scale.y;
                        }
                        else if(connector->velocity.y < 0.0f)
                        {
                            position.y = connector->position.y;
                        }
    
                        change_game_score(level, render_commands, stage_time, position, score_increase);
                        connector->last_score_increase_time = stage_time;                       
                    }
                }

                connector->connected_time += level_dt;

                // NOTE: Draw connection line
                f32 last_touch_t = connector->connected_time/connector_connect_time;
                last_touch_t = clamp01(last_touch_t);
                Vec3 line_colour = get_connect_colour(connector->connect_colour);
                line_colour = line_colour + ((vec3(0.0f, 1.0f, 0.0f) - line_colour) * last_touch_t);

                Vec3 player_line_pos = vec3((player->position + player->scale/2.0f), 0);
                Vec3 connector_line_pos = vec3((connector->position.x + connector->scale.x/2.0f), (connector->position.y + connector->scale.y/2.0f), 0);
                Vec4 colour = vec4(line_colour, 1.0f);
                
                Vec3 connector_to_player_vector = player_line_pos - connector_line_pos;
                Vec3 connector_to_player_line_vector_normalized = normalize(connector_to_player_vector);

                Vec3 connector_to_cursor_line_end = connector_line_pos + (connector_to_player_line_vector_normalized) * (magnitude(connector_to_player_vector) * (0.25f + (0.75f * last_touch_t)));

                f32 spin_t = stage_time + square(last_touch_t*6.0f);
                f32 spin_scaling = stage->connectors_scale + (stage->connectors_scale * last_touch_t);

                f32 line_mag = magnitude(connector_to_cursor_line_end - connector_line_pos);
                Vec2 control_point = (connector_line_pos.xy + ((connector_to_cursor_line_end - connector_line_pos) * 0.5f).xy) + vec2(sinf(spin_t), cosf(spin_t)) * line_mag*0.25f;

                push_curved_line(render_commands, 4, connector_line_pos.xy, vec4(colour.rgb, 0.85f), connector_to_cursor_line_end.xy, vec4(colour.rgb, 0.0f), control_point, thickness_transform(connector_line_thickness, draw_order_transform(entity_draw_order_connecting_line)));                
                
            }
        }

        //
        // NOTE: Changing Colour Tutorial
        //
        if(stage_meta->tutorial_type == tutorial_type_changing_colour)
        {
            if(change_colour_input_received)
            {
                if(stage_meta->tutorial_type == tutorial_type_changing_colour)
                {
                    ++stage->tutorial.colour_changes_count;
                }
            }
            
            if(stage->num_connections > 1)
            {
                // NOTE: End tutorial
                stage_meta->tutorial_type = tutorial_type_null;
            }   
        }

        //
        // NOTE: Update sound
        //
        {
            if(!level_stage_frozen(stage))
            {
                //
                // NOTE: Set spinner laser sound volume based on closest laser distance
                //
                {
                    f32 t = 0.1f + (1.0f - (stage->nearest_player_laser_dist / 200.0f));
                    t = clamp(t, 0.4f, 1.0f);
                    set_volume(stage->spinner_laser_sound, t);
                    set_play_speed(stage->spinner_laser_sound, 1.0f);                
                
                    if(player->touching_spinner_laser)
                    {
                        set_play_speed(stage->spinner_laser_sound, 2.0f);
                    }
                    else if(is_player_protected_from_spinner_lasers(stage))
                    {
                        set_play_speed(stage->spinner_laser_sound, 1.5f);
                    }                  
                }
            }
                        
            //
            // NOTE: Sound Echos
            //
            for(Level_Sound_Echo **sound_echo_ptr = &stage->first_active_sound_echo; *sound_echo_ptr; )
            {
                Level_Sound_Echo *echo = *sound_echo_ptr;

                if(gametime/*stage_time*/ >= echo->time_to_start)
                {
                    // NOTE: We take into account the original volume that the sound was played at
                    f32 volume = echo->volume * echo->sound_info.volume;

                    u32 playing_sound_flags = (echo->sound_info.backwards ? playing_sound_flag_reverse : 0);
                    
                    play_sound(level->audio, echo->sound_info.id, playing_sound_flags, echo->speed, echo->sound_info.variation, volume);

                    if(echo->mode == level_sound_echo_mode_loop)
                    {
                        echo->time_to_start = gametime/*stage_time*/ + (f32)random(level->rng, 20, 30);
                    }
                    else
                    {
                        *sound_echo_ptr = echo->next;
                        echo->next = stage->first_free_sound_echo;
                        stage->first_free_sound_echo = echo;
                    }
                }
                else
                {
                    sound_echo_ptr = &echo->next;
                }
            }
        }
        
        //
        // NOTE: Connectors are added, and scale down as time progresses
        //
        {
            // NOTE: Scale connectors
            {

                f32 min_size = medium_connector_count_scale_min_size;
                if(stage_meta->total_connectors == low_connectors_count)
                {
                    min_size = low_connector_count_scale_min_size;
                }
                else if(stage_meta->total_connectors == medium_connectors_count)
                {
                    min_size = medium_connector_count_scale_min_size;
                }
                else if(stage_meta->total_connectors == high_connectors_count)
                {
                    min_size = high_connector_count_scale_min_size;
                }
                else
                {
                    invalid_code_path;
                }
                
                real32 new_scale = connector_scale_max_size*(1.0f-time_to_small_size_t) + (min_size*time_to_small_size_t);
                
                stage->connectors_scale = new_scale;
            }            
            
            // NOTE: Add connectors
            {
                f32 time_to_all_connectors = 13.0f;
                f32 time_to_all_connectors_t = (stage->active_time / time_to_all_connectors);

                f32 connector_count_t = clamp01(time_to_all_connectors_t);
                if(stage_meta->tutorial_type == tutorial_type_changing_colour)
                {
                    // NOTE: For changing colour tutorial, spawn two connectors of alternating colour coming
                    // from left and right side, in opposite vertical directions
                    if(stage->tutorial.colour_changes_count >= tutorial_colour_changes_player_makes_to_progress)
                    {
                        if(stage->num_connectors == 0)
                        {
                            f32 spawn_y_position = (virtual_screen_height/2.0f) - (stage->connectors_scale/2.0f);
                            spawn_connector(stage, vec2(-stage->connectors_scale, spawn_y_position), vec2(1.0f, -1.0f), connect_colour_black, level->rng);
                            spawn_connector(stage, vec2(virtual_screen_width, spawn_y_position), vec2(-1.0f, 1.0f), connect_colour_white, level->rng);

                            stage->tutorial.connectors_spawned = true;
                        }
                    }
                }
                else if(stage_meta->tutorial_type == tutorial_type_null)
                {
                    int num_connectors_required = (int)(connector_count_t * stage_meta->total_connectors);
                    if(num_connectors_required <= array_count(stage->connectors))
                    {
                        int num_connectors_to_add = num_connectors_required - stage->num_connectors;
                        for(int connector_count = 0; connector_count < num_connectors_to_add; ++connector_count)
                        {                       
                            spawn_connector(stage, level->rng);
                        }
                    }
                }   
            }
        }
    }


    //
    // NOTE: Life Loss
    //
    for(Life_Loss **loss_ptr = &stage->first_active_life_loss; *loss_ptr; )
    {
        Life_Loss *loss = *loss_ptr;

        loss->time_life_left -= input->dt;

        f32 shake_t = clamp01( 1.0f - (loss->time_life_left / life_loss_lifespan_time) );
        Vec2 break_scale = interpolate(loss->scale, loss->large_scale, shake_t);

        Vec3 break_colour = loss->colour;
        Vec2 break_pos = loss->position - (break_scale - loss->scale)/2.0f;

        f32 alpha = (1.0f - shake_t);

        Vec2 md = break_pos + break_scale/2.0f;
        f32 shake = 10.0f;//player->touching_connector ? 8.0f : 2.0f;
        md.x += sinf(loss->time_life_left*shake)*break_scale.x*0.2f;
        md.y += cosf(loss->time_life_left*shake)*break_scale.y*0.2f;

#if 0
        draw_player_shape(render_commands, break_pos, break_scale, md, vec4(break_colour, alpha*0.6f), vec4(danger_element_colour, alpha*0.25f), stage, draw_order_transform(entity_draw_order_life_lost));
#else
        Bendy_Quad_Points bendy_quad = get_bendy_quad_points(0.2f + shake_t*0.6f);        
        push_shape_blocky_blend(render_commands, break_pos, break_scale, vec4(break_colour, alpha), vec4(danger_element_colour, alpha*0.75f), bendy_quad.points, array_count(bendy_quad.points), draw_order_transform(entity_draw_order_life_lost));
#endif

        if(loss->time_life_left > 0.0f)
        {
            loss_ptr = &loss->next;
        }
        else
        {
            *loss_ptr = loss->next;
            loss->next = stage->first_free_life_loss;
            stage->first_free_life_loss = loss;
        }
    }
    
    //
    // NOTE: Update Connection Changes
    //
    {
        for(Connection_Change **connection_change_ptr = &stage->first_active_connection_change; *connection_change_ptr; )
        {
            Connection_Change *change = *connection_change_ptr;

            Render_Transform transform = draw_order_transform(entity_draw_order_connection_change);
            
            f32 change_lifetime = 1.0f;
            f32 lifetime = (stage_time - change->time_spawned);
            f32 life_t = clamp01(lifetime / change_lifetime);

            // NOTE: If lost connection then do a "quaking" quad thing
            if(change->lost_connection)
            {
                Vec2 center = change->position + change->scale/2.0f;
            
                Vec2 target_scale = change->scale * 3.5f;
                Vec2 scale = target_scale*life_t;
                f32 alpha = clamp01(1.0f - life_t);
                Vec2 position = center - scale/2.0f;

                // NOTE: Interpolate between random points (random points are assigned
                // every so often and quad has to interpolate to the new points within that interval)
                f32 morph_interval = 0.125f;
                if((stage_time - change->last_point_change_time) >= morph_interval)
                {
                    s32 angle_buffer = 10;
                    u32 angle = 0;
                    for(u32 angle_index = 0; angle_index < 4; ++angle_index)
                    {
                        u32 angle_start_range = angle + angle_buffer;
                        angle += 90;
                        u32 angle_end_range = angle - angle_buffer;

                        f32 temp = change->next_point_angles[angle_index];
                        change->next_point_angles[angle_index] = degrees_to_radians((f32)random(level->rng, angle_start_range, angle_end_range));
                        change->last_point_angles[angle_index] = temp;
                    }

                    change->last_point_change_time = stage_time;
                }

                Vec2 points[array_count(change->last_point_angles)];
                f32 morph_t = ((stage_time - change->last_point_change_time) / morph_interval);
                for(u32 rand_angle_index = 0; rand_angle_index < array_count(change->last_point_angles); ++rand_angle_index)
                {
                    f32 point_angle = (change->last_point_angles[rand_angle_index] * (1 - morph_t)) + (change->next_point_angles[rand_angle_index] * morph_t);
                
                points[rand_angle_index] = (position + scale/2.0f) + (normalize_or_zero(vec2(cosf(point_angle), sinf(point_angle))) * magnitude(scale/2.0f));
                }

                // NOTE: Draw quaking quad
                {
                    f32 bkg_alpha = alpha*0.75f;
                    push_shape_blocky_blend(render_commands, vec2(0.0f), vec2(1.0f), vec4(danger_element_colour, bkg_alpha), vec4(0.0f, 1.0f, 0.0f, bkg_alpha), points, array_count(points), transform);

                    // NOTE: Outline
                    push_line_points(render_commands, points, array_count(points), vec4(0.0f, 1.0f, 0.0f, alpha), transform_flags(render_transform_flag_wrap_line_points, thickness_transform(3.0f, transform)));                
                }
            }
            
            // NOTE: Draw the "bullet" that shoots out/comes in
            {
                Vec3 colour = safe_element_colour;
                f32 change_t = life_t;
                // NOTE: If it's connection loss, then the "bullet" comes in from the sides
                if(change->lost_connection)
                {
                    change_t = (1.0f - life_t);
                    colour = danger_element_colour;
                }
                
                Vec2 scale = change->ray_scale;
                
                Vec2 position = change->position + change->scale/2.0f;
                position.y -= (scale.y*0.75f);
                
                position = clamp_v2(position, vec2(0.0f), vec2(virtual_screen_width, (virtual_screen_height - scale.y)));
                Vec4 final_colour = vec4(colour*(1.0f - change_t), 1.0f);
                Render_Transform rays_transform = corner_indent_transform(2.0f, transform_flags(render_transform_flag_additive_blend, transform));
                // NOTE: Left
                {
                    Vec2 p = vec2(position.x - (virtual_screen_width*change_t*0.25f) - scale.x, position.y);

                    push_rect(render_commands, p, scale, vec4(colour, (1.0f - life_t)*0.5f), transform);
                    push_blocky_blend_rect(render_commands, p, scale, final_colour, final_colour*0.25f, rays_transform);

                }

                // NOTE: Right
                {
                    Vec2 p = vec2(position.x + (virtual_screen_width*change_t*0.5f), position.y);
                    
                    push_rect(render_commands, p, scale, vec4(colour, (1.0f - life_t)*0.5f), transform);
                    push_blocky_blend_rect(render_commands, p, scale, final_colour*0.25f, final_colour, rays_transform);
                }
            }

            if(lifetime < change_lifetime)
            {
                connection_change_ptr = &change->next;
            }
            else
            {
                // Free it
                *connection_change_ptr = change->next;
                change->next = stage->first_free_connection_change;
                stage->first_free_connection_change = change;
            }
        }   
    }

    //
    // NOTE: Spikes Barrier Death
    //
    {
        for(Spikes_Barrier_Death **spikes_death_ptr = &stage->first_active_spikes_barrier_death; *spikes_death_ptr;)
        {
            Spikes_Barrier_Death *spikes_death = *spikes_death_ptr;

            f32 spikes_death_time = 0.75f;
            if(spikes_death->killed_player)
            {
                spikes_death_time = 1.25f;
            }
            f32 alive_time = (stage_time - spikes_death->spawn_time);
            f32 life_t = clamp01((alive_time / spikes_death_time));

            f32 start_rotation = 0.0f;
            
            if(spikes_death->killed_player)
            {
                f32 triangle_size = spikes_barrier_safe_width;
                f32 triangle_offset = spikes_barrier_triangle_spacing;

                {
                    u32 bottom_triangle_count = (s32)round_up(spikes_death->bottom_parting_y / (triangle_size + triangle_offset));
                    u32 starting_triangle_index = (u32)(life_t*bottom_triangle_count);
                    f32 triangle_y = (spikes_death->bottom_parting_y - ((triangle_size + triangle_offset) * (starting_triangle_index + 1)));

                    for(u32 bottom_triangle_index = starting_triangle_index; bottom_triangle_index < bottom_triangle_count; ++bottom_triangle_index)
                    {
                        push_triangle(render_commands, vec2(spikes_death->x, triangle_y), vec2(triangle_size), vec4(spikes_death->safe_colour, 1.0f - life_t), rotation_transform(-PI/2.0f + start_rotation + (life_t * PI*2.0f), vec2(triangle_size/2.0f)));

                        triangle_y -= (triangle_size + triangle_offset);

                        start_rotation = bottom_triangle_index * (PI*0.2f);
                    }
                }

                {
                    u32 top_triangle_count = (s32)round_up((virtual_screen_width - spikes_death->top_parting_y) / (triangle_size + triangle_offset));
                    u32 starting_triangle_index = (u32)(life_t*top_triangle_count);
                    f32 triangle_y = (spikes_death->top_parting_y + ((triangle_size + triangle_offset) * (starting_triangle_index + 1)));
                    for(u32 top_triangle_index = starting_triangle_index; top_triangle_index < top_triangle_count; ++top_triangle_index)
                    {
                        push_triangle(render_commands, vec2(spikes_death->x, triangle_y), vec2(triangle_size), vec4(spikes_death->safe_colour, 1.0f - life_t), rotation_transform(-PI/2.0f + start_rotation + (life_t * PI*2.0f), vec2(triangle_size/2.0f)));

                        triangle_y += (triangle_size + triangle_offset);

                        start_rotation = top_triangle_index * (PI*0.2f);
                    }
                }
            }
            else
            {
                Vec4 safe_hit_death_colour = vec4(safe_element_colour, 1.0f - life_t);
                safe_hit_death_colour.rgb = interpolate(safe_element_colour, vec3(0.0f), life_t);
                
                Render_Transform safe_hit_death_transform = transform_flags(render_transform_flag_additive_blend, corner_indent_transform(3.0f));

                push_rect(render_commands, vec2(spikes_death->x, -life_t*(spikes_death->bottom_parting_y)), vec2(spikes_barrier_safe_width, spikes_death->bottom_parting_y), safe_hit_death_colour, safe_hit_death_transform);
            
                push_rect(render_commands, vec2(spikes_death->x, spikes_death->top_parting_y + (virtual_screen_height - spikes_death->top_parting_y)*life_t), vec2(spikes_barrier_safe_width, (virtual_screen_height - spikes_death->top_parting_y)), safe_hit_death_colour, safe_hit_death_transform);
                
            }

            if(spikes_death->killed_by_shield)
            {
                f32 shield_alpha = 1.0f - life_t;

                Vec2 safe_area_scale = vec2((spikes_death->top_parting_y - spikes_death->bottom_parting_y));
                Vec2 safe_area_position = vec2(spikes_death->x + spikes_barrier_safe_width/2.0f - (safe_area_scale.x/2.0f), spikes_death->bottom_parting_y);

                draw_shield_around_entity(render_commands, stage, safe_area_position, safe_area_scale, temp_arena, shield_alpha);                
            }

            if(alive_time >= spikes_death_time)
            {
                *spikes_death_ptr = spikes_death->next;
                spikes_death->next = stage->first_free_spikes_barrier_death;
                stage->first_free_spikes_barrier_death = spikes_death;
            }
            else
            {
                spikes_death_ptr = &spikes_death->next;
            }            
        }
    }

    //
    // NOTE: Particles
    //
    update_particles(render_commands, stage, temp_arena, level_dt);

    //
    // NOTE: Draw tutorial control instructions
    //
    if(stage_meta->tutorial_type)
    {
        s32 control_hint_type = control_hint_type_none;
        
        if(stage_meta->tutorial_type == tutorial_type_changing_colour)
        {
            if(!stage->tutorial.connectors_spawned)
            {
                control_hint_type = control_hint_type_colour_change;
            }
        }
        else if(stage_meta->tutorial_type == tutorial_type_using_shield)
        {
            // NOTE: Shield control hint appears after getting shield
            if((stage->tutorial.got_shield && ((stage_time - stage->tutorial.got_shield_time) > 0.75f)  ) && !stage->tutorial.activated_shield)
            {
                control_hint_type = control_hint_type_shield_activate;
            }
        }

        if(control_hint_type != control_hint_type_none)
        {
            update_and_draw_mouse_control_hint(render_commands, input->dt, control_hint_type, &stage->tutorial);
        }
    }

    //
    // NOTE: Score Notifications
    //
    {        
        f32 notif_speed = 20.0f * level_dt;
        f32 notif_lifetime = 2.0f;

        u32 actual_min_score_notification_number_size = min_score_notification_number_size;
        u32 actual_max_score_notification_number_size = max_score_notification_number_size;
        // NOTE: When we're in the shield tutorial we want to make the score notification
        // particularly big to highlight the fact that you can get a bigger score with shield
        if(stage->meta.tutorial_stage_type == tutorial_type_using_shield)
        {
            actual_max_score_notification_number_size *= 2;
        }
          
        for(Score_Notification **notif_ptr = &level->stage.first_visible_score_notif; *notif_ptr;)
        {
            Score_Notification *notif = *notif_ptr;
            float t = (stage_time - notif->start_time) / notif_lifetime;

            if(t > 1.0f)
            {
                *notif_ptr = notif->next;
                notif->next = level->stage.first_free_score_notif;
                level->stage.first_free_score_notif = notif;
            }
            else
            {
                Vec3 colour = safe_element_colour;
                f32 velocity = 1.0f;
                
                f32 size = min_score_notification_number_size;
                if(notif->amount == connector_connected_in_shield_score_increase)
                {
                    f32 curve_t = sinf(t*10.0f);
                    size = /*round_up*/(interpolate(actual_min_score_notification_number_size, actual_max_score_notification_number_size, clamp01(curve_t)));
                    
                    colour = interpolate(safe_element_colour, safe_element_colour*0.25f, clamp01(curve_t));
                    notif->position.x += curve_t * level_dt;
                    velocity *= 4.0f;
                }

                push_blocky_number(render_commands, notif->amount, size, notif->position.x, notif->position.y, vec4(colour, 1.0f - t));

                notif->position.y += velocity * notif_speed;

                notif_ptr = &notif->next;
            }
        }
    }

    //
    // NOTE: Life lost background flash
    //
    {
        if(stage->life_lost_flash_start > -1.0f)
        {
            f32 flash_time_last = 0.5f;
            f32 flash_elapsed = (gametime - stage->life_lost_flash_start);
            f32 flash_t = clamp01(flash_elapsed / flash_time_last);
            
            if(flash_t > 1.0f)
            {
                stage->life_lost_flash_start = -1.0f;
            }
            else
            {
                float alpha = 1.0f - flash_t;

                push_rect(render_commands, vec2(0), vec2(virtual_screen_width, virtual_screen_height), vec4(1.0f, 0.34f, 0.75f, alpha), draw_order_transform(entity_draw_order_life_lost_flash));
            }

        }
    }    
    
    //
    // NOTE: UI
    //

    f32 stage_time_left = (stage_meta->stage_length_time - stage->active_time);
    
    //
    // NOTE: Background
    //
#if 1
    {
        // NOTE: Background made up of two parts to represent the time left
        // and a blob that apperas towards end to signal end of stage
        
        f32 normalized_stage_progress = clamp01(1.0f - (stage_time_left / stage_meta->stage_length_time));
        f32 night_section_y = (virtual_screen_height*normalized_stage_progress);

        // NOTE: Draw the time "blob" that gets filled in as we get close to the end of the stage
        {
            // NOTE: Add a little buffer so we show full thing a second before stage actually ends
            f32 fade = clamp01((stage_time_left - 1.0f) / time_from_end_for_sun);
                
            Vec2 points[] =
              {
                  vec2(0.0f, 0.0f),
                  vec2(1.0f, 0.0f),
                  vec2(1.0f, 1.0f),
                  vec2(0.0f, 1.0f)
              };

            Render_Transform tint_transform = default_transform();

            s32 pieces_to_draw = (s32)( (1.0f - fade)*tint_transform.blocky_blend_pieces_total_count );
            s32 draw_order = entity_draw_order_background_timeline_blob;

            f32 alpha = clamp01(1.0f - fade);
                
            Vec4 inner_colour = vec4(safe_element_colour*0.5f, alpha);
            Vec4 outer_colour = vec4(safe_element_colour, 0.6f);
            if(stage->mode == level_mode_game_over && level->lives > 0)
            {
                draw_order = entity_draw_order_game_over_tint + 1;

                f32 t = normalize_range_neg11(sinf(level->game_over_time*1.5f));

                inner_colour = vec4(safe_element_colour*0.5f, alpha - 0.25f*(1.0f - t));
                outer_colour = vec4(inner_colour.rgb, 0.25f*t);                    
            }
                
            tint_transform = transform_flags(0, draw_order_transform(draw_order, blocky_blend_pieces_to_draw_transform(pieces_to_draw, blocky_blend_center_transform(vec2(0.7f, 0.7f), tint_transform))));

            Vec2 scale = vec2(100.0f);
            scale.x *= ((f32)virtual_screen_width / (f32)virtual_screen_height);
                
            Vec2 position = vec2(virtual_screen_width*0.835f - (scale.x/2.0f), (night_section_y - scale.y));

            push_shape_blocky_blend(render_commands, position, scale, inner_colour, outer_colour, points, array_count(points), tint_transform);
        }

        f32 colour_t = 0.0f;
        f32 blending_pieces_t = 0.0f;
        
        if(stage_time_left <= time_from_end_for_sun)
        {
            colour_t = clamp01(1.0f - (stage_time_left / time_from_end_for_sun));
            blending_pieces_t = colour_t;           
        }

        Vec4 bottom_colour = vec4(interpolate((vec3(87.0f, 134.0f, 255.0f) / 255.0f), safe_element_colour, colour_t), 1.0f);
        Vec4 top_colour = vec4(interpolate(vec3(255.0f, 87.0f, 87.0f) / 255.0f, safe_element_colour, colour_t), 1.0f);
        
        // NOTE: Trying to put blending pieces on top of normal background bits        
        {
            
            Temporary_Memory temp_mem = begin_temporary_memory(temp_arena);            
        
            u32 blending_pieces = (u32)interpolate(6, 30, colour_t);//6;
            // blending_pieces*2 + 1 left_quad + 1 right_quad
            u32 total_quads = blending_pieces + blending_pieces + 2;
            s32 total_verts = total_quads*6;
            Vertex_XY_RGBA *verts = push_array(temp_arena, Vertex_XY_RGBA, total_verts);
            Vertex_XY_RGBA *v = verts;

            f32 center_offset = 0.15f*((f32)blending_pieces / 6.0f);

            bottom_colour = vec4(interpolate((vec3(87.0f, 134.0f, 255.0f) / 255.0f), safe_element_colour, colour_t), 1.0f);
            top_colour = vec4(0.0f, 0.0f, 1.0f, 1.0f);

            Vec4 col_left_side = bottom_colour;
            Vec4 col_right_side = top_colour;

            Render_Transform bkg_render_transform = draw_order_transform(entity_draw_order_background);

            // NOTE: We use more blend pieces as get closer to end of stage
            bkg_render_transform = transform_flags(render_transform_flag_blend_exclude_first_colour | render_transform_flag_blend_exclude_second_colour, blocky_blend_pieces_to_draw_transform(blending_pieces, blocky_blend_pieces_total_count_transform(blending_pieces, bkg_render_transform)));
            
            f32 center_bottom = night_section_y/virtual_screen_height;// 0.5525f;
            f32 center_top = center_bottom + center_offset;//0.695f;
            // NOTE: Top Half
            {
                f32 center_bottom_y = virtual_screen_height*center_bottom;
                f32 center_top_y = virtual_screen_height*center_top;
                f32 top_min_y = center_top_y;
                f32 top_max_y = virtual_screen_height;
                
                // NOTE: Top
                {
                    *v++ = vertex_xy_rgba(vec2(0.0f, top_min_y), col_right_side);
                    *v++ = vertex_xy_rgba(vec2(virtual_screen_width, top_min_y), col_right_side);
                    *v++ = vertex_xy_rgba(vec2(0.0f, top_max_y), col_right_side);

                    *v++ = vertex_xy_rgba(vec2(0.0f, top_max_y), col_right_side);
                    *v++ = vertex_xy_rgba(vec2(virtual_screen_width, top_min_y), col_right_side);
                    *v++ = vertex_xy_rgba(vec2(virtual_screen_width, top_max_y), col_right_side);   
                }

                // NOTE: Center Blending
                {
                    v =
                      add_blocky_blend_verts(v, col_left_side, col_right_side, vec2(0.0f, center_bottom_y), vec2(0.0f, center_top_y), vec2(virtual_screen_width, center_bottom_y), vec2(virtual_screen_width, center_top_y), bkg_render_transform);
                }
            }
            
            // NOTE: Bottom Half
            {
                
                bottom_colour = vec4(interpolate(vec3(255.0f, 87.0f, 87.0f) / 255.0f, safe_element_colour, colour_t), 1.0f);
                top_colour = vec4(vec3(1.0f, 0.0f, 0.0f), 1.0f);

                Vec4 col_left_side = bottom_colour;
                Vec4 col_right_side = top_colour;
                
                f32 center_bottom = night_section_y/virtual_screen_height;// 0.5525f;
                f32 center_top = center_bottom - center_offset;//0.695f;
                
                f32 center_bottom_y = virtual_screen_height*center_bottom;
                f32 center_top_y = virtual_screen_height*center_top;

                f32 bottom_min_y = 0.0f;
                f32 bottom_max_y = center_top_y;
                
                // NOTE: Center Blending
                {
                    v =
                      add_blocky_blend_verts(v, col_left_side, col_right_side, vec2(0.0f, center_bottom_y), vec2(0.0f, center_top_y), vec2(virtual_screen_width, center_bottom_y), vec2(virtual_screen_width, center_top_y), bkg_render_transform); 
                }
                // NOTE: Bottom
                {
                    *v++ = vertex_xy_rgba(vec2(0.0f, bottom_min_y), col_right_side);
                    *v++ = vertex_xy_rgba(vec2(virtual_screen_width, bottom_min_y), col_right_side);
                    *v++ = vertex_xy_rgba(vec2(0.0f, bottom_max_y), col_right_side);

                    *v++ = vertex_xy_rgba(vec2(0.0f, bottom_max_y), col_right_side);
                    *v++ = vertex_xy_rgba(vec2(virtual_screen_width, bottom_min_y), col_right_side);
                    *v++ = vertex_xy_rgba(vec2(virtual_screen_width, bottom_max_y), col_right_side);   
                }
            }

            assert((v - verts) <= total_verts);

            push_vertices(render_commands, (f32 *)verts, (v - verts), vertex_flag_xy | vertex_flag_rgba, vec4(1.0f), bkg_render_transform);

            end_temporary_memory(temp_mem);
        }
        
        
#if 0
        Temporary_Memory temp_mem = begin_temporary_memory(temp_arena);
        char *time_left_text = DEBUG_format_string(temp_arena, "Time: %d", (s32)stage->time);
        DEBUG_push_text(render_commands, time_left_text, 36, 100, 100, vec4(1.0f));
        end_temporary_memory(temp_mem);
#endif
        
    }
#endif

    if(stage_meta->tutorial_type == tutorial_type_null)
    {
        // NOTE: Score
        {
            int text_y = virtual_screen_height - 24;
            draw_score_meter_corner(render_commands, level->score, level->score_to_get, level->last_best_score, stage_time);
        }
                        
        // NOTE: Lives Display
        {
            Render_Transform transform = draw_order_transform(entity_draw_order_lives_display);
            for(s32 life_index = 0; life_index < level->lives; ++life_index)
            {
                Vec2 position = get_life_display_position(life_index);
                Vec2 scale = vec2(player_life_display_size);
                Vec2 md = (position + scale/2.0f);
                
                // NOTE: Want it to shake faster as lives left gets lower
                f32 shake_energy = 1.0f + (f32)(player_level_start_lives - level->lives);

                f32 shake = 2.0f*shake_energy;
                md.x += sinf(stage->time*shake)*scale.x*0.2f;
                md.y += cosf(stage->time*shake)*scale.y*0.2f;
                
                f32 alpha_t = ((sinf(stage->time*3.0f*shake_energy) + 1.0f) / 2.0f);
                f32 center_alpha = 1.0f - (alpha_t * 0.15f);
                Vec4 center_c = vec4(safe_element_colour, center_alpha);
                Vec4 edge_c = vec4(danger_element_colour, 0.25f);

                push_bendy_quad(render_commands, position, scale, vec4(safe_element_colour, 0.0f), vec4(safe_element_colour, 1.0f), 0.25f, transform);
                
                Vec2 glow_scale = vec2(player_life_display_glow_size);
                Vec2 glow_position = position - (glow_scale - scale)/2.0f;
                {
                    f32 corner_offset = 0.2f + (0.15f * normalize_range_neg11(sinf(stage->time)));
                    
                    Bendy_Quad_Points bendy_quad = get_bendy_quad_points(corner_offset);
                    push_shape_blocky_blend(render_commands, glow_position, glow_scale, vec4(safe_element_colour, 1.0f), vec4(0.2f), bendy_quad.points, array_count(bendy_quad.points), transform_flags(render_transform_flag_additive_blend, transform));
                }
                
            }            
        }
    }

    //
    // NOTE: Player Rendering
    //
    {
        Vec2 player_position = player->position;
        Vec2 player_scale = player->scale;
        
        if(is_player_shaking(player, stage_time))
        {
            f32 offset = 1.5f;
            f32 speed = 26.0f;
            
            player_scale += (vec2(cosf(stage_time*speed), sinf(stage_time*speed)) * offset);
            player_position += (player->scale - player_scale)/2.0f;
        }

        b32 show_player_hexagon = false;
        if(is_player_protected_from_spinner_lasers(stage))
        {
            show_player_hexagon = true;
        }
        
        if(show_player_hexagon)
        {
            f32 scale_offset = 0.0f;
            if(player->touching_spinner_laser)
            {
                scale_offset = normalize_range_neg11(sinf(stage_time*40.0f)*0.2f);
            }
 
            Vec2 laser_shield_scale = player->scale * (2.0f/* + scale_offset*/);
            Vec2 laser_shield_position = player->position + player->scale/2.0f;// - ((glow_scale - player->scale) / 2.0f);

            draw_laser_shield(render_commands, laser_shield_position, laser_shield_scale, vec4(get_connect_colour(player->connect_colour), 0.5f), 2.0f + scale_offset*10.0f, !player->touching_spinner_laser, player->touching_spinner_laser);
        }

        // NOTE: Don't display player if outta lives
        if(level->lives > 0)
        {
            draw_player(render_commands, get_connect_colour(player->connect_colour), player_position, player_scale, stage, player->last_time_changed_colour);
        }       

        // NOTE: Cursor colour matches player
        if(!level_stage_frozen(stage))
        {
            cursor->colour = get_connect_colour(player->connect_colour);
        }
    }


    //
    // NOTE: Stage Transition
    //
    {        
        stage->stages_in_transition = false;

        b32 died = (level->lives == 0);
        b32 end_of_stage_transition = false;
        f32 fade = 0.0f;
        if(stage_time_left <= game_mode_transition_time || died)
        {
            // NOTE: Fade for end of stage, triggered by time running out or death
            stage->stages_in_transition = true;
            if(died)
            {
                fade = (level->game_over_time / game_mode_transition_time);
            }
            else
            {
                fade = (1.0f - (stage_time_left / game_mode_transition_time));                
            }

            end_of_stage_transition = true;
        }
        else if(stage_time_left >= (stage_meta->stage_length_time - game_mode_transition_time) && !in_first_level_stage(level))
        {
            // NOTE: Fade for start of level
            stage->stages_in_transition = true;
            fade = ((stage_time_left - (stage_meta->stage_length_time - game_mode_transition_time)) / game_mode_transition_time);                        
        }
        
        fade = clamp01(fade);
        
        if(stage->stages_in_transition)
        {
            // NOTE: Play transition sound, varies based on what has caused the transition
            {
                if(!stage->transition_sound_played)
                {
                    if(end_of_stage_transition)
                    {
                        f32 speed = 1.0f;
                        if(died)
                        {
                            speed = 0.5f;
                        }
                        else if(in_last_level_stage(level))
                        {
                            speed = 2.0f;
                        }

                        level->last_level_stage_transition_play_sound_result = play_sound(level->audio, asset_level_stage_transition, playing_sound_flag_game_level_stage_no_fade, speed);

                        stage->transition_sound_played = true;
                    }
                }
            }
            
            // NOTE: If stages finished we apply a semi-transparent tint over the level
            if((in_last_level_stage(level) && end_of_stage_transition) || died)
            {
                push_rect(render_commands, vec2(0), vec2(virtual_screen_width, virtual_screen_height), vec4(vec3(0.0f), fade*0.9f), draw_order_transform(entity_draw_order_game_over_tint));
            }
            else
            {
                // NOTE: Draw a blocky shape that incrementally covers the screen based on transition time
                Vec2 points[] =
                  {
                      vec2(0.0f, 0.0f),
                      vec2(1.0f, 0.0f),
                      vec2(1.0f, 1.0f),
                      vec2(0.0f, 1.0f)
                  };

                Render_Transform tint_transform = default_transform();
                
                s32 pieces_to_draw = tint_transform.blocky_blend_pieces_total_count - ((s32)round_up((1.0f - fade)*tint_transform.blocky_blend_pieces_total_count));
                
                tint_transform = transform_flags(render_transform_flag_reverse_blend_draw_order, draw_order_transform(entity_draw_order_transition_tint, blocky_blend_pieces_to_draw_transform(pieces_to_draw, blocky_blend_center_transform(vec2(0.9f, 1.0f)))));
                
            
                push_shape_blocky_blend(render_commands, vec2(0.0f), vec2(virtual_screen_width, virtual_screen_height), vec4(safe_element_colour*0.75f, fade), vec4( /*1.75f*safe_element_colour*/ interpolate(safe_element_colour, vec3(1.0f), 0.5f), fade), points, array_count(points), tint_transform);   
            }

        }
    }
    
    //
    // NOTE: Game Over
    //
    if(level_stage_frozen(stage))    
    {
        if(stage->mode == level_mode_game_over)
        {
            b32 show_quit_button = false;

            level->current_end_of_level_score_filling += (f32)level->score * (0.625f * input->dt);
            level->current_end_of_level_score_filling = clamp(level->current_end_of_level_score_filling, 0.0f, (f32)level->score);

            if(level->current_end_of_level_score_filling != level->last_end_of_level_score_filling)
            {
                level->last_end_of_level_score_filling = level->current_end_of_level_score_filling;

                // NOTE: Only want to play sound if score got higher, 
                // and a certain interval of time passed
                if((level->game_over_time - level->last_end_of_level_score_filling_sound_played_time) >= end_of_level_score_fill_sound_time_interval)
                {
                    level->last_end_of_level_score_filling_sound_played_time = level->game_over_time;
                    play_sound(level->audio, asset_end_of_level_score_increase);
                }
            }
            
            u32 score_fill = (u32)level->current_end_of_level_score_filling;
            if(score_fill < level->score)
            {
                show_quit_button = false;
            }
            else
            {
                show_quit_button = true;
            }
                
            level->game_over_time += input->dt;
            update_flashing_value(&level->game_over_goal_flash, input->dt);

            // NOTE: If game over because we ran out of lives constantly spawn life loss
            if(level->lives <= 0)
            {
                if((level->game_over_time - level->time_last_spawned_life_loss_on_game_over) >= 0.25f)
                {
                    spawn_life_loss(level, player->position, player->scale, get_connect_colour(player->connect_colour), vec2(max(virtual_screen_width, virtual_screen_height)));

                    level->time_last_spawned_life_loss_on_game_over = level->game_over_time;
                }        
            }

            f32 size = 100.0f;
            Vec2 meter_position = vec2(virtual_screen_width/2.0f - size/2.0f, virtual_screen_height/2.0f - size/2.0f);
            
            f32 goal_scaling = 0.0f;
            // NOTE: When we play goal achieved sound, want the goal icon to flash
            if(level->played_goal_achieved_sound)
            {
                f32 time_to_initial_full_scale = 0.25f;
                f32 time_to_stay_full_scale = 0.5f;
                f32 time_to_come_down_to_normal_scale = 6.0f;
                
                f32 elapsed = level->game_over_time - level->time_played_goal_achieved_sound;
                if(elapsed < time_to_initial_full_scale)
                {
                    // NOTE: Go Big
                    goal_scaling = elapsed / time_to_initial_full_scale;
                }
                else if(elapsed < time_to_stay_full_scale)
                {
                    // NOTE: Stay Big
                    goal_scaling = 1.0f;
                }
                else
                {
                    // NOTE: Go Small
                    goal_scaling = (1.0f - (((elapsed - time_to_stay_full_scale)) / time_to_come_down_to_normal_scale));
                }
                
                goal_scaling = clamp01(goal_scaling)*4.0f;
            }

            draw_score_meter_floating(render_commands, meter_position, size, score_fill, level->score_to_get, level->last_best_score, &level->game_over_goal_flash, goal_scaling, 1.0f, level->game_over_time);

            if(level->last_best_score < level->score_to_get)
            {
                if(!level->played_goal_achieved_sound && score_fill >= level->score_to_get)
                {
                    play_sound(level->audio, asset_level_score_beaten);
                    activate_flash(&level->game_over_goal_flash);
                
                    level->played_goal_achieved_sound = true;
                    level->time_played_goal_achieved_sound = level->game_over_time;   
                }                
            }
            else
            {
                activate_flash(&level->game_over_goal_flash);
            }

#if 0
            if(!level->played_goal_achieved_sound && level->last_best_score < level->score_to_get && score_fill >= level->score_to_get)
            {
                play_sound(level->audio, asset_level_score_beaten);
                activate_flash(&level->game_over_goal_flash);
                
                level->played_goal_achieved_sound = true;
                level->time_played_goal_achieved_sound = level->game_over_time;
            } 
#endif
            
            if(show_quit_button)
            {
                Vec2 quit_button_position = vec2(meter_position.x, meter_position.y - menu_button_size - menu_button_spacing);
                UI_Id quit_button_id = make_ui_id(&level->game_over_goal_flash);
                if(do_menu_button(menu_button_icon_quit_level, ui_context, quit_button_id, render_commands, quit_button_position, menu_button_scale))
                {
                    result.action = game_mode_result_action_switch_mode;                    
                }
            }
            
        }
        else
        {
            invalid_code_path;
        }    
    }
    else if(stage_time_left <= 0.0f)
    {
        if(change_level_stage(level, stage))
        {

        }
        else
        {
            set_level_game_over(level, progress, false, &result);
        }
    }    
    
    //
    // NOTE: Debug Controls
    //
#if TWOSOME_INTERNAL
    if(first_button_press(input->DEBUG_restart_stage_button))
    {
        shutdown_level_stage(level);
        if(level->lives == 0)
        {
            level->lives = 1;
        }
        init_level_stage(level);
    }
    else if(first_button_press(input->DEBUG_prev_stage_button))
    {
        if(!change_level_stage(level, stage, -1))
        {
            shutdown_level_stage(level);
            init_level_stage(level);
        }        
    }
    else if(first_button_press(input->DEBUG_next_stage_button))
    {
        change_level_stage(level, stage);
    }
#endif
    
    return result;

}
