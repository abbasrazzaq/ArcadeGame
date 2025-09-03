#ifndef TWOSOME_ANIMATION_H
#define TWOSOME_ANIMATION_H


struct Key_Frame
{
    // IMPORTANT: Do NOT re-order these members, it will mess up existing animations initializations
    
    // NOTE: -1 to 1
    Vec2 eye_dir;
    f32 eye_size;

    f32 mouth_size;
    // NOTE: -1 to 1
    f32 mouth_tip_dir;
};

struct Animation_Frame
{
    // NOTE: The time we hit this frame
    f32 time;
    Key_Frame frame;
};

struct Animation
{
    u32 idle_anim;
    f32 current_anim_time;
    u32 current_anim;
    u32 _current_anim_store_index;
    
    Key_Frame current_frame;
    Key_Frame prev_frame;
    Key_Frame next_frame;
    
    f32 anim_change_time;    
    f32 frame_change_time;
    
    s32 last_frame_index;
};

enum Animations
{
    anim_idle,
    anim_noticing_girl,
    anim_about_to_speak,
    anim_yawn,
    anim_clears_throat,
    anim_sigh,
    anim_listening,
    anim_song,
    anim_closing_eyes_and_mouth,
    anim_song_whistle,
    anim_girl_saying_credits,
    anim_count
};

struct Animation_Store
{
    u32 type;
    struct
    {
        Animation_Frame *frames;
        u32 frames_count;
    };
};

//
// NOTE: What follows are global variables for common keyframes, animations and the animation store
//

// NOTE: Blank
global_variable Key_Frame blank =
{
    vec2(0.0f),
    0.0f,
    0.25f,
    0.0f
};
        
// NOTE: Happy
global_variable Key_Frame happy =
{
    vec2(0.5f),
    1.0f,
    1.0f,
    0.5f
};

// NOTE: Sad
global_variable Key_Frame sad =
{
    vec2(0.0f, -0.75f),
    0.75f,
    0.5f,
    -0.75f
};

// NOTE: We make a few key frames and just use these
// throughout singing
global_variable Key_Frame song_1_closed =
{
    vec2(0.0f, -0.25f),
    0.0f,

    0.0f,
    -0.5f,
};

global_variable Key_Frame song_whistle_closed = song_1_closed;
global_variable Key_Frame song_whistle_mid =
{
    song_1_closed.eye_dir,
    song_1_closed.eye_size,

    0.05f,
    song_1_closed.mouth_tip_dir,
};

global_variable Key_Frame song_1_mid =
{
    song_1_closed.eye_dir,
    song_1_closed.eye_size,
    
    0.25f,
    -0.4f
};

global_variable Key_Frame song_1_open =
{
    song_1_closed.eye_dir,
    song_1_closed.eye_size,
    0.5f,
    song_1_closed.mouth_tip_dir
};

global_variable Key_Frame song_2_closed =
{
    vec2(0.0f, 0.25f),
    0.75f,

    0.25f,
    0.0f,
};

global_variable Key_Frame song_2_mid =
{
    song_2_closed.eye_dir,
    song_2_closed.eye_size,
    0.75f,
    song_2_closed.mouth_tip_dir
};

global_variable Key_Frame song_2_open =
{
    song_2_closed.eye_dir,
    song_2_closed.eye_size,
    1.0f,
    song_2_closed.mouth_tip_dir
};

// NOTE: Boy looking sad
global_variable Animation_Frame idle_frames[] =
{
    {
        3.0f,
        sad
    },

    {
        8.0f,
        {
            vec2(-0.25f, -1.0f),
            0.5f,
            0.25f,
            -1.0f,
        }        
    }
};

// NOTE: Noticing girl
global_variable Animation_Frame noticing_girl_frames[] =
{
    {
        1.75f,
        {
            vec2(0.5f, -0.5f),
            0.65f,

            0.2f,
            -0.5f,
        }
    },

    {
        2.75f,
        {
            vec2(0.6f, -0.6f),
            0.6f,

            0.15f,
            -0.25f,
        }
    },

    {
        3.75f,
        {
            vec2(0.6f, -0.6f),
            0.6f,

            0.1f,
            -0.25f,
        }
    },

};

