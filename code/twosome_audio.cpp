#include "twosome_audio.h"


internal void initialize_captured_sounds_arena(Captured_Sounds_Arena *captured_sounds, Memory_Arena *perm_arena, Game_Audio *audio)
{
    zero_object(Captured_Sounds_Arena, *captured_sounds);

    captured_sounds->perm_arena = perm_arena;
    captured_sounds->audio = audio;
}

internal void init_game_audio(Game_Audio *audio, Memory_Arena *audio_perm_arena, Game_Assets *game_assets, f32 master_volume, RNG *rng)
{
    zero_object(Game_Audio, *audio);
    audio->perm_arena = audio_perm_arena;
    audio->game_assets = game_assets;
    audio->master_volume = clamp01(master_volume);
    audio->rng = rng;
    
    f32 *mix = audio->sound_mix;
    for(int sound_mix_index = 0; sound_mix_index < array_count(audio->sound_mix); ++sound_mix_index)
    {
        mix[sound_mix_index] = 1.0f;
    }

    mix[asset_connected_small] = 0.25f;
    mix[asset_connected_medium] = 0.25f;
    mix[asset_connected_high] = 0.2f;
    mix[asset_spikes_barrier_spawn] = 0.25f;
    mix[asset_spikes_barrier_safe_hit] = 0.75f;
    mix[asset_spikes_barrier_haze] = 0.25f;
    mix[asset_life_lost] = 0.75f;
    mix[asset_shield_collectable] = 0.075f;
    mix[asset_expanding_shield] = 0.5f;
    mix[asset_shield_deployed] = 0.5f;
    mix[asset_hub_firework_unexplode] = 0.25f;
    mix[asset_hub_firework_explodes_v1] = 0.25f;
    mix[asset_hub_firework_explodes_v2] = 0.25f;
    mix[asset_level_select_hover] = 0.5f;
    mix[asset_hub_ambience] = 0.2f;
    mix[asset_boy_about_to_speak] = 0.25f;
    mix[asset_boy_yawn] = 0.5f;
    mix[asset_boy_clears_throat] = 0.5f;
    mix[asset_boy_sighs] = 0.75f;
    mix[asset_level_score_beaten] = 0.75f;
    mix[asset_boys_song] = 0.75f;
    mix[asset_girl_saying_credits] = 0.15f;
}

internal f32 get_sound_mix_volume(Game_Audio *audio, int sound_id)
{
    f32 result = audio->sound_mix[sound_id];
    return result;
}

internal void start_cross_fade(Playing_Sound *s, b32 fade_out)
{
    f32 samples_to_fade_out_in;
    if(fade_out)
    {
        s->target_cross_fade_volume = 0.0f;
        samples_to_fade_out_in = ((f32)s->loaded_sound->total_samples - s->sample);
    }
    else
    {
        s->target_cross_fade_volume = 1.0f;
        samples_to_fade_out_in = ((f32)s->loaded_sound->total_samples - ((f32)s->loaded_sound->loop_point_samples));
    }

    f32 fade_duration_in_seconds = samples_to_fade_out_in / (f32)s->loaded_sound->samples_per_sec;

    f32 one_over_fade = 1.0f / fade_duration_in_seconds;
    s->delta_cross_fade_volume = one_over_fade * (s->target_cross_fade_volume - s->_cross_fade_volume);
}

internal void _set_volume(Playing_Sound *s, f32 volume, f32 fade_duration_in_seconds)
{
    if(fade_duration_in_seconds > 0.0f)
    {
        f32 one_over_fade = 1.0f / fade_duration_in_seconds;
        s->target_volume = volume;
        s->delta_current_volume = one_over_fade * (s->target_volume - s->volume);
    }
    else
    {
        s->target_volume = volume;
        s->volume = volume;
    }    
}

internal void set_volume(Playing_Sound *s, f32 volume)
{
    f32 fade_duration_in_seconds = 0.0f;
    if(s->playing)
    {
        // NOTE: If sound is playing we don't jump the volume because you can hear
        // a "crackle" so we've just arbitrarily picked a small value that changes
        // the volume quickly with no crackle (tried 1/60 but quick changes in volume
        // still gave crackle sooo... I don't know...
        fade_duration_in_seconds = 1.0f / 30.0f;
    }
    
    _set_volume(s, volume, fade_duration_in_seconds);
}

internal void set_play_speed(Playing_Sound *sound, f32 speed)
{
    assert(speed > 0.0f);
    sound->speed = speed;
}

internal void set_direction(Playing_Sound *s, s32 direction)
{
    assert(direction != 0);
    b32 change = false;
    
    if(direction > 0)
    {
        if(s->playing_flags & playing_sound_flag_reverse)
        {
            s->playing_flags &= ~playing_sound_flag_reverse;
            change = true;
        }
    }
    else if(direction < 0)
    {
        if(!(s->playing_flags & playing_sound_flag_reverse))
        {
            s->playing_flags |= playing_sound_flag_reverse;
            change = true;
        }        
    }

    if(change)
    {
        s->sample = (s->loaded_sound->total_samples - s->sample);
    }

    assert(s->sample >= 0 && s->sample < s->loaded_sound->total_samples);
    s->sample = clamp(s->sample, 0, s->loaded_sound->total_samples);
}

internal void reset_sound_sample_cursor(Playing_Sound *s, s32 silent_samples_offset = 0)
{
    // NOTE: Can have negative cursor to play some silent samples
    assert( in_range(silent_samples_offset, -(sound_start_silent_samples_pad - 1), 0) );
    silent_samples_offset = clamp(silent_samples_offset, -(sound_start_silent_samples_pad - 1), 0);
    
    s->sample = (f32)silent_samples_offset;

    if(s->playing_flags & playing_sound_flag_reverse)
    {
        // NOTE: When sound in reverse want starting pos to be 1, as reverse
        // mixing does total_samples - cursor to get index into sound data
        s->sample += 1;
    }
}

