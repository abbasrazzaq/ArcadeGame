#ifndef TWOSOME_GAME_HUB_H
#define TWOSOME_GAME_HUB_H


#define number_of_levels game_level_type_count
#define hub_entity_line_thickness 8.0f

#define prepping_to_sing_state_time 5.0f

// NOTE: Song script points (seconds)
#define song_script_point_start_singing 11.0f
#define song_script_point_girl_turns_around 39.0f
#define song_script_point_girl_flashes 45.0f
#define song_script_point_fireworks_explode 46.7f

#define boy_side_background_colour vec3(1.0f)
// NOTE: Both sides are the same colour at the start
#define girl_side_intro_background_colour boy_side_background_colour
#define girl_side_normal_background_colour vec3(0.0f)
//#define girl_side_boy_singing_background_colour (safe_element_colour*0.5f)
#define girl_side_boy_singing_background_colour (safe_element_colour*0.25f)

#define prepping_to_sing_level_select_swell_time 1.25f
#define prepping_to_sing_level_select_ray_direction_change_start_time (prepping_to_sing_level_select_swell_time + 0.25f)
#define prepping_to_sing_level_select_ray_direction_time 1.0f

#define time_for_girl_to_fade_in 1.5f
#define girl_fades_in_state_time (time_for_girl_to_fade_in + 1.0f)
#define time_for_girl_to_turn_to_boy 5.0f

#define face_fast_flash_value_time 2.0f
#define face_slow_flash_value_time 8.0f

#define background_melody_interval 6.0f
#define background_melody_interval_variation_start 0.6f
#define background_melody_interval_variation_end 1.4f

#define hub_progress_button_radius 100.0f

#define firework_fade_out_time 2.5f

#define firework_smallest_max_scale virtual_screen_width*0.25f
#define firework_largest_max_scale virtual_screen_width*0.75f

#define hub_ui_elements_fade_speed (1.0f / 0.25f)

struct Hub_Progress_Button
{
    Vec2 position;
    Vec2 scale;

    Vec2 goal_positions[number_of_levels];
    Vec2 goal_scale;
    
    Flashing_Value flash;
};

struct Level_Select
{
    Vec2 position;
    Vec2 scale;
    s32 level_type;
    b32 connected;
    b32 unlocked;
};

enum Game_Hub_State
{
    // IMPORTANT: These states need to be in the order that they occur
    game_hub_state_boy_alone            = 0,
    game_hub_state_girl_fades_in        = 1,
    game_hub_state_boy_notices_girl     = 2,
    game_hub_state_boy_attempts_chat    = 3,
    game_hub_state_normal               = 4,
    game_hub_state_boy_prepping_to_sing = 5,
    game_hub_state_boy_singing          = 6,
    game_hub_state_boy_girl_talk        = 7,
};

struct Boy_Idle_Anim
{
    u32 anim;
    u32 sound;
};

struct Boy_Face
{
    Vec2 position;
    Vec2 scale;
    b32 has_sang;    
    // NOTE: This is just a state to use whilst singing
    b32 whistling;
    
    Flashing_Value colour_flash;
    
    f32 next_time_to_play_special_idle_anim;

    Boy_Idle_Anim idle_anims[4];
    u32 idle_anims_played;
    
    Animation anim;
};

struct Hair_Strand
{
    f32 bob_speed;
    f32 bob_size;
    f32 bob_velocity;
    
    Vec2 position;
    Vec2 scale;
    f32 start_rot;
    f32 end_rot;

    f32 bob;
};

enum Girl_Movement_Flags
{
    girl_movement_flag_turn_to_boy = 1 << 0,
    girl_movement_flag_move_to_boy = 1 << 1
};

struct Girl_Face
{
    Vec2 original_position;
    Vec2 position;
    Vec2 scale;

    f32 rot_t;

    u32 movement_flags;
    
    Hair_Strand strands[6];
    
    Flashing_Value colour_flash;

    Animation anim;

    b32 saying_credits;
};

struct Firework
{
    Vec2 position;
    f32 radius;
    f32 max_radius;
    Vec3 colour;
    Vec2 velocity;
    
    f32 explode_time;
    f32 launch_time;
    
    b32 exploded;
    b32 fade_out;

    s32 shape;
    
    Firework *next;
};

struct Hub_UI_Fade
{
    f32 progress_button;
    f32 selector_buttons;


    f32 credits_button;
};

struct Game_Hub
{
    s32 state;
    f32 state_change_time;
    
    u32 level_scores_to_get[number_of_levels];
    Level_Select level_selects[number_of_levels];
    s32 last_level_select_index_hovered_over;
    
    Hub_Progress_Button progress_button;

    Boy_Face boy_face;
    Girl_Face girl_face;
    Vec3 girl_side_background_colour;

    Hub_UI_Fade ui_fade;

    // NOTE: Used to select one of the chord assets to play as background music
    u32 current_background_chord_type_index;
    u32 background_melody_index;
    f32 background_melody_last_played_time;
    f32 background_melody_speed;

    Playing_Sound *boys_song_whistle;
    Playing_Sound *boys_song;
    Overlapping_Sound *background_ambience;
    Playing_Sound *girl_saying_credits;

    Flashing_Value score_meter_goal_flash;
    Flashing_Value score_meter_fade;
    Flashing_Value next_level_select_flash;

    Memory_Arena *mode_arena;
    
    Firework *first_active_firework;
    Firework *first_free_firework;
    f32 next_firework_spawn_time;
};

#endif