// NOTE: About to speak
global_variable Animation_Frame about_to_speak_frames[] =
{
    {
        1.65f,
        {
            vec2(0.9f, 0.0f),
            0.75f,
            0.8f,
            0.0f
        }        
    },

    {
        1.9f,
        {
            vec2(1.0f, 0.0f),
            0.6f,
            0.7f,
            0.0f
        }        
    },

    {
        3.5f,
        {
            vec2(0.9f, 0.0f),
            0.6f,
            0.7f,
            0.0f
        }
    }
};

// NOTE: Yawn
global_variable Animation_Frame yawn_frames[] =
{
    {
        2.25f,
        {
            vec2(0.0f, -1.0f),
            0.0f,
            1.0f,
            0.0f
        }        
    },
    
    {
        3.25f,
        {
            vec2(0.0f, -0.75f),
            0.25f,
            0.25f,
            0.0f
        }        
    }
};

// NOTE: Clears Throat
global_variable Animation_Frame clears_throat_frames[] =
{
    {
        0.1f,
        {
            sad.eye_dir - vec2(0.1f),
            0.5f,
            
            0.5f,
            sad.mouth_tip_dir,
        }        
    },
            
    {
        0.2f,
        {
            sad.eye_dir - vec2(0.1f),
            0.475f,
                    
            0.45f,
            sad.mouth_tip_dir,
        }
    },

    {
        0.5f,
        {
            sad.eye_dir - vec2(0.1f),
            0.5f,
                    
            0.6f,
            sad.mouth_tip_dir,
        }        
    },
            
    {
        0.6f,
        {
            sad.eye_dir - vec2(0.1f),
            0.475f,
                    
            0.45f,
            sad.mouth_tip_dir,
        }
    }

};

// NOTE: Sigh
global_variable Animation_Frame sigh_frames[] =
{
    {
        2.2f,
        {
            vec2(-0.2f, -0.6f),
            0.0f,
            0.5f,
            -0.25f
        }        
    },
            
    {
        4.0f,
        {
            vec2(-0.1f, -0.7f),
            0.5f,                    
            0.0f,
            0.0f
        }        
    }
};

#if 0
// NOTE: Girl listening to & smiling at someone talking
global_variable Animation_Frame listening_frames[] =
{
    {
        3.0f,
        {
            vec2(1.0f, 0.0f),
            0.9f,

            0.1f,
            0.0f
        }
    },
    
    {
        6.0f,
        {
            vec2(1.0f, 0.0f),
            0.8f,

            0.5f,
            0.0f
        }
    },
    
    {
        10.0f,
        {
            vec2(0.9f, 0.0f),
            0.85f,

            0.3f,
            0.0f
        }        
    },
    
    {
        13.0f,
        {
            vec2(0.9f, 0.0f),
            1.0f,

            0.7f,
            0.5f
        }        
    },
    
    {
        15.0f,
        {
            vec2(1.0f, 0.0f),
            1.0f,

            0.9f,
            0.6f
        }        
    },
    
    {
        20.0f,
        {
            vec2(0.9f, 0.0f),
            1.0f,

            0.85f,
            0.7f
        }        
    },

    {
        23.0f,
        {
            vec2(0.9f, 0.0f),
            1.0f,

            0.7f,
            0.8f
        }        
    }
};
#else
// NOTE: Girl listening to & smiling at someone talking
global_variable Animation_Frame listening_frames[] =
{
    {
        3.0f,
        {
            vec2(1.0f, 0.0f),
            0.8f,

            0.1f,
            0.0f
        }
    },
    
    {
        6.0f,
        {
            vec2(1.0f, 0.0f),
            0.7f,

            0.4f,
            0.0f
        }
    },
    
    {
        10.0f,
        {
            vec2(0.9f, 0.0f),
            0.55f,

            0.2f,
            0.0f
        }        
    },
    
    {
        13.0f,
        {
            vec2(0.9f, 0.0f),
            0.8f,

            0.5f,
            0.4f
        }        
    },
    
    {
        15.0f,
        {
            vec2(1.0f, 0.0f),
            0.8f,

            0.6f,
            0.5f
        }        
    },
    
    {
        20.0f,
        {
            vec2(0.9f, 0.0f),
            0.8f,

            0.65f,
            0.6f
        }        
    },

    {
        23.0f,
        {
            vec2(0.9f, 0.0f),
            0.8f,

            0.5f,
            0.7f
        }        
    }
};
#endif