internal Playing_Sound *load_sound(Game_Audio *audio, uint32 sound_id, u32 playing_sound_flags = 0, int variation_index = -1)
{
    if(!audio->first_free_playing_sound)
    {
        audio->first_free_playing_sound = push_struct(audio->perm_arena, Playing_Sound);
        zero_object(Playing_Sound, *audio->first_free_playing_sound);
    }

    Playing_Sound *sound = audio->first_free_playing_sound;
    audio->first_free_playing_sound = audio->first_free_playing_sound->next;
    
    zero_object(Playing_Sound, *sound);
        
    sound->next = audio->first_playing_sound;
    audio->first_playing_sound = sound;

    sound->id = sound_id;
    _set_volume(sound, 1.0f, 0.0f);
    sound->_cross_fade_volume = 1.0f;
    set_play_speed(sound, 1.0f);
    
    sound->playing_flags = playing_sound_flags;    
    sound->_owned_by_audio_system = false;

    Asset *found_asset = load_sound(audio->game_assets, audio, sound->id, variation_index);

    sound->loaded_sound = &found_asset->sound;
    assert(sound->loaded_sound && sound->loaded_sound->data && sound->loaded_sound->total_samples);
    sound->_mix_volume = get_sound_mix_volume(audio, sound->id);
    
    reset_sound_sample_cursor(sound);
    
    return sound;
}

internal Overlapping_Sound *load_overlapping_sound(Game_Audio *audio, u32 sound_id, u32 playing_sound_flags = 0, b32 cross_fade = false, int variation_index = -1)
{
    // NOTE: The looping flag doesn't make much sense with overlapping sound
    // as it's a continuous thing of the same sound playing one after another
    assert(!(playing_sound_flags & playing_sound_flag_loop));
    
    if(!audio->first_free_overlapping_sound)
    {
        audio->first_free_overlapping_sound = push_struct(audio->perm_arena, Overlapping_Sound);
    }

    Overlapping_Sound *os = audio->first_free_overlapping_sound;
    zero_object(Overlapping_Sound, *os);
    audio->first_free_overlapping_sound = os->next;

    os->next = audio->first_playing_overlapping_sound;
    audio->first_playing_overlapping_sound = os;

    // IMPORTANT: The secondary sound needs to come after the primary sound in list, because
    // output sound relies (and asserts) on this so it can next sound on right sample boundary
    os->secondary = load_sound(audio, sound_id, playing_sound_flags, variation_index);
    os->primary = load_sound(audio, sound_id, playing_sound_flags, variation_index);
    assert(os->primary->next == os->secondary);
    
    os->cross_fade = cross_fade;

    if(os->cross_fade)
    {        
        os->primary->_cross_fade_volume = 1.0f;
        os->secondary->_cross_fade_volume = 0.0f;
    }
    
    return os;
}

internal Play_Sound_Result play_sound(Playing_Sound *sound, f32 speed = 1.0f, f32 volume = 1.0f)
{
    // IMPORTANT: We set the volume before changing playing flag to true, because the set_volume
    // uses it to check whether it can change the output_volume directly
    set_volume(sound, volume);

    assert(speed > 0.0f);
    set_play_speed(sound, speed);

    reset_sound_sample_cursor(sound);
    
    sound->playing = true;
    sound->paused = false;

    Play_Sound_Result result = {};
    result.id = sound->id;
    result.backwards = (sound->playing_flags & playing_sound_flag_reverse);
    result.variation = sound->loaded_sound->variation;
    result.volume = sound->volume;

    return result;
}

internal Play_Sound_Result play_sound_using_attributes_of_another(Playing_Sound *sound, Playing_Sound *master_sound)
{
    Play_Sound_Result result = play_sound(sound, master_sound->speed, master_sound->volume);

    // NOTE: Inherit any fading that's happening
    sound->target_volume = master_sound->target_volume;
    sound->delta_current_volume = master_sound->delta_current_volume;

    return result;
}

internal Play_Sound_Result play_sound(Overlapping_Sound *os, f32 speed = 1, f32 volume = 1.0f)
{
    Play_Sound_Result result = play_sound(os->primary, speed, volume);
    return result;
}

internal b32 is_sound_playing(Overlapping_Sound *os)
{
    b32 result = (os->primary->playing || os->secondary->playing);
    return result;
}

internal void resume_sound(Playing_Sound *sound)
{
    sound->playing = true;
    sound->paused = false;
}

internal void resume_sound(Overlapping_Sound *os)
{
    if(os->primary->paused)
    {
        resume_sound(os->primary);
    }
    if(os->secondary->paused)
    {
        resume_sound(os->secondary);
    }
}

internal void pause_sound(Playing_Sound *sound)
{
    sound->playing = false;
    sound->paused = true;
}

internal void pause_sound(Overlapping_Sound *os)
{
    if(os->primary->playing)
    {
        pause_sound(os->primary);
    }
    if(os->secondary->playing)
    {
        pause_sound(os->secondary);
    }
}

internal void stop_sound(Playing_Sound *s)
{
    s->playing = false;
    s->paused = false;    
}

internal void stop_sound(Overlapping_Sound *os)
{
    stop_sound(os->primary);
    stop_sound(os->secondary);
}

internal b32 sound_stopped(Playing_Sound *sound)
{
    b32 result = (!sound->playing && !sound->paused);
    return result;
}

internal Play_Sound_Result play_sound(Game_Audio *audio, uint32 sound_id, int playing_sound_flags = 0, f32 speed = 1, int variation_index = -1, f32 volume = 1.0f)
{
    Playing_Sound *sound = load_sound(audio, sound_id, playing_sound_flags, variation_index);
    sound->_owned_by_audio_system = true;
    Play_Sound_Result result = play_sound(sound, speed, volume);
    _set_volume(sound, volume, 0.0f);
    
    return result;
}

