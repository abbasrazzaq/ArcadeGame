#include "twosome_animation.h"


internal void init_anim(Animation *anim, Key_Frame start_frame)
{
    anim->last_frame_index = -1;
    anim->prev_frame = start_frame;
    anim->current_frame = anim->prev_frame;
    anim->next_frame = anim->prev_frame;
}

internal void jump_to_anim_index(Animation *anim, u32 index)
{    
    Animation_Store *anim_store = &animation_store[anim->_current_anim_store_index];
    assert(index < anim_store->frames_count);
    
    anim->last_frame_index = -1;
    anim->prev_frame = anim_store->frames[index].frame;
    anim->current_frame = anim->prev_frame;
    
    anim->current_anim_time = anim_store->frames[index].time;
    anim->frame_change_time = anim->current_anim_time;

    u32 next_frame_index = (index + 1) % anim_store->frames_count;
    assert(next_frame_index < anim_store->frames_count);
    anim->next_frame = anim_store->frames[next_frame_index].frame;
}

internal void jump_to_first_anim_frame(Animation *anim)
{
    jump_to_anim_index(anim, 0);
}

internal void jump_to_random_anim_frame(Animation *anim, RNG *rng)
{
    Animation_Store *anim_store = &animation_store[anim->_current_anim_store_index];
    jump_to_anim_index(anim, random_exclusive(rng, 0, anim_store->frames_count));
}

internal void set_anim(Animation *anim, u32 anim_type)
{
    anim->current_anim = anim_type;    
    anim->current_frame = anim->current_frame;
    anim->prev_frame = anim->current_frame;
    anim->last_frame_index = -1;
    
    anim->current_anim_time = 0.0f;
    anim->anim_change_time = 0.0f;
            
    b32 found_anim = false;
    for(Memory_Index anim_store_index = 0; anim_store_index < array_count(animation_store); ++anim_store_index)
    {
        if(animation_store[anim_store_index].type == anim_type)
        {
            anim->_current_anim_store_index = anim_store_index;
            found_anim = true;
            break;
        }
    }

    assert(found_anim);
}

internal void set_idle_anim(Animation *anim, u32 anim_type)
{
    set_anim(anim, anim_type);
    anim->idle_anim = anim_type;    
}

internal void cancel_non_idle_anim(Animation *anim, b32 jump_to_first_idle_frame = false)
{
    // NOTE: If current animation isn't idle, we go back to idle
    if(anim->current_anim != anim->idle_anim)
    {
        set_anim(anim, anim->idle_anim);
        
        if(jump_to_first_idle_frame)
        {
            jump_to_first_anim_frame(anim);            
        }
    }
}

internal b32 is_current_anim_idle(Animation *anim)
{
    b32 result = (anim->current_anim == anim->idle_anim);
    return result;
}

internal void update_animation(Animation *anim, f32 dt)
{
    anim->current_anim_time += dt;

    // NOTE: Find the frame that we should be interpolating to
    s32 next_frame_index = -1;
    Animation_Frame next_anim_frame = {};
    
    // NOTE: If the animation finishes, we'll need to go a second time when change the animation to idle
    while(next_frame_index < 0)
    {        
        Animation_Store *anim_store = &animation_store[anim->_current_anim_store_index];
    
        Animation_Frame *frames = anim_store->frames;
        s32 frames_count = anim_store->frames_count;
    
        assert(frames && frames_count);    
        
        for(s32 key_frame_index = 0; key_frame_index < frames_count; ++key_frame_index)
        {
            if(frames[key_frame_index].time >= anim->current_anim_time)
            {
                // NOTE: If it's a new frame then we need to record when we changed
                if(key_frame_index != anim->last_frame_index)
                {
                    anim->frame_change_time = anim->current_anim_time;

                    if(anim->last_frame_index >= 0)
                    {
                        anim->prev_frame = frames[anim->last_frame_index].frame;
                    }
                        
                    anim->last_frame_index = key_frame_index;
                }

                next_frame_index = key_frame_index;
                anim->next_frame = frames[next_frame_index].frame;
                break;
            }
        }

        if(next_frame_index > -1)
        {
            next_anim_frame = frames[next_frame_index];
        }
        else
        {
            // NOTE: Current animation finished, go back to idle
            set_anim(anim, anim->idle_anim);
        }
    }

    // NOTE: Interpolate between animation frames
    
    Key_Frame next = next_anim_frame.frame;

    // NOTE: The frame duration is just how long before next frame has to start
    f32 frame_duration = (next_anim_frame.time - anim->frame_change_time);
    assert(frame_duration > 0.0f);
    
    f32 frame_time_left = (next_anim_frame.time - anim->current_anim_time);
    
    f32 frame_t = (1.0f - (frame_time_left / frame_duration));
    frame_t = clamp01(frame_t);

    Key_Frame tween_frame = {};
    tween_frame.eye_dir = interpolate(anim->prev_frame.eye_dir, next.eye_dir, frame_t);
    tween_frame.eye_size = interpolate(anim->prev_frame.eye_size, next.eye_size, frame_t);

    tween_frame.mouth_size = interpolate(anim->prev_frame.mouth_size, next.mouth_size, frame_t);
    tween_frame.mouth_tip_dir = interpolate(anim->prev_frame.mouth_tip_dir, next.mouth_tip_dir, frame_t);

    anim->current_frame = tween_frame;
    
}