global_variable Animation_Frame closing_eyes_and_mouth_frames[] =
{
    { 2.0f, song_1_closed },
    { 3.0f, song_1_closed },
};

global_variable Animation_Frame song_whistle_frames[] =
{
    { 0.1f, song_whistle_mid },
    { 1.0f, song_whistle_closed },
    { 1.5f, song_whistle_mid },
    { 2.25f, song_whistle_closed },
    { 3.25f, song_whistle_mid },
    { 3.75f, song_whistle_closed },
};

global_variable Animation_Frame song_frames[] =
{
    // NOTE: Part 1
    { 2.0f, song_1_closed },
    { 4.0f, song_1_closed },
    { 4.5f, song_1_open },
    { 5.3f, song_1_closed },
    { 5.8f, song_1_open },
    { 6.0f, song_1_mid },
    { 6.2f, song_1_closed },    
    { 7.2f, song_1_mid },       
    { 7.8f, song_1_open },
    { 8.3f, song_1_mid },    
    { 8.9f, song_1_closed },
    { 9.2f, song_1_mid },
    { 9.6f, song_1_closed },
    { 9.9f, song_1_mid },
    { 10.4f, song_1_open },
    { 11.4f, song_1_closed },
    { 11.6f, song_1_open },
    { 11.9f, song_1_mid },
    { 13.0f, song_1_open },
    { 13.5f, song_1_mid },
    { 14.2f, song_1_open },
    { 15.1f, song_1_mid },
    { 16.0f, song_1_closed },
    { 16.7f, song_1_open },
    { 17.3f, song_1_closed },
    { 18.7f, song_1_open },
    { 18.9f, song_1_closed },
    { 19.4f, song_1_mid },    
    { 19.7f, song_1_open },
    { 20.4f, song_1_closed },
    { 20.9f, song_1_open },
    { 21.4f, song_1_closed },
    { 22.3f, song_1_mid },
    { 22.3f, song_1_mid },    
    { 23.1f, song_1_open },
    { 23.9f, song_1_closed },
    { 24.4f, song_1_open },
    { 25.4f, song_1_mid },
    { 26.6f, song_1_open },
    { 26.8f, song_1_mid },
    { 27.9f, song_1_open },
    { 28.2f, song_1_mid },
    { 29.2f, song_1_closed },
    { 29.9f, song_1_open },
    { 30.4f, song_1_closed },
    { 31.5f, song_1_open },
    { 32.0f, song_1_closed },
    { 33.2f, song_1_mid },
    { 34.5f, song_1_closed },
    { 36.9f, song_1_open },
    { 37.9f, song_1_mid },
    { 38.6f, song_1_closed },
    { 39.3f, song_1_open },
    { 39.9f, song_1_closed },
    { 40.7f, song_1_mid },
    { 41.6f, song_1_open },
    { 42.3f, song_1_closed },
    { 43.0f, song_1_mid },
    { 43.6f, song_1_open },
    { 43.6f, song_1_mid },
    { 44.4f, song_1_closed },
    
    // NOTE: Pause
    { 46.0f, song_2_closed },
    { 47.2f, song_2_closed },
    
    // NOTE: Part 2
    { 47.5f, song_2_open },
    { 48.4f, song_2_mid },
    { 48.9f, song_2_closed },
    { 49.4f, song_2_open },
    { 50.0f, song_2_closed },
    { 50.4f, song_2_mid },
    { 51.0f, song_2_open },
    { 51.5f, song_2_mid },
    { 51.9f, song_2_open },
    { 52.5f, song_2_closed },
    { 53.0f, song_2_closed },
    { 53.5f, song_2_mid },
    { 54.3f, song_2_open },
    { 54.3f, song_2_closed },
    { 55.1f, song_2_open },
    { 56.0f, song_2_mid },
    { 57.0f, song_2_closed },
    { 58.1f, song_2_open },
    { 58.9f, song_2_mid },
    { 59.1f, song_2_closed },
    { 59.7f, song_2_open },
    { 60.0f, song_2_closed },
    { 61.0f, song_2_mid },
    { 62.1f, song_2_open },
    { 62.7f, song_2_mid },
    { 63.5f, song_2_open },
    { 64.0f, song_2_closed },
    { 64.7f, song_2_open },
    { 65.0f, song_2_mid },
    { 65.9f, song_2_closed },
    { 66.9f, song_2_open },
    { 67.9f, song_2_closed },
    { 68.7f, song_2_mid },
    { 69.1f, song_2_closed },
    { 69.9f, song_2_open },
    { 70.5f, song_2_closed },
    { 71.8f, song_2_mid },
    { 72.6f, song_2_open },
    { 73.0f, song_2_mid },
    { 73.8f, song_2_closed },
    { 74.5f, song_2_mid },
    { 75.0f, song_2_open },
    { 76.0f, song_2_closed },
    { 76.8f, song_2_mid },
    { 77.4f, song_2_open },
    { 78.2f, song_2_mid },
    { 78.9f, song_2_closed },
    { 79.7f, song_2_mid },
    { 80.4f, song_2_closed },
    { 81.2f, song_2_mid },
    { 82.1f, song_2_closed },
    { 83.0f, song_2_open },
    { 83.5f, song_2_mid },
    { 84.2f, song_2_open },
    { 85.1f, song_2_mid },
    { 86.1f, song_2_open },
    { 87.2f, song_2_mid },
    { 89.0f, song_2_closed },
    
};