internal int play_overlapping_sound(Game_Audio *audio, u32 sound_id, int playing_sound_flags = 0, b32 cross_fade = false, f32 speed = 1, int variation_index = -1)
{
    Overlapping_Sound *os = load_overlapping_sound(audio, sound_id, playing_sound_flags, cross_fade, variation_index);
    os->primary->_owned_by_audio_system = true;
    os->secondary->_owned_by_audio_system = true;

    os->cross_fade = cross_fade;
    
    play_sound(os, speed);

    return os->primary->loaded_sound->variation;
}

internal void release_sound(Game_Audio *audio, Playing_Sound *sound)
{
    b32 found_sound = false;
    for(Playing_Sound **playing_sound_ptr = &audio->first_playing_sound; *playing_sound_ptr; )
    {
        Playing_Sound *s = *playing_sound_ptr;

        if(s == sound)
        {
            *playing_sound_ptr = s->next;
            s->next = audio->first_free_playing_sound;
            audio->first_free_playing_sound = s;

            found_sound = true;
            break;
        }
        else
        {
            playing_sound_ptr = &s->next;
        }
    }

    assert(found_sound);
}

internal void restart_sound(Playing_Sound *sound)
{
    stop_sound(sound);
    play_sound(sound);
}

internal void fade_out_sound(Playing_Sound *s, f32 fade_duration_in_seconds)
{
    _set_volume(s, 0.0f, fade_duration_in_seconds);
}

internal void fade_in_sound(Playing_Sound *s, f32 fade_duration_in_seconds)
{
    _set_volume(s, 1.0f, fade_duration_in_seconds);
}

internal void fade_out_sound(Overlapping_Sound *os, f32 fade_duration_in_seconds)
{
    _set_volume(os->primary, 0.0f, fade_duration_in_seconds);
    _set_volume(os->secondary, 0.0f, fade_duration_in_seconds);
}

internal void fade_in_sound(Overlapping_Sound *os, f32 fade_duration_in_seconds)
{
    _set_volume(os->primary, 1.0f, fade_duration_in_seconds);
    _set_volume(os->secondary, 1.0f, fade_duration_in_seconds);
}

internal void give_sound_to_audio_system(Playing_Sound *sound, f32 fade_duration_in_seconds = 0.0f)
{
    sound->playing_flags &= ~playing_sound_flag_loop;
    sound->_owned_by_audio_system = true;
    if(fade_duration_in_seconds > 0.0f)
    {
        fade_out_sound(sound, fade_duration_in_seconds);
    }
}

internal Captured_Sound *add_captured_sound(Captured_Sounds_Arena *captured_arena)
{
    if(!captured_arena->first_free_captured_sound)
    {
        captured_arena->first_free_captured_sound = push_struct(captured_arena->perm_arena, Captured_Sound);
    }

    Captured_Sound *cs = captured_arena->first_free_captured_sound;
    zero_object(Captured_Sound, *cs);
    captured_arena->first_free_captured_sound = cs->next;

    cs->next = captured_arena->first_captured_sound;
    captured_arena->first_captured_sound = cs;

    return cs;
}

internal Playing_Sound *add_captured_sound(Captured_Sounds_Arena *captured_arena, Playing_Sound *s)
{
    Captured_Sound *capture = add_captured_sound(captured_arena);
    capture->sound = s;
    capture->type = captured_sound_type_sound;

    return s;
}

internal Overlapping_Sound *add_captured_sound(Captured_Sounds_Arena *captured_arena, Overlapping_Sound *os)
{
    Captured_Sound *capture = add_captured_sound(captured_arena);
    capture->overlapping_sound = os;
    capture->type = captured_sound_type_overlapping;

    return os;
}

internal void release_captured_sounds(Captured_Sounds_Arena *captured_arena)
{
    free_link_list(Captured_Sound, captured_arena->first_captured_sound, captured_arena->first_free_captured_sound);
}

internal void relinquish_sounds_to_audio_system(Captured_Sounds_Arena *captured_arena)
{
    Game_Audio *audio = captured_arena->audio;
    for(Captured_Sound *s = captured_arena->first_captured_sound; s; s = s->next)
    {
        if(s->type == captured_sound_type_sound)
        {
            give_sound_to_audio_system(s->sound);
        }
        else if(s->type == captured_sound_type_overlapping)
        {
            give_sound_to_audio_system(s->overlapping_sound->primary);
            give_sound_to_audio_system(s->overlapping_sound->secondary);

            b32 found_overlapping_sound_object = false;
            for(Overlapping_Sound **os_ptr = &audio->first_playing_overlapping_sound; *os_ptr; os_ptr = &(*os_ptr)->next)
            {
                Overlapping_Sound *os = *os_ptr;
                if(os == s->overlapping_sound)
                {
                    *os_ptr = os->next;

                    os->next = audio->first_free_overlapping_sound;
                    audio->first_free_overlapping_sound = os;
                    found_overlapping_sound_object = true;
                    break;
                }
            }
            assert(found_overlapping_sound_object);
            
        }
        else
        {
            invalid_code_path;
        }
    }
    
    release_captured_sounds(captured_arena);
}

internal void fade_out_all_sounds_owned_by_audio_system(Game_Audio *audio, f32 fade_duration_in_seconds, u32 exclude_sounds_with_flags = 0)
{
    for(Playing_Sound *s = audio->first_playing_sound; s; s = s->next)
    {
        if(s->_owned_by_audio_system && !(s->playing_flags & playing_sound_flag_no_fade) && !(s->playing_flags & exclude_sounds_with_flags))
        {
            fade_out_sound(s, fade_duration_in_seconds);
        }
    }
}

internal void pause_and_capture_currently_playing_sounds(Game_Audio *audio)
{
    initialize_captured_sounds_arena(&audio->pause_captured_sounds, audio->perm_arena, audio);
    
    // NOTE: Pause all currently playing sounds, and put them in a list so they can be resumed when we un-pause game.
    for(Playing_Sound *s = audio->first_playing_sound; s; s = s->next)
    {
        if(s->playing && !(s->playing_flags & playing_sound_flag_no_pause_capture))
        {
            pause_sound(s);
            add_captured_sound(&audio->pause_captured_sounds, s);
        }
    }
}

internal void resume_pause_game_captured_sounds(Game_Audio *audio)
{    
    for(Captured_Sound *cs = audio->pause_captured_sounds.first_captured_sound; cs; cs = cs->next)
    {
        resume_sound(cs->sound);
    }

    release_captured_sounds(&audio->pause_captured_sounds);
}

internal f32 get_sound_length(Playing_Sound *s)
{
    f32 length_in_seconds = (f32)s->loaded_sound->total_samples / (f32)s->loaded_sound->samples_per_sec;
    return length_in_seconds;
}

internal f32 get_sound_position(Playing_Sound *s)
{
    f32 position_in_seconds = s->sample  / (f32)s->loaded_sound->samples_per_sec;
    return position_in_seconds;
}

#if TWOSOME_INTERNAL
internal f32 DEBUG_set_sound_position(Playing_Sound *s, f32 position_in_seconds)
{
    // NOTE: Not currently supporting reverse sounds - would have to make sure offsets are correct
    assert(s->playing_flags & playing_sound_flag_reverse);
    
    assert(position_in_seconds < get_sound_length(s));
    s->sample = position_in_seconds * (f32)s->loaded_sound->samples_per_sec;    
}

internal void DEBUG_draw_sound_debug_info(Game_Render_Commands *render_commands, Playing_Sound *s, int x, int y)
{
    char buffer[256];
    sprintf(buffer, "Volume: %f", s->volume);
    DEBUG_push_text(render_commands, buffer, 24, x, y, vec4(1.0f));

    y -= 24;    
    sprintf(buffer, "Speed: %f", s->speed);
    DEBUG_push_text(render_commands, buffer, 24, x, y, vec4(1.0f));

    y -= 24;
    sprintf(buffer, "Position: %f", get_sound_position(s));
    DEBUG_push_text(render_commands, buffer, 24, x, y, vec4(1.0f));    
}

internal void DEBUG_draw_overlapping_sound_debug_info(Game_Render_Commands *render_commands, Overlapping_Sound *os)
{
    DEBUG_push_text(render_commands, "Cross fade:", 24, 100, 130, vec4(1.0f));
    
    char buffer[256];
    sprintf(buffer, "Primary: %f", os->primary->_cross_fade_volume);
    DEBUG_push_text(render_commands, buffer, 24, 100, 100, vec4(1.0f));

    sprintf(buffer, "Secondary: %f", os->secondary->_cross_fade_volume);
    DEBUG_push_text(render_commands, buffer, 24, 100, 70, vec4(1.0f));


    DEBUG_push_text(render_commands, "Volume:", 24, 300, 130, vec4(1.0f));
    sprintf(buffer, "Primary: %f", os->primary->volume);
    DEBUG_push_text(render_commands, buffer, 24, 300, 100, vec4(1.0f));

    sprintf(buffer, "Secondary: %f", os->secondary->volume);
    DEBUG_push_text(render_commands, buffer, 24, 300, 70, vec4(1.0f));
}

#endif

internal void update_game_audio(Game_Audio *audio, f32 dt)
{
    // NOTE: Used to do stuff in here, but moved it to mixer, so now
    // this function is empty and dead inside...
}