global_variable Key_Frame saying_credits_mid =
{
    vec2(0.0f, 0.0f),
    0.6f,

    0.5f,
    0.15f
    
};

global_variable Key_Frame saying_credits_closed =
{
    vec2(0.0f),
    0.5f,

    0.0f,
    0.1f,
};

global_variable Key_Frame saying_credits_open =
{
    vec2(0.0f),
    0.65f,

    0.55f,
    0.25f
};

global_variable Animation_Frame girl_saying_credits_frames[] =
{
    {
        0.4f,
        saying_credits_mid
    },

    {
        0.8f,
        saying_credits_closed,
    },

    {
        1.5f,
        saying_credits_closed
    },

    {
        1.8f,
        saying_credits_mid
    },

    {
        2.0f,
        saying_credits_open
    },

    {
        2.3f,
        saying_credits_mid
    },

    {
        2.8f,
        saying_credits_open
    },
    
    {
        3.4f,
        saying_credits_closed
    },

    {
        3.7f,
        saying_credits_mid
    },

    {
        5.0f,
        saying_credits_closed
    },

    {
        6.0f,
        saying_credits_closed
    },
};

#define store_anim(anim) (anim), array_count((anim))
global_variable Animation_Store animation_store[] =
{
    { anim_idle, store_anim(idle_frames) },
    { anim_about_to_speak, store_anim(about_to_speak_frames) },
    { anim_noticing_girl, store_anim(noticing_girl_frames) },
    { anim_yawn, store_anim(yawn_frames) },
    { anim_clears_throat, store_anim(clears_throat_frames) },
    { anim_sigh, store_anim(sigh_frames) },
    { anim_listening, store_anim(listening_frames) },
    { anim_song, store_anim(song_frames) },
    { anim_closing_eyes_and_mouth, store_anim(closing_eyes_and_mouth_frames) },
    { anim_song_whistle, store_anim(song_whistle_frames) },
    { anim_girl_saying_credits, store_anim(girl_saying_credits_frames) }
    
};

#endif