internal void output_playing_sounds(Game_Audio *audio, Game_Sound_Output_Buffer *sound_buffer, Memory_Arena *temp_arena)
{
    TIMED_BLOCK();

    Temporary_Memory temp_mem = begin_temporary_memory(temp_arena);

    // NOTE: Padding of 4 silent samples so SIMD is happy
    assert(sound_start_silent_samples_pad == 4);
    assert(sound_end_silent_samples_pad == 4);
    assert(sound_total_silent_samples_pad == 8);
    
    assert((sound_buffer->sample_count % 4) == 0);
    
    //
    // NOTE: Get buffers for our sounds to write their samples to and clear to zero
    //
    u32 chunk_count = sound_buffer->sample_count / 4;
    
    __m128 *left_channel_mix = push_array(temp_arena, __m128, chunk_count, align_no_clear(16));
    __m128 *right_channel_mix = push_array(temp_arena, __m128, chunk_count, align_no_clear(16));

    __m128 zero = _mm_set1_ps(0.0f);
    __m128 one = _mm_set1_ps(1.0f);
    {
        __m128 *left_channel_dest = left_channel_mix;
        __m128 *right_channel_dest = right_channel_mix;
        
        for(u32 chunk_index = 0; chunk_index < chunk_count; ++chunk_index)
        {
            _mm_store_ps((float *)left_channel_dest++, zero);
            _mm_store_ps((float *)right_channel_dest++, zero);
        }
    }
    
    f32 seconds_per_sample = 1.0f / (f32)sound_buffer->samples_per_second;
    
    //
    // NOTE: Get samples from currently playing sounds
    //
    for(Playing_Sound **playing_sound_ptr = &audio->first_playing_sound; *playing_sound_ptr; )
    {
        TIMED_BLOCK();
        
        // IMPORTANT: The Overlapping Sounds update can swap the pointers in the list so we don't assign
        // a local playing_sound pointer until after overlapping update bit
        if((*playing_sound_ptr)->playing) 
        {
            s32 sample_out_start_index = 0;
            s32 total_chunks_to_mix = chunk_count;

            __m128 *left_channel_dest = left_channel_mix;
            __m128 *right_channel_dest = right_channel_mix;
            
            //
            // NOTE: Overlapping Sounds
            //
            {
                if((*playing_sound_ptr)->loaded_sound->loop_point_samples > 0)
                {
                    for(Overlapping_Sound *os = audio->first_playing_overlapping_sound; os; os = os->next)
                    {
                        if(os->primary == *playing_sound_ptr)
                        {
                            f32 target_sample_cursor = ( os->primary->sample + (sound_buffer->sample_count * os->primary->speed) );
                            f32 loop_point_samples = (f32)os->primary->loaded_sound->loop_point_samples;
                    
                            if(target_sample_cursor > loop_point_samples)
                            {
                                s32 sample_overlap_point = (s32)((loop_point_samples - os->primary->sample) / os->primary->speed);
                                assert(sample_overlap_point < target_sample_cursor);

                                s32 chunks_overlap_point = (sample_overlap_point / 4);
                                assert(chunks_overlap_point < total_chunks_to_mix);
                                chunks_overlap_point = clamp(chunks_overlap_point, 0, total_chunks_to_mix);
                                total_chunks_to_mix -= chunks_overlap_point;
                                left_channel_dest += chunks_overlap_point;
                                right_channel_dest += chunks_overlap_point;

                                s32 leftover_samples = (sample_overlap_point % 4);

                                // NOTE: Swap primary and secondary
                                Playing_Sound *original_primary = os->primary;
                                Playing_Sound *original_secondary = os->secondary;
                                os->secondary = original_primary;
                                os->primary = original_secondary;

                                assert(!os->primary->playing);
                                // NOTE: Want to use the attributes of secondary sound here, because that *was* the
                                // primary sound, and we know that the primary sound was started first, and is
                                // setup correctly.
                                play_sound_using_attributes_of_another(os->primary, os->secondary);

                                // NOTE: Not currently supporting reversing overlapping sounds
                                assert(!(os->primary->playing_flags & playing_sound_flag_reverse));
                                assert(leftover_samples < 4);
                                os->primary->sample = (f32)(leftover_samples * -1);

                                // NOTE: If we're cross fading, then fade one out whilst fading other one in
                                if(os->cross_fade)
                                {
                                    start_cross_fade(os->secondary, true);
                                    start_cross_fade(os->primary, false);                                    
                                }

                                // NOTE: Swap pointers in list, so that primary sound is always before secondary sound
                                // in the list (if primary sound was after, then wouldn't have chance to process
                                // secondary secondary mix as it's already been iterated over)
                                assert( (*playing_sound_ptr) == original_primary && (*playing_sound_ptr)->next == original_secondary );
                                original_primary->next = original_secondary->next;
                                original_secondary->next = original_primary;
                                // NOTE: Sets last playing sound's next pointer to secondary sound
                                *playing_sound_ptr = original_secondary;                                
                                
                                assert((*playing_sound_ptr) == original_secondary && (*playing_sound_ptr)->next == original_primary);
                            }

                            break;
                        }
                    }
                }
            }            
            
            Playing_Sound *s = *playing_sound_ptr;
            Loaded_Sound *loaded_sound = s->loaded_sound;
            assert(loaded_sound->data && loaded_sound->total_samples);

            // NOTE: Make sure sound has 4 padding samples at either end for our SIMD and for speed modulating offset
            assert( (((
                       loaded_sound->data_size - (loaded_sound->total_samples * loaded_sound->channels * sizeof(s16))
                       ) / loaded_sound->channels) / sizeof(s16)) == sound_total_silent_samples_pad );

            s32 sample_direction;
            if(s->playing_flags & playing_sound_flag_reverse)
            {
                sample_direction = -1;
            }
            else
            {
                sample_direction = 1;
            }
            __m128i sample_direction_x4 = _mm_set1_epi32(sample_direction);            

            __m128 master_and_mix_volume = _mm_mul_ps(_mm_set1_ps(audio->master_volume), _mm_set1_ps(s->_mix_volume));
            
            while(total_chunks_to_mix && s->playing)
            {
                //
                // NOTE: Work how many samples we can satisfy with number of samples the sound's got left
                //
                s32 chunks_to_mix = total_chunks_to_mix;
                b32 reset_sample_cursor_after_mixing = false;
                s32 reset_position_offset = 0;
                //
                // NOTE: Work out how many chunks we can fulfil with samples left in sound
                //
                {                    
                    assert(s->sample >= -3 && s->sample <= s->loaded_sound->total_samples);
                    s->sample = clamp(s->sample, -3, s->loaded_sound->total_samples);
                    
                    s32 total_samples_aligned = align4(loaded_sound->total_samples);
                    s32 samples_left_in_sound_aligned = (s32)((total_samples_aligned - s->sample) / s->speed);

                    s32 chunks_left_in_sound = ( samples_left_in_sound_aligned / 4 );
                    if(chunks_to_mix >= chunks_left_in_sound)
                    {
                        chunks_to_mix = chunks_left_in_sound;

                        if(s->playing_flags & playing_sound_flag_loop)
                        {
                            reset_sample_cursor_after_mixing = true;
                                
                            s32 left_over_samples = 0;
                            // NOTE: If couldn't do all chunks, then have to offset cursor so that
                            // it accomodates for the sampes at the end that didn't fit in a block of 4
                            // by giving it a negative value which puts silent padded samples on the already
                            // mixed samples, and the leftover samples in the remaining block of 4
                            if(chunks_to_mix > chunks_left_in_sound)
                            {
                                s32 samples_left_in_sound_unaligned = (s32)( ((loaded_sound->total_samples - s->sample) / s->speed));
                                left_over_samples = (samples_left_in_sound_aligned - samples_left_in_sound_unaligned);
                                assert(left_over_samples < 4);
                                left_over_samples = clamp(left_over_samples, 0, 3);
                            }

                            reset_position_offset = -left_over_samples;   
                        }
                        else
                        {
                            // NOTE: stop_sound
                            s->playing = false;
                            s->paused = false;
                        }
                    }                          
                }
                
                __m128 d_volume_x4 = zero;
                __m128 volume_x4 = _mm_set1_ps(s->volume);
                b32 volume_finished = false;
                s32 volume_chunk_count = chunks_to_mix;
                {
                    f32 d_volume = seconds_per_sample * s->delta_current_volume;
                    f32 d_volume_chunk = 4.0f * d_volume;
                    if(d_volume_chunk != 0.0f)
                    {
                        f32 delta_volume = (s->target_volume - s->volume);
                        volume_chunk_count = (u32)(round(delta_volume / d_volume_chunk));

                        d_volume_x4 = _mm_set1_ps(d_volume_chunk);
                        volume_x4 = _mm_setr_ps(s->volume + d_volume*0,
                                                s->volume + d_volume*1,
                                                s->volume + d_volume*2,
                                                s->volume + d_volume*3);
                    }
                }

                __m128 d_cross_fade_volume_x4 = zero;
                __m128 cross_fade_volume_x4 = _mm_set1_ps(s->_cross_fade_volume);
                b32 cross_fade_finished = false;
                s32 cross_fade_chunk_count = chunks_to_mix;
                {
                    f32 d_cross_fade = (seconds_per_sample * s->delta_cross_fade_volume) * s->speed;
                    f32 d_cross_fade_chunk = 4.0f * d_cross_fade;
                    if(d_cross_fade_chunk != 0.0f)
                    {
                        f32 delta_cross_fade = (s->target_cross_fade_volume - s->_cross_fade_volume);
                        cross_fade_chunk_count = (u32)(round(delta_cross_fade / d_cross_fade_chunk));

                        d_cross_fade_volume_x4 = _mm_set1_ps(d_cross_fade_chunk);
                        cross_fade_volume_x4 = _mm_setr_ps(s->_cross_fade_volume + d_cross_fade*0,
                                                           s->_cross_fade_volume + d_cross_fade*1,
                                                           s->_cross_fade_volume + d_cross_fade*2,
                                                           s->_cross_fade_volume + d_cross_fade*3);
                    }
                }

                // NOTE: We're going to go with the one that can get finished in the least
                // chunks, then the other one can finish using the rest of the chunks_to_mix
                if(volume_chunk_count < cross_fade_chunk_count)
                {
                    if(chunks_to_mix > volume_chunk_count)
                    {
                        chunks_to_mix = volume_chunk_count;
                        volume_finished = true;
                    }
                }
                else if(cross_fade_chunk_count < volume_chunk_count)
                {
                    if(chunks_to_mix > cross_fade_chunk_count)
                    {
                        chunks_to_mix = cross_fade_chunk_count;
                        cross_fade_finished = true;
                    }
                }
                else if(volume_chunk_count == cross_fade_chunk_count)
                {
                    if(chunks_to_mix > volume_chunk_count)
                    {
                        chunks_to_mix = volume_chunk_count;
                        volume_finished = true;
                        cross_fade_finished = true;

                        assert(volume_finished && cross_fade_finished);
                    }                    
                }
                                
                s32 channel_count = loaded_sound->channels;
                __m128i channel_count_minus_one = _mm_set1_epi32(channel_count - 1);
                assert(loaded_sound->channels > 1);
                
                f32 d_sample = sample_direction * s->speed;
                __m128 sample_chunk_offsets = _mm_setr_ps(0.0f*d_sample,
                                                          1.0f*d_sample,
                                                          2.0f*d_sample,
                                                          3.0f*d_sample);
                
                //
                // NOTE: Mix sounds samples
                //                
                f32 begin_sample_position = s->sample;
                f32 forward_end_sample_position = begin_sample_position + chunks_to_mix*(4.0f*s->speed);
                f32 end_sample_position;
                if(s->playing_flags & playing_sound_flag_reverse)
                {
                    begin_sample_position = (f32)s->loaded_sound->total_samples - s->sample;
                    end_sample_position = begin_sample_position - chunks_to_mix*(4.0f*s->speed);
                }
                else
                {
                    end_sample_position = forward_end_sample_position;
                }

#if TWOSOME_SLOW
                s32 assert_data_min_index = ( -1 * (sound_start_silent_samples_pad*channel_count) );
                s32 assert_data_max_index = ( ((loaded_sound->total_samples + sound_start_silent_samples_pad) * channel_count) - 1 );
#endif
                
                f32 loop_index_c = (end_sample_position - begin_sample_position) / (f32)chunks_to_mix;
                for(s32 chunk_index = 0; chunk_index < chunks_to_mix; ++chunk_index)
                {
                    TIMED_BLOCK();
                 
                    __m128 sample_position = _mm_set1_ps(begin_sample_position + (loop_index_c*chunk_index));
                    __m128 sample_cursor = _mm_add_ps(sample_position, sample_chunk_offsets);
                    
                    assert( ((f32 *)&sample_cursor)[3] >= -3.0f && ((f32 *)&sample_cursor)[3] < (f32)(s->loaded_sound->total_samples + 3) );

                    __m128i sample_cursor_s32 = _mm_cvttps_epi32(sample_cursor);
                    
                    __m128 fractional_part = _mm_sub_ps(sample_cursor, _mm_cvtepi32_ps(sample_cursor_s32));
                    if(s->playing_flags & playing_sound_flag_reverse)
                    {
                        fractional_part = _mm_sub_ps(one, fractional_part);
                    }
                    
                    assert(  (in_range01(((f32 *)&fractional_part)[0]) || ((f32 *)&sample_cursor)[0] < 0) &&
                             (in_range01(((f32 *)&fractional_part)[1]) || ((f32 *)&sample_cursor)[1] < 0) &&
                             (in_range01(((f32 *)&fractional_part)[2]) || ((f32 *)&sample_cursor)[2] < 0) &&
                             (in_range01(((f32 *)&fractional_part)[3]) || ((f32 *)&sample_cursor)[3] < 0)   );
                    
                    __m128i second_sample_cursor = _mm_add_epi32(sample_cursor_s32, sample_direction_x4);
                    assert(  ((s32 *)&second_sample_cursor)[3] < (s32)(loaded_sound->total_samples + 4)
                             || (s->playing_flags & playing_sound_flag_reverse && ((s32 *)&second_sample_cursor)[3] >= -4) );

                    __m128i s1_ch1_indices = _mm_setr_epi32(((s32 *)&sample_cursor_s32)[0]*channel_count,
                                                            ((s32 *)&sample_cursor_s32)[1]*channel_count,
                                                            ((s32 *)&sample_cursor_s32)[2]*channel_count,
                                                            ((s32 *)&sample_cursor_s32)[3]*channel_count);

                    __m128i s2_ch1_indices = _mm_setr_epi32(((s32 *)&second_sample_cursor)[0]*channel_count,
                                                            ((s32 *)&second_sample_cursor)[1]*channel_count,
                                                            ((s32 *)&second_sample_cursor)[2]*channel_count,
                                                            ((s32 *)&second_sample_cursor)[3]*channel_count);

                    __m128i s1_ch2_indices = _mm_add_epi32(s1_ch1_indices, channel_count_minus_one);
                    __m128i s2_ch2_indices = _mm_add_epi32(s2_ch1_indices, channel_count_minus_one);

                    assert( in_range( ((s32 *)&s1_ch1_indices)[0], assert_data_min_index, assert_data_max_index )
                            && in_range( ((s32 *)&s1_ch1_indices)[1], assert_data_min_index, assert_data_max_index )
                            && in_range( ((s32 *)&s1_ch1_indices)[2], assert_data_min_index, assert_data_max_index )
                            && in_range( ((s32 *)&s1_ch1_indices)[3], assert_data_min_index, assert_data_max_index ) );

                    assert( in_range( ((s32 *)&s2_ch1_indices)[0], assert_data_min_index, assert_data_max_index )
                            && in_range( ((s32 *)&s2_ch1_indices)[1], assert_data_min_index, assert_data_max_index )
                            && in_range( ((s32 *)&s2_ch1_indices)[2], assert_data_min_index, assert_data_max_index )
                            && in_range( ((s32 *)&s2_ch1_indices)[3], assert_data_min_index, assert_data_max_index ) );

                    assert( in_range( ((s32 *)&s1_ch2_indices)[0], assert_data_min_index, assert_data_max_index )
                            && in_range( ((s32 *)&s1_ch2_indices)[1], assert_data_min_index, assert_data_max_index )
                            && in_range( ((s32 *)&s1_ch2_indices)[2], assert_data_min_index, assert_data_max_index )
                            && in_range( ((s32 *)&s1_ch2_indices)[3], assert_data_min_index, assert_data_max_index ) );

                    assert( in_range( ((s32 *)&s2_ch2_indices)[0], assert_data_min_index, assert_data_max_index )
                            && in_range( ((s32 *)&s2_ch2_indices)[1], assert_data_min_index, assert_data_max_index )
                            && in_range( ((s32 *)&s2_ch2_indices)[2], assert_data_min_index, assert_data_max_index )
                            && in_range( ((s32 *)&s2_ch2_indices)[3], assert_data_min_index, assert_data_max_index ) );

                    
                    // NOTE: Left Channel
                    __m128 left_channel_sample;                    
                    {
                        __m128 s1 = _mm_setr_ps(loaded_sound->data[ ((s32 *)&s1_ch1_indices)[0] ],
                                                loaded_sound->data[ ((s32 *)&s1_ch1_indices)[1] ],
                                                loaded_sound->data[ ((s32 *)&s1_ch1_indices)[2] ],
                                                loaded_sound->data[ ((s32 *)&s1_ch1_indices)[3] ] );

                        __m128 s2 = _mm_setr_ps(loaded_sound->data[ ((s32 *)&s2_ch1_indices)[0] ],
                                                loaded_sound->data[ ((s32 *)&s2_ch1_indices)[1] ],
                                                loaded_sound->data[ ((s32 *)&s2_ch1_indices)[2] ],
                                                loaded_sound->data[ ((s32 *)&s2_ch1_indices)[3] ] );

                        __m128 a = _mm_mul_ps(s1, _mm_sub_ps(one, fractional_part));
                        __m128 b = _mm_mul_ps(s2, fractional_part);
                        left_channel_sample = _mm_add_ps(a, b);

                    }
                    // NOTE: Right Channel
                    __m128 right_channel_sample;
                    {
                        __m128 s1 = _mm_setr_ps(loaded_sound->data[ ((s32 *)&s1_ch2_indices)[0] ],
                                                loaded_sound->data[ ((s32 *)&s1_ch2_indices)[1] ],
                                                loaded_sound->data[ ((s32 *)&s1_ch2_indices)[2] ],
                                                loaded_sound->data[ ((s32 *)&s1_ch2_indices)[3] ] );

                        __m128 s2 = _mm_setr_ps(loaded_sound->data[ ((s32 *)&s2_ch2_indices)[0] ],
                                                loaded_sound->data[ ((s32 *)&s2_ch2_indices)[1] ],
                                                loaded_sound->data[ ((s32 *)&s2_ch2_indices)[2] ],
                                                loaded_sound->data[ ((s32 *)&s2_ch2_indices)[3] ] );

                        __m128 a = _mm_mul_ps(s1, _mm_sub_ps(one, fractional_part));
                        __m128 b = _mm_mul_ps(s2, fractional_part);
                        right_channel_sample = _mm_add_ps(a, b);                        
                    }

#if 1
                    assert( in_range( ((f32 *)&left_channel_sample)[0], s16_min_value, s16_max_value )
                            && in_range( ((f32 *)&left_channel_sample)[1], s16_min_value, s16_max_value )
                            && in_range( ((f32 *)&left_channel_sample)[2], s16_min_value, s16_max_value )
                            && in_range( ((f32 *)&left_channel_sample)[3], s16_min_value, s16_max_value )
                            && in_range( ((f32 *)&right_channel_sample)[0], s16_min_value, s16_max_value )
                            && in_range( ((f32 *)&right_channel_sample)[1], s16_min_value, s16_max_value )
                            && in_range( ((f32 *)&right_channel_sample)[2], s16_min_value, s16_max_value )
                            && in_range( ((f32 *)&right_channel_sample)[3], s16_min_value, s16_max_value ) );
#endif
                    
                    __m128 left_dest = _mm_load_ps((float *)&left_channel_dest[0]);
                    __m128 right_dest = _mm_load_ps((float *)&right_channel_dest[0]);

                    __m128 final_output_volume = _mm_mul_ps(_mm_mul_ps(volume_x4, cross_fade_volume_x4), master_and_mix_volume);
                    
                    left_dest = _mm_add_ps(left_dest, _mm_mul_ps(left_channel_sample, final_output_volume));
                    right_dest = _mm_add_ps(right_dest, _mm_mul_ps(right_channel_sample, final_output_volume));
                    
#if 1
                    assert(    in_range( ((f32 *)&left_dest)[0], s16_min_value, s16_max_value )
                            && in_range( ((f32 *)&left_dest)[1], s16_min_value, s16_max_value )
                            && in_range( ((f32 *)&left_dest)[2], s16_min_value, s16_max_value )
                            && in_range( ((f32 *)&left_dest)[3], s16_min_value, s16_max_value )
                            && in_range( ((f32 *)&right_dest)[0], s16_min_value, s16_max_value )
                            && in_range( ((f32 *)&right_dest)[1], s16_min_value, s16_max_value )
                            && in_range( ((f32 *)&right_dest)[2], s16_min_value, s16_max_value )
                            && in_range( ((f32 *)&right_dest)[3], s16_min_value, s16_max_value ) );
#endif
                    
                    _mm_store_ps((float *)left_channel_dest, left_dest);
                    _mm_store_ps((float *)right_channel_dest, right_dest);
                    
                    ++left_channel_dest;
                    ++right_channel_dest;

                    volume_x4 = _mm_add_ps(volume_x4, d_volume_x4);
                    cross_fade_volume_x4 = _mm_add_ps(cross_fade_volume_x4, d_cross_fade_volume_x4);
                }

                s->sample = forward_end_sample_position;
                
                if(volume_finished)
                {
                    s->volume = s->target_volume;
                    s->delta_current_volume = 0.0f;
                }
                else
                {
                    s->volume = ((f32 *)&volume_x4)[3];
                }
                s->volume = clamp01(s->volume);

                if(cross_fade_finished)
                {
                    s->_cross_fade_volume = s->target_cross_fade_volume;
                    s->delta_cross_fade_volume = 0.0f;
                }
                else
                {
                    s->_cross_fade_volume = ((f32*)&cross_fade_volume_x4)[3];
                }
                s->_cross_fade_volume = clamp01(s->_cross_fade_volume);

                if(reset_sample_cursor_after_mixing)
                {
                    reset_sound_sample_cursor(s, reset_position_offset);

                    // NOTE: Have to refill last chunk, as last one would've been padded with silent samples
                    // to fulfil chunk of 4 samples, even though didn't have 4 actual samples to put in
                    if(reset_position_offset != 0)
                    {
                        --left_channel_dest;
                        --right_channel_dest;
                        --chunks_to_mix;
                    }
                }
                
                assert(total_chunks_to_mix >= chunks_to_mix);
                total_chunks_to_mix -= chunks_to_mix;
            }
        }

        Playing_Sound *s = *playing_sound_ptr;
        
        b32 sound_finished = (!s->playing && !s->paused);
        if(sound_finished && s->_owned_by_audio_system)
        {
            // NOTE: Only free the sound if the sound is owned by audio system
                
            *playing_sound_ptr = s->next;
            s->next = audio->first_free_playing_sound;
            audio->first_free_playing_sound = s;
        }
        else
        {
            playing_sound_ptr = &s->next;
        }
    }

    //
    // NOTE: Write out samples from sounds to sample out buffer
    //
    {
        __m128 *source0 = left_channel_mix;
        __m128 *source1 = right_channel_mix;
        __m128i *sample_out = (__m128i *)sound_buffer->samples;
    
        for(u32 chunk_index = 0; chunk_index < chunk_count; ++chunk_index)
        {
            __m128 s0 = _mm_load_ps((float *)source0++);
            __m128 s1 = _mm_load_ps((float *)source1++);

            __m128i l = _mm_cvtps_epi32(s0);
            __m128i r = _mm_cvtps_epi32(s1);

            __m128i lr0 = _mm_unpacklo_epi32(l, r);
            __m128i lr1 = _mm_unpackhi_epi32(l, r);

            __m128i s01 = _mm_packs_epi32(lr0, lr1);
        
            *sample_out++ = s01;
        }
    }
    
    end_temporary_memory(temp_mem);
}
